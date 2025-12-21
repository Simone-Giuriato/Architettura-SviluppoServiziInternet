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
    char *servizio_remoto;

    int sd, i = 1;
    rxb_t rxb;

    if (argc < 3)
    {
        fprintf(stderr, "Uso: %s server porta\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    signal(SIGPIPE, SIG_IGN);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    host_remoto = argv[1];
    servizio_remoto = argv[2];

    if ((err = getaddrinfo(host_remoto, servizio_remoto, &hints, &res)) != 0)   //ottengo risoluzione nome del server
    {
        fprintf(stderr, "Errore risoluzione nome: %s\n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }

    // connessione con FALLBACK

    for (ptr = res; ptr != NULL; ptr = ptr->ai_next)
    {
        if ((sd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) < 0)
        {
            fprintf(stderr, "Creazione socket fallita\n");
            exit(EXIT_FAILURE);
        }

        if (connect(sd, ptr->ai_addr, ptr->ai_addrlen) == 0)
        {
            printf("connect riuscita al tenativo %d\n", i);
            break;
        }
        i++;
        close(sd);
    }

    if (ptr == NULL)    //controllo se ptr==NULL, nel caso lo fosse nessun indirizzo corrispondente è stato trovato per il servr
    {
        fprintf(stderr, "Errore risolzuione nome: nessun indirizzo corrispondente trovato \n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);  //libero memoria

    rxb_init(&rxb, MAX_REQUEST_SIZE * 2);   //inizializzo struttura per leggere dati dalla socket

    for (;;)    //ciclo infinito, ogni scorrimento rappresneta una richiesta
    {
        char categoria[1024];
        char produttore[1024];
        char ordinamento[1024];

        puts("Inserisci la categoira (fine per uscire):");
        if (fgets(categoria, sizeof(categoria), stdin) == NULL)
        {
            perror("fgets");
            exit(EXIT_FAILURE);
        }

        if (strcmp(categoria, "fine\n") == 0)
        {
            break;
        }
        puts("Inserisci il produttore (fine per uscire):");
        if (fgets(produttore, sizeof(produttore), stdin) == NULL)
        {
            perror("fgets");
            exit(EXIT_FAILURE);
        }

        if (strcmp(produttore, "fine\n") == 0)
        {
            break;
        }

        puts("Inserisci l'ordinamneto [crescente o decrescente] (fine per uscire):");
        if (fgets(ordinamento, sizeof(ordinamento), stdin) == NULL)
        {
            perror("fgets");
            exit(EXIT_FAILURE);
        }

        if (strcmp(ordinamento, "fine\n") == 0)
        {
            break;
        }

        //invio i dati al server, scrivendoli completamente sulla socket 

        if (write_all(sd, categoria, strlen(categoria)) < 0)
        {
            perror("write");
            exit(EXIT_FAILURE);
        }
        if (write_all(sd, produttore, strlen(produttore)) < 0)
        {
            perror("write");
            exit(EXIT_FAILURE);
        }
        if (write_all(sd, ordinamento, strlen(ordinamento)) < 0)
        {
            perror("write");
            exit(EXIT_FAILURE);
        }


        

        char response[MAX_REQUEST_SIZE];    //array che conterrà le risposte del server scritte nella socket
        size_t response_len;
        for (;;)    //ciclo infinito per leggere le risposte dal server finchè non riceverò segnale di fine richiesta (--- END REQUEST ---)
        {

            memset(response, 0, sizeof(response));
            response_len = sizeof(response) - 1;

            if (rxb_readline(&rxb, sd, response, &response_len) < 0)    //leggo riga di testo dal socket e memorizzo in array response 
            {
                rxb_destroy(&rxb);
                fprintf(stderr, "Connessione chiusa dal server\n");
                close(sd);
                exit(EXIT_FAILURE);
            }


            //validazione UTF-8
#ifdef USE_LIBUNISTRING 
            if (u8_check((uint8_t *)response, response_len) != NNULL)
            {
                fprintf(stderr, "Request is not valid UTF-8\n");
                rxb_destroy(&rxb);
                close(sd);
                exit(EXIT_FAILURE);
            }
#endif

            puts(response); //stampo a console

            if (strcmp(response, "--- END REQUEST ---") == 0)
            {
                break;
            }
        }
    }
    rxb_destroy(&rxb);
    close(sd);
    return 0;
}