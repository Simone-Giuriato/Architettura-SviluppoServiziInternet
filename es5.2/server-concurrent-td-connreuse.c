//non è stato fatto il punto opzionale, intuisco una grep con una wc -l ?

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
#include "utils.h"
#include "rxb.h"

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

int autorizza(const char *username, const char *password)
{ // funzione data dalla traccia
    return 1;
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
        fprintf(stderr, "Errore setup indirizzo di bind %s\n", gai_strerror(err));
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

    // attendo client
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
            perror("fork1");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            // FIGLIO

            int status, pid1, pid2, p1p2[2];
            const char *end_request = "\n--- END REQUEST ---\n";
            rxb_t rxb;

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
            rxb_init(&rxb, MAX_REQUEST_SIZE);

            for (;;)
            {
                char username[1024];
                size_t username_len;
                char password[1024];
                size_t password_len;
                

                memset(username, 0, sizeof(username));
                username_len = sizeof(username) - 1;

                if (rxb_readline(&rxb, ns, username, &username_len) < 0)
                {
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }

#ifdef USE_LIBUNISTRING
                if (u8_check((uint8_t *)username, username_len) != NULL)
                {
                    fprintf(stderr, "Request is not valid UTF_8");
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }
#endif

                memset(password, 0, sizeof(password));
                password_len = sizeof(password) - 1;

                if (rxb_readline(&rxb, ns, password, &password_len) < 0)
                {
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }

#ifdef USE_LIBUNISTRING
                if (u8_check((uint8_t *)password, password_len) != NULL)
                {
                    fprintf(stderr, "Request is not valid UTF_8");
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }
#endif

                

                // verifica autorizzazione
                if (autorizza(username, password) != 1)
                {
                    char *unauthorized = "Non autorizzato\n";
                    write_all(ns, unauthorized, strlen(unauthorized));
                    write_all(ns, end_request, strlen(end_request));
                    continue;
                }

                if (pipe(p1p2) < 0)
                {
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }

                if ((pid1 = fork()) < 0)
                {
                    perror("fork pid1");
                    exit(EXIT_FAILURE);
                }
                else if (pid1 == 0)
                {
                    // nipote 1

                    close(ns);
                    close(p1p2[0]);
                    close(1);
                    if (dup(p1p2[1]) < 0)
                    {
                        perror("dup1");
                        exit(EXIT_FAILURE);
                    }

                    close(p1p2[1]);
                    execlp("grep", "grep", username, "test.txt", (char *)NULL);
                    perror("grep");
                    exit(EXIT_FAILURE);
                }

                if ((pid2 = fork()) < 0)
                {

                    perror("fork pidd2");
                    exit(EXIT_FAILURE);
                }
                else if (pid2 == 0)
                {
                    // nipote 2

                    close(p1p2[1]);

                    close(0);
                    if(dup(p1p2[0] )< 0)
                    {
                        perror("dup p1p2[0]");
                        exit(EXIT_FAILURE);
                    }

                    close(p1p2[0]);

                    close(1);
                    if (dup(ns) < 0)
                    {
                        perror(" dup ns");
                        exit(EXIT_FAILURE);
                    }
                    close(ns);

                    execlp("sort", "sort", "-r", "-n", (char *)NULL);
                    perror("sort");
                    exit(EXIT_FAILURE);
                }

                // figlio
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
            close(ns);
            exit(EXIT_SUCCESS);
        }
        close(ns);
    }
    close(sd);

    return 0;
}