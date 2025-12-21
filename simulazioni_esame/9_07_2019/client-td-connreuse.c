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
#include <signal.h>
#include <unistd.h>
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
    servizio_remoto = argv[2];

    if ((err = getaddrinfo(host_remoto, servizio_remoto, &hints, &res)) != 0)
    {
        fprintf(stderr, "Errore in risoluzione ome %s\n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }

    for (ptr = res; ptr != NULL; ptr = ptr->ai_next)
    {
        if ((sd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) < 0)
        {
            fprintf(stderr, "creazione socket fallita\n");
            continue;
        }

        if (connect(sd, ptr->ai_addr, ptr->ai_addrlen) == 0)
        {
            printf("connect riusicta al tentaivo %d \n", i);
            break;
        }
        i++;
        close(sd);
    }

    if (ptr == NULL)
    {
        fprintf(stderr, "Errore in risoluzione nome:nessun indirizzo corrisponde trovato\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);

    rxb_init(&rxb, MAX_REQUEST_SIZE * 2);

    for (;;)
    {
        char username[1024];
        char password[1024];
        char categoria[1024];

        puts("Inserisci username (fine per uscire):");
        if (fgets(username, sizeof(username), stdin) == NULL)
        {
            perror("fgets");
            exit(EXIT_FAILURE);
        }

        if (strcmp(username, "fine\n") == 0)
        {
            break;
        }

        puts("Inserisci password (fine per uscire):");
        if (fgets(password, sizeof(password), stdin) == NULL)
        {
            perror("fgets");
            exit(EXIT_FAILURE);
        }

        if (strcmp(password, "fine\n") == 0)
        {
            break;
        }
        puts("Inserisci categoria (fine per uscire):");
        if (fgets(categoria, sizeof(categoria), stdin) == NULL)
        {
            perror("fgets");
            exit(EXIT_FAILURE);
        }

        if (strcmp(categoria, "fine\n") == 0)
        {
            break;
        }

        if (write_all(sd, username, strlen(username)) < 0)
        {
            perror("write");
            exit(EXIT_FAILURE);
        }
        if (write_all(sd, password, strlen(password)) < 0)
        {
            perror("write");
            exit(EXIT_FAILURE);
        }
        if (write_all(sd, categoria, strlen(categoria)) < 0)
        {
            perror("write");
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
                close(sd);
                exit(EXIT_FAILURE);
            }

#ifdef USE_LIBUNISTRING
            if (u8_check((uint8_t *)response, response_len) != NULL)
            {
                fprintf(stderr, "Request is not valide UTF-8");
                rxb_destroy(&rxb);
                close(sd);
                exit(EXIT_FAILURE);
            }
#endif

            puts(response);
            if(strcmp(response,"--- END REQUEST ---")==0){
                break;
            }
        }
    }
    rxb_destroy(&rxb);  //nel client va messo qua
    close(sd);
    return 0;
}