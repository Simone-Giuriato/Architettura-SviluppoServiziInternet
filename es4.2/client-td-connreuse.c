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

int main(int argc, char **argv)
{
    int err;
    struct addrinfo hints, *res, *ptr;
    char *host_remoto;
    char *servizio_remoto;
    int sd, i = 1;

    rxb_t rxb;

    /* Controllo argomenti */
    if (argc < 3)
    {
        fprintf(stderr, "Uso %s server porta", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Ignora SIGPIPE
       Quando un client scrive su un socket chiuso dal server il kernel invia SIGPIPE:
       - Comportamento di default --> terminazione del processo
       - Qui si vuole evitare crash improvvisi
       */
    signal(SIGPIPE, SIG_IGN);

    /* Costruzione dell'indirizzo */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    /* Risoluzione dell'host */
    host_remoto = argv[1];
    servizio_remoto = argv[2];

    if ((err = getaddrinfo(host_remoto, servizio_remoto, &hints, &res)) != 0)
    {
        fprintf(stderr, "Errore risoluzione nome %s\n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }

    /* connessione con fallback */
    for (ptr = res; ptr != NULL; ptr = ptr->ai_next)
    {
        // Prova a creare un socket usando la famiglia, tipo e protocollo dell'indirizzo corrente
        if ((sd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) < 0)
        {
            // Se la creazione del socket fallisce, stampa un messaggio di errore
            fprintf(stderr, "creazione socket fallita\n");
            // Passa al prossimo indirizzo della lista (fallback)
            continue;
        }

        // Tenta la connessione al server usando l'indirizzo corrente
        if (connect(sd, ptr->ai_addr, ptr->ai_addrlen) == 0)
        {
            // Se la connessione riesce, stampa il numero del tentativo riuscito
            printf("connect riuscita al tentativo %d\n", i);
            // Esce dal ciclo: non serve più tentare altri indirizzi
            break;
        }

        // Incrementa il contatore dei tentativi falliti
        i++;
        // Chiude il socket perché la connessione non ha funzionato
        close(sd);
    }

    /* Verifica sul risultato restituito da getaddrinfo */
    if (ptr == NULL)
    {
        fprintf(stderr, "Errore risoluzione nome: nessun indirizzo corrispondente trovato\n");
        exit(EXIT_FAILURE);
    }

    /* a questo punto, posso liberare la memoria allocata da getaddrinfo */
    freeaddrinfo(res);

    /* Inizializzo buffer di ricezione*/
    rxb_init(&rxb, MAX_REQUEST_SIZE);

    for (;;)
    {
        char nome[4096];
        char annata[4096];

        // lettura nome dall'utente
        printf("Inserisci nome del vino:\n");
        if (fgets(nome, sizeof(nome), stdin) == NULL)
        {
            perror("fgets");
            exit(EXIT_FAILURE);
        }

        if (strcmp(nome, "fine\n") == 0)
        {
            break;
        }

        // invio richiesta al server
        if (write_all(sd, nome, strlen(nome)) < 0)
        {
            perror("while1");
            exit(EXIT_FAILURE);
        }

        // lettura annata dall'utente
        printf("Inserisci l'annata:\n");
        if (fgets(annata, sizeof(annata), stdin) == NULL)
        {
            perror("fgets");
            exit(EXIT_FAILURE);
        }

        if (strcmp(annata, "fine\n") == 0)
        {
            break;
        }

        // invio richiesta al server
        if (write_all(sd, annata, strlen(nome)) < 0)
        {
            perror("while1");
            exit(EXIT_FAILURE);
        }

        /* Leggo la risposta del server e la stampo a video */
        for (;;)
        {
            char response[MAX_REQUEST_SIZE];
            size_t response_len;

            /* Inizializzo il buffer response a zero e non uso l'ultimo
             * byte, così sono sicuro che il contenuto del buffer sarà
             * sempre null-terminated. In questo modo, posso interpretarlo
             * come una stringa C. Questa è un'operazione che va svolta
             * prima di leggere ogni nuova risposta. */
            memset(response, 0, sizeof(response));
            response_len = sizeof(response) - 1;

            /* Ricezione risultato */
            if (rxb_readline(&rxb, sd, response, &response_len) < 0)
            {
                /* Se sono qui, è perché ho letto un EOF. Significa che
                 * il Server ha chiuso la connessione, per cui dealloco
                 * rxb (opzionale) ed esco. */
                rxb_destroy(&rxb);
                fprintf(stderr, "Connesione chiusa dal server\n");
                exit(EXIT_FAILURE);
            }

/* Verifico che il testo sia UTF-8 valido */
#ifdef USE_LIBUNISTRING
            if (u8_check((uint8_t *)response, response_len) != NULL)
            {
                /* Server che malfunziona - inviata riga risposta
                 * con stringa UTF-8 non valida */
                fprintf(stderr, "Request is not valid UTF-8");
                close(sd);
                exit(EXIT_FAILURE);
            }
#endif

            // stampo riga letta dal server
            puts(response);

            /* Passo a nuova richiesta una volta terminato input Server */
            if (strcmp(response, "--- END REQUEST ---") == 0)
            {
                break;
            }
        }
    }

    /* chiudo socket */
    close(sd);
    return 0;
}