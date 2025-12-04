#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#ifdef USE_LIBUNISTRING
#include <unistr.h>
#endif
#include "rxb.h"
#include "utils.h"
#include "average.h"

#define MAX_REQUEST_SIZE (64 * 1024)

void handler(int signo)
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

    signal(SIGPIPE, SIG_IGN);

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
        fprintf(stderr, "Errore setup indirizzo bind: %s\n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }

    if ((sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
    {
        perror("socket");
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

    for (;;)
    {
        printf("Server in ascolto...\n");
        if ((ns = accept(sd, NULL, NULL)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        if ((pid = fork()) < 0)
        {
            perror("prima fork");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            int status, pid1, pid2, p1p2[2], p2pf[2];
            const char *end_request = "\n--- END REQUEST ---\n";
            rxb_t rxb;

            // chiusura socket processo padre
            close(sd);

            // configurazione del gestore SIGCHLD
            memset(&sa, 0, sizeof(sa));
            sigemptyset(&sa.sa_mask);
            sa.sa_handler = SIG_DFL;
            if (sigaction(SIGCHLD, &sa, NULL) == -1)
            {
                perror("sigaction");
                exit(EXIT_FAILURE);
            }

            rxb_init(&rxb, MAX_REQUEST_SIZE * 2);

            for (;;)
            {
                char regione[1024];
                char n_località[1024];

                size_t regione_len;
                size_t n_località_len;

                // lettura regione passata dal client
                memset(regione, 0, sizeof(regione));
                regione_len = sizeof(regione) - 1;

                if (rxb_readline(&rxb, ns, regione, &regione_len) < 0)
                {
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }

#ifdef USE_LIBUNISTRING
                if (u8_check((uint8_t *)regione, regione_len) != NULL)
                {
                    fprintf(stderr, "Request is not valid UTF-8\n");
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }
#endif

                // lettura numero località passata dal client
                memset(n_località, 0, sizeof(n_località));
                n_località_len = sizeof(n_località) - 1;

                if (rxb_readline(&rxb, ns, n_località, &n_località_len) < 0)
                {
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }

#ifdef USE_LIBUNISTRING
                if (u8_check((uint8_t *)n_località, n_località_len) != NULL)
                {
                    fprintf(stderr, "Request is not valid UTF-8\n");
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }
#endif

                // imposto percorso file
                char nomefile[4096];
                snprintf(nomefile, sizeof(nomefile), "./%s.txt", regione);
                if (pipe(p1p2) < 0)
                {
                    perror("p1p2");
                    exit(EXIT_FAILURE);
                }

                if (pipe(p2pf) < 0) // server per il punto opzionale... farò comunicare nipote 2 col figlio usando questa pipe e non la socket
                {
                    perror("p2pf");
                    exit(EXIT_FAILURE);
                }

                if ((pid1 = fork()) < 0)
                {
                    perror("fork pid1");
                    exit(EXIT_FAILURE);
                }
                else if (pid1 == 0)
                {
                    // nipote1

                    close(ns);
                    close(p1p2[0]);

                    close(p2pf[0]);
                    close(p2pf[1]);

                    // ridireziono stdout
                    close(1);
                    if (dup(p1p2[1]) < 0)
                    {
                        perror("dup1");
                        exit(EXIT_FAILURE);
                    }

                    close(p1p2[1]);

                    execlp("sort", "sort", "-n", "-r", nomefile, (char *)NULL); // n dicende di interpretare come numeri (senza interpreta come lettere-->ascii)
                    perror("sort");
                    exit(EXIT_FAILURE);
                }

                if ((pid2 = fork()) < 0)
                {
                    perror("fork pid2");
                    exit(EXIT_FAILURE);
                }
                else if (pid2 == 0)
                {
                    // nipote 2
                    close(ns); // chiudo qua e non dopo perchè non faccio la dup con ns ma con pipe e poi si arrangerà la funzione output_with_average a far tutto
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
                    if (dup(p2pf[1]) < 0) // non metto più l output nella socket ma sulla pipe che collega nipote 2 e figlio per applicare output_with average
                    {
                        perror("dup ns");
                        exit(EXIT_FAILURE);
                    }
                    close(p2pf[1]);

                    // close(ns);
                    execlp("head", "head", "-n", n_località, (char *)NULL);
                    perror("head");
                    exit(EXIT_FAILURE);
                }

                // figlio
                close(p1p2[0]);
                close(p1p2[1]);
                close(p2pf[1]);

                output_with_average(p2pf[0], ns); // fa tutto questo metodo fornito dal prof...
                // prende in input un descrittore dove leggere e noi gli passiamo le righe selezionate, si estrae i dati sulla neve e fa una media
                // poi manda sul decsrittore di output(la socket ns) sia le righe e sia la media (vedere average.c per il funzionamento)

                close(p2pf[0]);

                close(p2pf[0]);
                waitpid(pid1, &status, 0);
                waitpid(pid2, &status, 0);

                if (write_all(ns, end_request, strlen(end_request)) < 0)
                {
                    perror("write");
                    exit(EXIT_FAILURE);
                }
            }

            close(ns);
            exit(EXIT_SUCCESS);
        }
        // padre
        close(ns);
    }
    close(sd);
    return 0;
}