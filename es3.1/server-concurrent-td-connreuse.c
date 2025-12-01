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

// gestore SIGCHILD
void handler(int signo)
{
    int status;
    (void)signo;
    // gestisco tutti i figli terminati
    while (waitpid(-1, &status, WNOHANG))
    {
        continue;
    }
}

int main(int argc, char **argv)
{
    struct addrinfo hints, *res;
    int err, sd, ns, pid, on;
    struct sigaction sa;

    // controllo argomenti
    if (argc != 2)
    {
        fprintf(stderr, "Uso: %s porta\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // ignoro SIGPIPE
    signal(SIGPIPE, SIG_IGN);

    // installo SIGCHLD

    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);

    sa.sa_flags = SA_RESTART;
    sa.sa_handler = handler;

    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((err = getaddrinfo(NULL, argv[1], &hints, &res)) != 0)
    {
        fprintf(stderr, "Error setup indirizzo bind %s\n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }

    if ((sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
    {
        perror("Errore in socket");
        exit(EXIT_FAILURE);
    }

    on = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    if (bind(sd, res->ai_addr, res->ai_addrlen) < 0)
    {
        perror("Errore in bind");
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(res);

    if (listen(sd, SOMAXCONN) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // attendo i client...

    for (;;)
    {
        printf("Server in ascolto...\n");

        if ((ns = accept(sd, NULL, NULL)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Generazione figlio
        if ((pid = fork()) < 0)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            int pid2;
            int status;
            const char *end_request = "\n--- END REQUEST ---\n";
            rxb_t rxb;

            close(sd);

            rxb_init(&rxb, MAX_REQUEST_SIZE);

            // avvio ciclo gestione rishcieste

            for (;;)
            {
                size_t option_len;
                char option[MAX_REQUEST_SIZE];

                memset(option, 0, sizeof(option));
                option_len = sizeof(option) - 1;

                // leggo richiesta da client
                if (rxb_readline(&rxb, ns, option, &option_len) < 0)
                {

                    rxb_destroy(&rxb);
                    break;
                }

#ifdef USE_LIBUNISTRING
                // verifico che il messaggio sia utf-8 valido
                if (u8_check((unit8_t *)option, option_len) != NULL)
                {
                    fprintf(stderr, "Request is not valid UTF-8!\n");
                    close(ns);
                    exit(EXIT_FAILURE);
                }
#endif

                if ((pid2 = fork()) < 0)
                {
                    perror("seconda fork");
                    exit(EXIT_FAILURE);
                }
                else if (pid2 == 0)
                {
                    close(1);
                    if (dup(ns) < 0)
                    {
                        perror("dup");
                        exit(EXIT_FAILURE);
                    }

                    close(ns);

                    if (strlen(option) == 0)
                    {
                        execlp("ps", "ps", (char *)NULL);
                        perror("excelp ps 1");
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        execlp("ps", "ps", option, (char *)NULL);
                        perror("excelp ps 2");
                        exit(EXIT_FAILURE);
                    }
                }

                // figlio
                // Attendo che invii la risposta
                wait(&status);
                
                write_all(ns, "\n", 1); // assicura che ci sia un \n finale

                if (write_all(ns, end_request, strlen(end_request)) < 0)
                {
                    perror("write");
                    exit(EXIT_FAILURE);
                }
            }
            close(ns); // chiudo socket attiva
            // termino il figlio
            exit(EXIT_SUCCESS);
        }
       

        // PADRE
        close(ns);
    }

    close(sd); // chiudo socket passiva

    return 0;
}
