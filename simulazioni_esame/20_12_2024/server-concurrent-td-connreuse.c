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
    sa.sa_handler= handler;
    sa.sa_flags=SA_RESTART;

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

    if ((sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol) )< 0)
    {
        perror("Errore in socket");
        exit(EXIT_FAILURE);
    }

    on = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
    {
        perror("setosockopt");
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
            perror("FORK");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            // FIGLIO

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

            rxb_init(&rxb, MAX_REQUEST_SIZE * 2);

            for (;;)
            {

                char nome[MAX_REQUEST_SIZE];
                size_t nome_len;
                char data[MAX_REQUEST_SIZE];
                size_t data_len;
                char numero[MAX_REQUEST_SIZE];
                size_t numero_len;

                // lettura nome dalla connessione:

                memset(nome, 0, sizeof(nome));
                nome_len = sizeof(nome) - 1;

                if (rxb_readline(&rxb, ns, nome, &nome_len) < 0)
                {
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }

#ifdef USE_LIBUNISTRING
                if (u8_check((uint8_t *)nome, nome_len) != NULL)
                {
                    frpitnf(stderr, "Request is not valid UTF-8");
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }
#endif

                // lettura data dalla connessione:

                memset(data, 0, sizeof(data));
                data_len = sizeof(data) - 1;

                if (rxb_readline(&rxb, ns, data, &data_len) < 0)
                {
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }

#ifdef USE_LIBUNISTRING
                if (u8_check((uint8_t *)data, data_len) != NULL)
                {
                    frpitnf(stderr, "Request is not valid UTF-8");
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }
#endif

                // lettura nome dalla connessione:

                memset(numero, 0, sizeof(numero));
                numero_len = sizeof(numero) - 1;

                if (rxb_readline(&rxb, ns, numero, &numero_len) < 0)
                {
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }

#ifdef USE_LIBUNISTRING
                if (u8_check((uint8_t *)numero, numero_len) != NULL)
                {
                    frpitnf(stderr, "Request is not valid UTF-8");
                    rxb_destroy(&rxb);
                    close(ns);
                    exit(EXIT_FAILURE);
                }
#endif

                char nomefile[MAX_REQUEST_SIZE + 256];

                //Uso il percorso in locale ./field_service/CoimbraEnergy/20241220.txt) al posto di /var/local/field_service/nome/data.txt).
                snprintf(nomefile,sizeof(nomefile),"./field_service/%s/%s.txt",nome,data);

                if(pipe(p1p2)<0){
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }
                if((pid1=fork())<0){
                    perror("FORK");
                    exit(EXIT_FAILURE);
                }else if(pid1==0){
                    //NIPOTE 1

                    close(ns);
                    close(p1p2[0]);

                    //ridirezione stdout
                    close(1);
                    if(dup(p1p2[1])<0){
                        perror("pipe1");
                        exit(EXIT_FAILURE);
                    }
                    close(p1p2[1]);

                    execlp("sort","sort", "-n","-r",nomefile,(char*)NULL);
                    perror("sort");
                    exit(EXIT_FAILURE);
                }

                if((pid2=fork())<0){
                    perror("fork2");
                    exit(EXIT_FAILURE);
                }else if(pid2==0){
                    //NIPOTE 2
                    close(p1p2[1]);

                    //ridirezione stdin
                    close(0);
                    if(dup(p1p2[0])<0){
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }
                    close(p1p2[0]);

                    //ridirezione stdout
                    close(1);
                    if(dup(ns)<0){
                        perror("dup ns");
                        exit(EXIT_FAILURE);
                    }

                    close(ns);
                    execlp("head","head","-n",numero,(char*)NULL);
                    perror("head");
                    exit(EXIT_FAILURE);
                }

                close(p1p2[0]);
                close(p1p2[1]);

                waitpid(pid1,&status,0);
                waitpid(pid2,&status,0);

                if(write_all(ns,end_request,strlen(end_request))<0){

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
