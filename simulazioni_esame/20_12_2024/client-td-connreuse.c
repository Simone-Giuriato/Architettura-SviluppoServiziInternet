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

int main(int argc, char **argv)
{

    int err;
    struct addrinfo hints, *res, *ptr;
    char *host_remoto;
    char *servzio_remoto;
    int sd, i = 1;
    rxb_t rxb;

    if (argc < 3)
    {
        fprintf(stderr, "Uso: %s hostname porta", argv[0]);
        exit(EXIT_FAILURE);
    }

    signal(SIGPIPE, SIG_IGN);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    host_remoto = argv[1];
    servzio_remoto = argv[2];

    if ((err = getaddrinfo(host_remoto, servzio_remoto, &hints, &res)) != 0)
    {
        fprintf(stderr, "Errore risoluzione nome %s\n:", gai_strerror(err));
        exit(EXIT_FAILURE);
    }

    // CONNESIONE CON FALLBACK
    for (ptr = res; ptr != NULL; ptr = ptr->ai_next)
    {
        if ((sd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) < 0)
        {
            fprintf(stderr, "creazione socket fallita");
        }

        if (connect(sd, ptr->ai_addr, ptr->ai_addrlen) == 0)
        {
            printf("connect riuscita al %d tentativo\n", i);
            break;
        }
        i++;
        close(sd);
    }

    if (ptr == NULL)
    {
        fprintf(stderr, "Errore risluzione nome: nessun indirizzo corrispondente");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);

    rxb_init(&rxb, MAX_REQUEST_SIZE * 2);

    for (;;)
    {
        char nome[1024], data[1024], numero[1024];

        puts("Inserisci nome cliente (fine per uscire)");
        if (fgets(nome, sizeof(nome), stdin) == NULL)
        {
            perror("fgets");
            exit(EXIT_FAILURE);
        }
        if (strcmp(nome, "fine\n") == 0)
        {
            break;
        }

        puts("Inserisci data (fine per uscire)");
        if (fgets(data, sizeof(data), stdin) == NULL)
        {
            perror("fgets");
            exit(EXIT_FAILURE);
        }
        if (strcmp(data, "fine\n") == 0)
        { // mettere \n perche fgets la importa
            break;
        }

        puts("Inserisci numero da visualizzare (fine per uscire)");
        if (fgets(numero, sizeof(numero), stdin) == NULL)
        {
            perror("fgets");
            exit(EXIT_FAILURE);
        }
        if (strcmp(numero, "fine\n") == 0)
        {
            break;
        }

        if (write_all(sd, nome, strlen(nome)) < 0)
        {
            perror("write nome");
            exit(EXIT_FAILURE);
        }
        if (write_all(sd, data, strlen(data)) < 0)
        {
            perror("write data");
            exit(EXIT_FAILURE);
        }
        if (write_all(sd, numero, strlen(numero)) < 0)
        {
            perror("write numero");
            exit(EXIT_FAILURE);
        }

        for (;;)
        {
            char response[MAX_REQUEST_SIZE];
            size_t response_len;
            memset(response, 0, sizeof(response));
            response_len = sizeof(response) - 1;

            if (rxb_readline(&rxb, sd, response, &response_len) < 0)
            {
                rxb_destroy(&rxb);
                fprintf(stderr, "Connessione chiusa dal server\n");
                exit(EXIT_FAILURE);
            }

#ifdef USE_LIBUNISTRING
            if (u8_check((uint8_t *)response, response_len) != NULL)
            {
                frpitnf(stderr, "Request is not valid UTF-8");
                rxb_destroy(&rxb);
                close(sd);
                exit(EXIT_FAILURE);
            }
#endif

            puts(response);
            if (strcmp(response, "--- END REQUEST ---") == 0)
            {
                break;
            }
        }
    }
    rxb_destroy(&rxb);  //mettere
    close(sd);

    return 0;
}