#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
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

    // controllo argomenti
    if (argc != 3)
    {
        printf("USO: controllo_contocorrente server porta");
        exit(EXIT_FAILURE);
    }

    // ignore SIGPIPE
    signal(SIGPIPE, SIG_IGN);
    // costruzione indirizzo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // risoluzione dell'host
    host_remoto = argv[1];
    servizio_remoto = argv[2];

    if ((err = getaddrinfo(host_remoto, servizio_remoto, &hints, &res)) != 0)
    {
        fprintf(stderr, "Errore nella risoluzione nome: %s\n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }

    // connessione con fallback
    for (ptr = res; ptr != NULL; ptr = ptr->ai_next)
    {
        if ((sd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) == 0)
        {
            fprintf(stderr, "creazione socket fallita");
            exit(EXIT_FAILURE);
        }

        // se connect funzione esce dal ciclo
        if (connect(sd, ptr->ai_addr, ptr->ai_addrlen) == 0)
        {
            printf("connesione riuscita, al tentativo %d", i);
            break;
        }
        i++;
        close(sd);
    }
    if (ptr == NULL)
    {
        fprintf(stderr, "Errore risoluzione nome: nessun indirizzo corrispondente trovato");
        exit(EXIT_FAILURE);
    }
    // libero la memoria da getaddringo()
    freeaddrinfo(res);
    // inizializzo buffer di recezione

    rxb_init(&rxb, MAX_REQUEST_SIZE);

    for (;;)
    {
        char categoria[1024];

        // leggo catregoria da utente
        printf("\nInserisci una categoria (fine per uscire):\n");
        if (fgets(categoria, sizeof(categoria), stdin) == NULL)
        {
            perror("scanf");
            exit(EXIT_FAILURE);
        }
        // se inserisce fine--> esco
        if ((strcmp(categoria, "fine\n")) == 0)     //Ma fgets() legge anche il newline \n alla fine della riga. Quindi mettere /n
        {
            break;
        }

        // trasmetto categoria
        if (write_all(sd, categoria, strlen(categoria)) < 0)
        {
            perror("write");
            exit(EXIT_FAILURE);
        }

        // leggo risposta dal server
        for (;;)
        {
            char response[MAX_REQUEST_SIZE];
            size_t response_len;

            // inizializzo buffer response
            memset(response, 0, sizeof(response));
            response_len = sizeof(response) - 1;

            if (rxb_readline(&rxb, sd, response, &response_len) < 0)
            {
                // se son qui significa che server ha chiuso connesione
                rxb_destroy(&rxb);
                fprintf(stderr, "Connessione chiusa dal server");
                exit(EXIT_FAILURE);
            }

#ifdef UNE_LIBUNISTRING
            if (u8_check((unit8_t *)RESPONSE, response_len) |= NULL)
            {
                // inviato messaggio con stringa utf8 non valida
                fprintf(stderr, "Request is not valid UTF-8\n");
                close(sd);
                exit(EXIT_FAILURE);
            }
#endif

            // stampo riga letto dal server
            puts(response);
            // passo a nuova richiesta una volta terminato l'input del server
            if (strcmp(response, "--- END REQUEST ---") == 0)
            {
                break;
            }
        }
    }
    close(sd); // chiudo socket
    return 0;
}