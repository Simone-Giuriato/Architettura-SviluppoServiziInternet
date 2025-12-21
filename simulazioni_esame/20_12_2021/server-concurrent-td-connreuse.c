#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#ifdef USE_LIBUNISTRING
#include <unistr.h>
#endif
#include "rxb.h"
#include "utils.h"

#define MAX_REQUEST_SIZE (64 * 1024)

void handler(int signo) //gestione segnale SIGCHLD
{
    int status;
    (void)signo;
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

    if (argc != 2)
    {
        fprintf(stderr, "Uso: %s porta\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    signal(SIGPIPE, SIG_IGN);   //ignoro segnale, evitando così che programma termini se provo a scirvere su socket chiusa dal client

    sigemptyset(&sa.sa_mask);   //inizializzo gestore segnali sa per la terminazione dei figli
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = handler;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)    //gestisce i figli terminti in modo asincrono
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;        //supporto sia Ipv4 e Ipv6
    hints.ai_socktype = SOCK_STREAM;    //socket di tipo STREAM
    hints.ai_flags = AI_PASSIVE;        //server agisce in modo passivo, attende connessioni

    if ((err = getaddrinfo(NULL, argv[1], &hints, &res)) != 0)  //ottengo info sull'indirizzo del server in base ai parametri passati
    {
        fprintf(stderr, "Errore setup indirizzo bind%s\n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }

    if ((sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)  //creo socket in base ai valori restituiti da getaddrinfo
    {
        perror("Errore in socket");
        exit(EXIT_FAILURE);
    }

    on = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)  //opzione SO_REUSEADDR per consentire il riutilizzo immediato dell'indirizzo di socke in caso di riavvio rapido
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    if (bind(sd, res->ai_addr, res->ai_addrlen) < 0)    //associo il socket all'indirizzo specificato da getaddrinfo
    {
        perror("Errore in bind");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);  //libero memoria dalle risorse allocate dalla getaddrinfo

    if (listen(sd, SOMAXCONN) < 0)  //entra in modalità di ascolto
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    for (;;)    //ciclo infinito per accettare le connessioni in entrata, per ogni connessione accettata creo un figlio, il padre rimane in ascolto per accettare connessioni
    {
        printf("Server in ascolto...\n");

        if ((ns = accept(sd, NULL, NULL)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        if ((pid = fork()) < 0)
        {
            perror("FORK");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            // FIGLIO si occuperà della lettura dei dati, esecuzione comandi (nipoti), scrittura risultati sulla connessione e chiusura connessione

            int status, pid1, pid2, p1p2[2];
            const char *end_request = "\n--- END REQUEST ---\n";
            rxb_t rxb;

            close(sd);  //chiusura del socket del processo padre

            memset(&sa, 9, sizeof(sa));
            sigemptyset(&sa.sa_mask);
            sa.sa_handler = SIG_DFL;
            if (sigaction(SIGCHLD, &sa, NULL) == -1)
            {
                perror("sigaction");
                exit(EXIT_FAILURE);
            }
            rxb_init(&rxb, MAX_REQUEST_SIZE * 2);   //inizializzazione struttura rxb per leggere dati dalla connessione

            for (;;)
            {
                char categoria[1024];
                char produttore[1024];
                char ordinamento[1024];

                size_t categoria_len;
                size_t produttore_len;
                size_t ordinamento_len;

                // lettura categoria dalla connessione
                memset(categoria, 0, sizeof(categoria));
                categoria_len = sizeof(categoria) - 1;

                if (rxb_readline(&rxb, ns, categoria, &categoria_len) < 0)
                {

                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }

#ifdef USE_LIBUNISTRING
                if (u8_check((uint8_t *)categoria, categoria_len) != NNULL)
                {
                    fprintf(stderr, "Request is not valid UTF-8\n");
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }
#endif

                // lettura produttore dalla connessione
                memset(produttore, 0, sizeof(produttore));
                produttore_len = sizeof(produttore) - 1;

                if (rxb_readline(&rxb, ns, produttore, &produttore_len) < 0)
                {

                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }

#ifdef USE_LIBUNISTRING
                if (u8_check((uint8_t *)produttore, produttore_len) != NNULL)
                {
                    fprintf(stderr, "Request is not valid UTF-8\n");
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }
#endif

                // lettura ordinamento dalla connessione
                memset(ordinamento, 0, sizeof(ordinamento));
                ordinamento_len = sizeof(ordinamento) - 1;

                if (rxb_readline(&rxb, ns, ordinamento, &ordinamento_len) < 0)
                {

                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }

#ifdef USE_LIBUNISTRING
                if (u8_check((uint8_t *)ordinamento, ordinamento_len) != NNULL)
                {
                    fprintf(stderr, "Request is not valid UTF-8\n");
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }
#endif

                //(uso il percorso ./last_minute_gifts/categoria.txt al posto di /var/local/last_minute_gifts).
                char nomefile[MAX_REQUEST_SIZE];
                snprintf(nomefile, sizeof(nomefile), "./last_minute_gifts/%s.txt", categoria);

                if (pipe(p1p2) < 0)
                {
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }

                if ((pid1 = fork()) < 0)
                {
                    perror("fork1");
                    exit(EXIT_FAILURE);
                }
                else if (pid1 == 0)
                {
                    // NIPOTE 1
                    close(ns);
                    close(p1p2[0]);

                    // ridirezione stdout
                    close(1);
                    if (dup(p1p2[1]) < 0)
                    {
                        perror("dup1");
                        exit(EXIT_FAILURE);
                    }

                    execlp("grep", "grep", produttore, nomefile, (char *)NULL); //seleziono solo quelli del produttore desiderato
                    perror("grep");
                    exit(EXIT_FAILURE);
                }

                if ((pid2 = fork()) < 0)
                {
                    perror("fork2");
                    exit(EXIT_FAILURE);
                }
                else if(pid2 == 0)
                {
                    // NIPOTE 2
                    close(p1p2[1]);

                    // ridirezione stdin
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
                        perror("dup ns");
                        exit(EXIT_FAILURE);
                    }
                    close(ns);

                    // capisco se vuole crescente o decresente
                    if (strcmp(ordinamento, "crescente") == 0)
                    {

                        // CRESCENTE
                        execlp("sort", "sort", "-n", (char *)NULL);
                        perror("sort");
                        exit(EXIT_FAILURE);
                    }
                    else
                    {

                        // DESCRESCENTE
                        execlp("sort", "sort", "-n", "-r", (char *)NULL);
                        perror("sort");
                        exit(EXIT_FAILURE);
                    }
                }
                close(p1p2[0]);
                close(p1p2[1]);

                waitpid(pid1, &status, 0);
                waitpid(pid2, &status, 0);

                if (write_all(ns, end_request, strlen(end_request)) < 0)
                {
                    perror("write");
                    exit(EXIT_FAILURE);
                }
            }
            rxb_destroy(&rxb);
            close(ns);
            exit(EXIT_FAILURE);
        }
        close(ns);
    }
    close(sd);

    return 0;
}
