#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#ifdef USE_LIBUNISTRING
#include <unistr.h>
#endif
#include "utils.h"
#include "rxb.h"

#define MAX_REQUEST_SIZE (64 * 1024)

/* Gestore del segnale SIGCHLD */
void handler(int signo)
{
    int status;
    (void)signo;

    /* gestisco tutti i figli terminati */
    while (waitpid(-1, &status, WNOHANG) > 0)
    {
        continue;
    }
}

int main(int argc, char **argv)
{
    struct addrinfo hints, *res;
    int err, sd, ns, pid, on;

    struct sigaction sa;

    /* Controllo argomenti */
    if (argc != 2)
    {
        fprintf(stderr, "Uso: %s porta\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Ignora segnale SIGPIPE:
      Se il client chiude il socket e il server prova a scriver --> SIGPIPE --> default = kill del server.
      Ignorandolo evita la terminazione del processo.
      */
    signal(SIGPIPE, SIG_IGN);

    /* Installo gestore SIGCHLD usando la syscall sigaction, che è uno
     * standard POSIX, al posto di signal che ha semantiche diverse a
     * seconda del sistema operativo */
    sigemptyset(&sa.sa_mask);
    /* uso SA_RESTART per evitare di dover controllare esplicitamente se
     * accept è stata interrotta da un segnale e in tal caso rilanciarla
     * (si veda il paragrafo 21.5 del testo M. Kerrisk, "The Linux
     * Programming Interface") */
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = handler;

    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    /* Prepare getaddrinfo */
    memset(&hints, 0, sizeof(hints));
    /* Usa AF_INET per forzare solo IPv4, AF_INET6 per forzare solo IPv6 , tutti e due uso AF_UNSPEC*/
    hints.ai_family = AF_UNSPEC;     // compatibile IPv4 e IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;     // produce un indirizzo "bindabile" (equivalende a 0.0.0.0 o ::

    /* Passando NULL come hostname, AI_PASSIVE fa si che il server si vincoli a tutte le interfacce di rete. */
    if ((err = getaddrinfo(NULL, argv[1], &hints, &res)) != 0) /*getaddrinfo() è una funzione standard disponibile in C (e in molte API di rete, anche su Windows) che serve per risolvere un nome host (dominio) o un indirizzo di rete in uno o più indirizzi IP + informazioni di socket*/
    {
        fprintf(stderr, "Errore setup indirizzo bind: %s\n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }
     /* Crea un passivo (listening) socket TCP nella famiglia fornita da getaddrinfo (IPv4 o IPv6) */
    if ((sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
    {
        printf("Errore in socket");
        exit(EXIT_FAILURE);
    }
      /* Serve per:
        - permettere di riavviare il server subito dopo la chiusura
        - evitare errori "address already in use"

        Altrimenti TCP mantiene la porta in stato TIME_WAIT per 2 minuti
        */

    on = 1;

    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
     /* Il socket viene associato:
        - all'indirizzo IP scelto (tutti, con AI_PASSIVE)
        - alla porta indicata da argv[1]
        Se fallisce --> molto probabilmente la porta è occupata (o permessi insufficienti)
        */

    if (bind(sd, res->ai_addr, res->ai_addrlen) < 0)
    {
        perror("Errore in bind");
        exit(EXIT_FAILURE);
    }

     /* At this point, I can free the memory allocated by getaddrinfo */
    freeaddrinfo(res);


     /*
        Trasformo il socket in listening socket: riceve richieste di connessione.
        SOMAXCONN è la massima dimensione possibile della coda di connessioni pendenti consentita dal sistema.
        */
    if (listen(sd, SOMAXCONN) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    /* Attendo i client... */

    for (;;)
    {

        printf("Server in ascolto...\n");


          /*
                - blocca finchè arriva un client
                - crea un nuovo socket attivo (ns)
                - il socket passivo (sd) rimane in ascolto

                Gestione EINTR:
                se arriva un segnale --> accept() fallisce con EINTR --> si deve ripetere l'operazione
                Questo è lo standard POSIX
                */

        if ((ns = accept(sd, NULL, NULL)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        /* Generazione di un figlio */

        if ((pid = fork()) < 0)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            // figlio
            int pid1, pid2, p1p2[2], status;

            const char *end_request = "\n--- END REQUEST ---\n";
            rxb_t rxb;

            close(sd);

            /* Disabilito gestore SIGCHLD */
            memset(&sa, 0, sizeof(sa));
            sigemptyset(&sa.sa_mask);
            sa.sa_handler = SIG_DFL;

            if (sigaction(SIGCHLD, &sa, NULL) == -1)
            {
                perror("sigaction");
                exit(EXIT_FAILURE);
            }

            /* Inizializzo buffer di ricezione */
            rxb_init(&rxb, MAX_REQUEST_SIZE);

            /* Avvio ciclo gestione richieste */
            for (;;)
            {

                char nome[MAX_REQUEST_SIZE];
                size_t nome_len;
                char annata[MAX_REQUEST_SIZE];
                size_t annata_len;

                /* Inizializzo il buffer nome a zero e non uso
                 * l'ultimo byte, così sono sicuro che il contenuto del
                 * buffer sarà sempre null-terminated. In questo modo,
                 * posso interpretarlo come una stringa C e passarlo
                 * direttamente alla funzione execlp. Questa è
                 * un'operazione che va svolta prima di ogni nuova
                 * richiesta. */
                memset(nome, 0, sizeof(nome));
                nome_len = sizeof(nome) - 1;

                /* Leggo richiesta da Client */
                if (rxb_readline(&rxb, ns, nome, &nome_len) < 0)
                {
                    /* Se sono qui, è perché ho letto un
                     * EOF. Significa che il Client ha
                     * chiuso la connessione, per cui
                     * dealloco rxb ed esco dal ciclo di
                     * gestione delle richieste. */

                    rxb_destroy(&rxb);
                    break;
                }
/* Verifico che il messaggio sia UTF-8 valido */
#ifdef USE_LIBUNISTRING
                if (u8_check((uint8_t *)nome, nome_len) != NULL)
                {
                    /* Client che malfunziona - inviato messaggio
                     * con stringa UTF-8 non valida */
                    fprintf(stderr, "Request is not valid UTF-8");
                    close(ns);
                    exit(EXIT_FAILURE);
                }
#endif

                /* Inizializzo il buffer annata a zero e non uso
                 * l'ultimo byte, così sono sicuro che il contenuto del
                 * buffer sarà sempre null-terminated. In questo modo,
                 * posso interpretarlo come una stringa C e passarlo
                 * direttamente alla funzione execlp. Questa è
                 * un'operazione che va svolta prima di ogni nuova
                 * richiesta. */
                memset(annata, 0, sizeof(annata));
                annata_len = sizeof(annata) - 1;

                // leggo richiesta da client
                if (rxb_readline(&rxb, ns, annata, &annata_len) < 0)
                {
                    /* Se sono qui, è perché ho letto un
                     * EOF. Significa che il Client ha
                     * chiuso la connessione, per cui
                     * dealloco rxb ed esco dal ciclo di
                     * gestione delle richieste. */

                    rxb_destroy(&rxb);
                    break;
                }
/* Verifico che il messaggio sia UTF-8 valido */
#ifdef USE_LIBUNISTRING
                if (u8_check((uint8_t *)annata, annata_len) != NULL)
                {
                    /* Client che malfunziona - inviato messaggio
                     * con stringa UTF-8 non valida */
                    fprintf(stderr, "Request is not valid UTF-8");
                    close(ns);
                    exit(EXIT_FAILURE);
                }
#endif

                /* Creo pipe, la crea il figlio dando vita a nipoti */

                if (pipe(p1p2) < 0)
                {
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }

                if ((pid1 = fork()) < 0)
                {
                    perror("seconda fork");
                    exit(EXIT_FAILURE);
                }
                else if (pid1 == 0)
                {
                    // nipote 1
                    // chiudo descrittori che non servono
                    close(ns);
                    close(p1p2[0]);
                    // ridireziono stdout
                    close(1);
                    if (dup(p1p2[1]) < 0)
                    {
                        perror("dup");
                        exit(EXIT_FAILURE);
                    }
                    close(p1p2[1]);

                    // eseguo grep selezionando solo vini con quel nome, leggo dal file di testo magazzino.txt
                    execlp("grep", "grep", nome, "magazzino.txt", (char *)NULL);
                    perror("grep1");
                    exit(EXIT_FAILURE);
                }

                if ((pid2 = fork()) < 0)
                {
                    perror("terza fork");
                    exit(EXIT_FAILURE);
                }
                else if (pid2 == 0)
                {
                    // nipote 2

                    // chiudo descrittori non necessari

                    close(p1p2[1]);

                    // ridireziono stdin
                    close(0);
                    if (dup(p1p2[0]) < 0)
                    {
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }
                    close(p1p2[0]);

                    // ridirezione stdout
                    close(1);
                    if (dup(ns) < 0)
                    {
                        perror("dup3");
                        exit(EXIT_FAILURE);
                    }
                    close(ns);

                    execlp("grep", "grep", annata, (char *)NULL);
                    perror("grep2");
                    exit(EXIT_FAILURE);
                }
                // figlio

                // chiudo descrittori non necessari
                close(p1p2[1]);
                close(p1p2[0]);

                // aspetto terminazione figli
                waitpid(pid1, &status, 0);
                waitpid(pid2, &status, 0);

                // mando una stringa di notifica fine richieste
                if (write_all(ns, end_request, strlen(end_request)) < 0)
                {
                    perror("write");
                    exit(EXIT_FAILURE);
                }
            }
            /*
                Chiusura della scoket: torna ad attendere il prossimo client
                Questo è un server transient: una connessione --> una richiesta --> una risposta --> chiudi
                */
            close(ns);
            // termino figlio
            exit(EXIT_SUCCESS);
        }
        // padre
        // chiudo socket attiva
        close(ns);
    }
    // chiudo socket passiva
    close(sd);

    return 0;
}