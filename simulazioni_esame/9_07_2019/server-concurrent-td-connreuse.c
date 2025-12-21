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

void handler(int signo)
{
    int status;
    (void)signo;
    while (waitpid(-1, &status, WNOHANG)>0)
    {
        continue;
    }
}

int autorizza(const char *username, const char *password)
{

    return 1;
}
int main(int argc, char **argv)
{

    struct addrinfo hints, *res;
    int err, sd, ns, on, pid;
    struct sigaction sa;

    if (argc != 2)
    {
        fprintf(stderr, "Uso: %s porta \n", argv[0]);
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
        fprintf(stderr, "Errore setup indirizzo bind: %s \n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }

    if ((sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol) )< 0)
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
            // FIGLIO
            int status, pid1, pid2, p1p2[2], p2p3[2], pid3;
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
                char username[1024];
                size_t username_len;
                char password[1024];
                size_t password_len;
                char categoria[1024];
                size_t categoria_len;

                // lettura username
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
                    fprintf(stderr, "Request is not valide UTF-8");
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }
#endif

                // lettura password
                memset(password, 0, sizeof(password));
                password_len = sizeof(password) - 1;

                if (rxb_readline(&rxb, ns, password, &password_len) < 0)
                {
                    rxb_destroy(&rxb);
                    close(ns);
                    perror("readline");
                    exit(EXIT_FAILURE);
                }

#ifdef USE_LIBUNISTRING
                if (u8_check((uint8_t *)password, password_len) != NULL)
                {
                    fprintf(stderr, "Request is not valide UTF-8");
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }
#endif

                // lettura password
                memset(categoria, 0, sizeof(categoria));
                categoria_len = sizeof(categoria) - 1;

                if (rxb_readline(&rxb, ns, categoria, &categoria_len) < 0)
                {
                    rxb_destroy(&rxb);
                    close(ns);
                    perror("readline");
                    exit(EXIT_FAILURE);
                }

#ifdef USE_LIBUNISTRING
                if (u8_check((uint8_t *)categoria, categoria_len) != NULL)
                {
                    fprintf(stderr, "Request is not valide UTF-8");
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }
#endif

                // verifica autorizzazione
                if (autorizza(username, password) != 1)
                {
                    char *unauthorized = "\n Non autorizzato";
                    write_all(ns, unauthorized, strlen(unauthorized));
                    write_all(ns, end_request, strlen(end_request));
                    continue;
                }
                char nomefile[4096];
                snprintf(nomefile, sizeof(nomefile), "./macchine_caffè/%s.txt", categoria);

                if (pipe(p1p2) < 0)
                {
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }

                if ((pid1 =fork()) < 0)
                {
                    perror("fork1");
                    exit(EXIT_FAILURE);
                }
                else if (pid1 == 0)
                {
                    // NIPOTE 1

                    close(ns);
                    close(p1p2[0]);

                    // ridirezione 1
                    close(1);
                    if (dup(p1p2[1]) < 0)
                    {
                        perror("dup1");
                        exit(EXIT_FAILURE);
                    }
                    close(p1p2[1]);

                    execlp("sort", "sort", "-n", "-r",nomefile, (char *)NULL);
                    exit(EXIT_FAILURE);
                }
                if (pipe(p2p3) < 0)
                {
                    perror("pipe2");
                    exit(EXIT_FAILURE);
                }

                if ((pid2 = fork()) < 0)
                {
                    perror("fork2");
                    exit(EXIT_FAILURE);
                }
                else if (pid2 == 0)
                {
                    close(p2p3[0]);
                    close(p1p2[1]);
                    close(ns);
                    close(0);
                    
                    if (dup(p1p2[0]) < 0)
                    {
                        perror("dup 2");
                        exit(EXIT_FAILURE);
                    }
                    close(p1p2[0]);

                    close(1);
                    if (dup(p2p3[1]) < 0)
                    {
                        perror("ns");
                        exit(EXIT_FAILURE);
                    }
                    close(p2p3[1]);

                    execlp("cut", "cut", "-d", ",", "-f", "1 3 4");
                    perror("cut");
                    exit(EXIT_FAILURE);
                }

                close(p1p2[0]);
                close(p1p2[1]);

                if ((pid3 = fork()) < 0)
                {
                    perror("fork2");
                    exit(EXIT_FAILURE);
                }
                else if (pid3 == 0)
                {
                    close(p2p3[1]);

                    // NIPOTE 3
                    close(0);
                    if (dup(p2p3[0]) < 0)
                    {
                        perror("dup 3");
                        exit(EXIT_FAILURE);
                    }
                    close(p2p3[0]);

                    close(1);
                    if (dup(ns) < 0)
                    {
                        perror("ns");
                        exit(EXIT_FAILURE);
                    }

                    close(ns);

                    execlp("head", "head", "-n", "10", (char *)NULL);
                    exit(EXIT_FAILURE);
                }
                close(p2p3[1]);
                close(p2p3[0]);

                waitpid(pid1, &status, 0);
                waitpid(pid2, &status, 0);
                waitpid(pid3, &status, 0);

                if (write_all(ns, end_request, strlen(end_request)) < 0)
                {
                    perror("write");
                    exit(EXIT_FAILURE);
                }
            }
            rxb_destroy(&rxb);      //nel server va messo qua perchè dichiaro rxy nel figlio e non le padre iniziale
            close(ns);
            exit(EXIT_SUCCESS);
        }
        close(ns);
    }

    close(sd);
    return 0;
}
