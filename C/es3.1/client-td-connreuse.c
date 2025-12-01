//PER QUALCHE MOTIVO IL CLIENT UNA VOLTA RICEVUTO LE INFO DAL SERVER SI IMPALLA E NON PASSA ALLA PROSSI A RICHIESTA, PERÒ TUTTO SOMMATO VA MA CON QUESTO PROBLEMS

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
// #include "average.h"
#include "utils.h"
#include "rxb.h"

#define MAX_REQUEST_SIZE (64 * 1024)
#define DIM 4096

int main(int argc, char **argv)
{
    int err, sd, i = 1;
    struct addrinfo hints;
    struct addrinfo *res, *ptr;

    char *host_remoto;
    char *servizio_remoto;
    rxb_t rxb;

    // controllo argomenti
    if (argc < 3)
    {
        printf("Uso: rps hostname porta <option>");
        exit(EXIT_FAILURE);
    }
    // ingoro SIGPIPE
    signal(SIGPIPE, SIG_IGN);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    host_remoto = argv[1];
    servizio_remoto = argv[2];
    if ((err = getaddrinfo(host_remoto, servizio_remoto, &hints, &res)) != 0)
    {
        fprintf(stderr, "Errore risoluzione nome %s\n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }

    // connesione con fallback
    for (ptr = res; ptr != NULL; ptr = ptr->ai_next)
    {
        if ((sd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) < 0)
        {
            fprintf(stderr, "creazione socket fllita\n");
            exit(EXIT_FAILURE);
        }

        if (connect(sd, ptr->ai_addr, ptr->ai_addrlen) == 0)
        {
            printf("connect riuscita al tentatiivo %d\n", i);
            break;
        }
        i++;
        close(sd);
    }

    if (ptr == NULL)
    {
        fprintf(stderr, "Errore risoluzione nome: nessun indirizzo corrispondente trovato\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);

    rxb_init(&rxb, MAX_REQUEST_SIZE);

    for (;;)
    {
        char option[4096];

        // lettura dall'utente
        printf("Inserisic l'opzione da associare a ps (fine per terminare):");
        if (fgets(option, sizeof(option), stdin) == NULL)
        {
            perror("fgets");
            exit(EXIT_FAILURE);
        }

        // esco se mette fine
        if (strcmp(option, "fine") == 0)
        {
            break;
        }

        // invio al server
        if (write_all(sd, option, strlen(option)) < 0)
        {
            perror("write");
            exit(EXIT_FAILURE);
        }

        // leggo le risposte del server e sampo a video
        for (;;)
        {
            char response[MAX_REQUEST_SIZE];
            size_t response_len;

            memset(response, 0, sizeof(response));
            response_len = sizeof(response) - 1;

            // ricezione risulatato
            if (rxb_readline(&rxb, sd, response, &response_len) < 0)
            {
                rxb_destroy(&rxb);
                exit(EXIT_FAILURE);
            }

#ifdef USE_LIBUNISTRING
            // verifico che il messaggio sia utf-8 valido
            if (u8_check((unit8_t *)response, response_len) != NULL)
            {
                fprintf(stderr, "Request is not valid UTF-8!\n");
                close(sd);
                exit(EXIT_FAILURE);
            }
#endif

            // stampo riga letta dal server:
            puts(response);

            // passo a nuova richiesta una olta terminato input server
            if (strcmp(response, "--- END REQUEST ---") == 0)
            {
                break;
            }
        }
    }

    // chiudo la socket
    close(sd);

    return 0;
}