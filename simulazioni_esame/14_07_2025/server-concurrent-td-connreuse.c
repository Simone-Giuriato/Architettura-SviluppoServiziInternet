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
        fprintf(stderr, "Uso: %s porta", argv[0]);
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
        fprintf(stderr, "Errore: setup indirizzo bind: %s\n", gai_strerror(err));
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

    // atendo client
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
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            // figlio
            int status, pid1, pid2, p1p2[2];
            const char *end_request = "\n--- END REQUEST ---\n";
            rxb_t rxb;
            close(sd);

            memset(&sa, 0, sizeof(sa));
            sigemptyset(&sa.sa_mask);
            sa.sa_handler = SIG_DFL;
            if (sigaction(SIGCHLD, &sa, NULL) == -1)
            {
                perror("sigaction");
                exit(EXIT_FAILURE);
            }
            rxb_init(&rxb, MAX_REQUEST_SIZE);

            for (;;)
            {

                char nazione[1024];
                char tipologia[1024];
                char budget[1024];
                size_t nazione_len;
                size_t tipologia_len;
                size_t budget_len;

                memset(nazione, 0, sizeof(nazione));
                nazione_len = sizeof(nazione) - 1;

                if (rxb_readline(&rxb, ns, nazione, &nazione_len) < 0)
                {
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }

#ifdef USE_LIBUNISTRING
                if (u8_check((uint8_t *)nazione, nazione_len) != NULL)
                {
                    fprintf(stderr, "Request is not valid UTF8\n");
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }
#endif

                memset(tipologia, 0, sizeof(tipologia));
                tipologia_len = sizeof(tipologia) - 1;

                if (rxb_readline(&rxb, ns, tipologia, &tipologia_len) < 0)
                {
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }

#ifdef USE_LIBUNISTRING
                if (u8_check((uint8_t *)tipologia, tipologia_len) != NULL)
                {
                    fprintf(stderr, "Request is not valid UTF8\n");
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }
#endif

                memset(budget, 0, sizeof(budget));
                budget_len = sizeof(budget) - 1;

                if (rxb_readline(&rxb, ns, budget, &budget_len) < 0)
                {
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }

#ifdef USE_LIBUNISTRING
                if (u8_check((uint8_t *)budget, budget_len) != NULL)
                {
                    fprintf(stderr, "Request is not valid UTF8\n");
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }
#endif

                // uso  il  percorso  ./holiday_packages  al  posto  di  /var/local/holiday_packages
                char nomefile[4096];
                snprintf(nomefile, sizeof(nomefile), "./holiday_packages/%s/%s.txt", nazione, tipologia);

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
                    // nipote1
                    close(ns);
                    close(p1p2[0]);
                    // ridirezione stdout in standard della p1p2
                    close(1);

                    if (dup(p1p2[1]) < 0)
                    {
                        perror("dup");
                        exit(EXIT_FAILURE);
                    }
                    close(p1p2[1]);

                    execlp("grep", "grep", budget, nomefile, (char *)NULL); // trovo info sul mio budget
                    perror("execlp grep");
                    exit(EXIT_FAILURE);
                }

                if ((pid2 = fork()) < 0)
                {
                    perror("fork2");
                    exit(EXIT_FAILURE);
                }
                else if (pid2 == 0)
                {
                    // nipote 2
                    close(p1p2[1]);

                    // ridirizono stdin, faccio leggere sulla pipe
                    close(0);
                    if (dup(p1p2[0]) < 0)
                    {
                        perror("dup");
                        exit(EXIT_FAILURE);
                    }
                    close(p1p2[0]);

                    // ridirezione stdout faccio scrivere sulla socket
                    close(1);
                    if (dup(ns) < 0)
                    {
                        perror("dup ns");
                        exit(EXIT_FAILURE);
                    }
                    close(ns); // qui chiudo ns non prima come in nipote 1

                    execlp("sort", "sort", "-n", "-r", (char *)NULL);
                    perror("sort");
                    exit(EXIT_FAILURE);
                }

                close(p1p2[1]);
                close(p1p2[0]);

                waitpid(pid1, &status, 0);
                waitpid(pid2, &status, 0);

                if (write_all(ns, end_request, strlen(end_request)) < 0)
                {
                    perror("wirte");
                    exit(EXIT_FAILURE);
                }
            }
            close(ns);
            exit(EXIT_SUCCESS);
        }
        close(ns);
    }
    close(sd);
    return 0;
}