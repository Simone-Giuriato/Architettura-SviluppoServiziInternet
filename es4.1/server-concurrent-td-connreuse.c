
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

#include "utils.h"
#include "rxb.h"
#define MAX_REQUEST_SIZE (64 * 1024)

// gestore segnale SIGCHLD
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

    // controllo argomenti
    if (argc != 2)
    {
        fprintf(stderr, "Uso: %s porta\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // IGNORO sigpipe
    signal(SIGPIPE, SIG_IGN);

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

    if ((err = getaddrinfo(NULL, argv[0], &hints, &res)) != 0)
    {
        fprintf(stderr, "Errore setup indirizzo bind%s\n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }

    if ((sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
    {
        perror("Errore socket");
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
        perror("bind");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);

    if (listen(sd, SOMAXCONN) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Attendo i client
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
            const char *end_request = "\n--- END REQUEST ---\n";
            rxb_t rxb;

            close(sd);
            int pid_n1, pid_n2, pipe_n1n2[2], status;

            // disabilito gestore SIGCHLD
            memset(&sa, 0, sizeof(sa));
            sigemptyset(&sa.sa_mask);

            sa.sa_handler = SIG_DFL;

            if (sigaction(SIGCHLD, &sa, NULL) == -1)
            {

                perror("sigaction");
                exit(EXIT_FAILURE);
            }
            // inizializzo buffer di ricezione
            rxb_init(&rxb, MAX_REQUEST_SIZE);

            // avvio ciclo di gestione richieste
            for (;;)
            {
                char categoria[MAX_REQUEST_SIZE];
                size_t categoria_len;

                // inizializzo buffer categoria a 0
                memset(categoria, 0, sizeof(categoria));
                categoria_len = sizeof(categoria) - 1;

                // leggo richiesta da client
                if (rxb_readline(&rxb, ns, categoria, &categoria_len) < 0)
                {
                    // se son qui ho trovato un EOF
                    rxb_destroy(&rxb);
                    break;
                }

#ifdef UNE_LIBUNISTRING
                if (u8_check((unit8_t *)categoria, categoria_len) |= NULL)
                {
                    // inviato messaggio con stringa utf8 non valida
                    fprintf(stderr, "Request is not valid UTF-8\n");
                    close(ns);
                    exit(EXIT_FAILURE);
                }
#endif

                // creo pipe
                if (pipe(pipe_n1n2) < 0)
                {
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }

                if ((pid_n1 = fork()) < 0)
                {
                    perror("secondo fork");
                    exit(EXIT_FAILURE);
                }
                else if (pid_n1 == 0)
                {
                    // nipote 1

                    close(ns);
                    close(pipe_n1n2[0]);

                    close(1);
                    if (dup(pipe_n1n2[1]) < 0)
                    {
                        perror("dup");
                        exit(EXIT_FAILURE);
                    }
                    close(pipe_n1n2[1]);
                    execlp("grep", "grep", categoria, "conto_corrente.txt", (char *)NULL);
                    perror("execlp grep 1");
                    exit(EXIT_FAILURE);
                }

                if ((pid_n2 = fork()) < 0)
                {
                    perror("secondo fork");
                    exit(EXIT_FAILURE);
                }
                else if (pid_n2 == 0)
                {
                    // nipote 2
                    close(pipe_n1n2[1]);
                    close(0);

                    if (dup(pipe_n1n2[0]) < 0)
                    {
                        perror("dup");
                        exit(EXIT_FAILURE);
                    }
                    close(pipe_n1n2[0]);

                    // ridirezione stdout
                    if (dup(ns) < 0)
                    {
                        perror("dup");
                        exit(EXIT_FAILURE);
                    }
                    close(ns);

                    execlp("sort", "sort", "-r", (char *)NULL);
                    exit(EXIT_FAILURE);
                }

                // FIGLIO
                close(pipe_n1n2[0]);
                close(pipe_n1n2[1]);

                waitpid(pid_n1, &status, 0);
                waitpid(pid_n2, &status, 0);

                // mando stringa fine richiesta
                if (write_all(ns, end_request, strlen(end_request)) < 0)
                {
                    perror("while");
                    exit(EXIT_FAILURE);
                }
            }
            //figlio fuori for gestione richieste
            // chiudo socket attiva
            close(ns);
            exit(EXIT_SUCCESS); // termino figlio
        }
        // PADRE
        // chiudo socket attiva
        close(ns);
    }
    close(sd);
    return 0;
}


//FIGLIO GESTISCE CONNESIONE COI CLIENT, E I 2 NIPOTI FANNO GREP E SORT