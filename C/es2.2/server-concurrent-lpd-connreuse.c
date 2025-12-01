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
#ifdef USE_LIBSTRING
#include <unsistr.h>
#endif
#include "utils.h"

#define MAX_REQUEST_SIZE (64 * 1024)

/* ============================================================
 *  Gestore SIGCHLD
 *  ------------------------------------------------------------
 *  Serve a evitare i processi zombie.  
 *  waitpid(-1, WNOHANG) raccoglie tutti i figli terminati senza
 *  bloccare il processo padre.
 * ============================================================ */
void handler(int signo)
{
    int status;
    (void)signo; // non usato

    while (waitpid(-1, &status, WNOHANG) > 0)
    {
        // continuo finché rimangono figli terminati da raccogliere
    }
}

int main(int argc, char *argv[])
{
    struct addrinfo hints, *res;
    int err, sd, ns, pid, on;
    struct sigaction sa;

    /* ============================================================
     *  Controllo argomenti
     * ============================================================ */
    if (argc != 2)
    {
        fprintf(stderr, "USO: %s porta\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Evito che SIGPIPE uccida il server nel caso un client chiuda la socket */
    signal(SIGPIPE, SIG_IGN);

    /* ============================================================
     *  Installazione gestore SIGCHLD con sigaction
     *  ------------------------------------------------------------
     *  SA_RESTART evita che accept() fallisca con EINTR
     * ============================================================ */
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = handler;

    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    /* ============================================================
     *  Preparazione indirizzo server (bind su tutte le interfacce)
     * ============================================================ */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;      // IPv4 o IPv6
    hints.ai_socktype = SOCK_STREAM;  // TCP
    hints.ai_flags = AI_PASSIVE;      // bind su tutte le interfacce

    if ((err = getaddrinfo(NULL, argv[1], &hints, &res)) != 0)
    {
        fprintf(stderr, "Errore setup indirizzo bind: %s\n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }

    /* Creazione socket */
    if ((sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
    {
        perror("Errore in socket");
        exit(EXIT_FAILURE);
    }

    /* SO_REUSEADDR: permette di riavviare il server subito dopo un crash */
    on = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    /* Bind alla porta indicata */
    if (bind(sd, res->ai_addr, res->ai_addrlen) < 0)
    {
        perror("Errore in bind");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);

    /* Socket passiva in ascolto */
    if (listen(sd, SOMAXCONN) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    /* ============================================================
     *  CICLO PRINCIPALE DEL SERVER (PADRE)
     * ============================================================ */
    for (;;)
    {
        printf("Server in ascolto...\n");

        /* Attende connessioni */
        if ((ns = accept(sd, NULL, NULL)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        /* ========================================================
         *  Processo figlio per gestire il client
         * ======================================================== */
        if ((pid = fork()) < 0)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            /* ===== FIGLIO ===== */

            uint8_t len[2];
            int letti = 0;
            char stringa1[MAX_REQUEST_SIZE];
            char stringa2[MAX_REQUEST_SIZE];
            size_t dim_stringa1, dim_stringa2, dim_response;
            char response[80];

            /* Il figlio chiude la socket passiva */
            close(sd);

            /* ====================================================
             *  GESTIONE DELLA CONNESSIONE PERSISTENTE
             * ==================================================== */
            for (;;)
            {
                /* ------------------------------
                 * 1. Lettura lunghezza stringa1
                 * ------------------------------ */
                letti = read_all(ns, len, 2);
                if (letti < 0)
                {
                    perror("read");
                    exit(EXIT_FAILURE);
                }
                else if (letti == 0)
                {
                    printf("Client disconnesso.\n");
                    break;
                }

                /* Decodifica big endian → uint16 */
                dim_stringa1 = len[1] | (len[0] << 8);

                /* Lettura stringa 1 */
                memset(stringa1, 0, sizeof(stringa1));
                letti = read_all(ns, stringa1, dim_stringa1);

                if (letti <= 0)
                {
                    printf("Client disconnesso.\n");
                    break;
                }

                /* ------------------------------
                 * 2. Lettura lunghezza stringa2
                 * ------------------------------ */
                letti = read_all(ns, len, 2);
                if (letti <= 0)
                {
                    printf("Client disconnesso.\n");
                    break;
                }

                dim_stringa2 = len[1] | (len[0] << 8);

                /* Lettura stringa 2 */
                memset(stringa2, 0, sizeof(stringa2));
                letti = read_all(ns, stringa2, dim_stringa2);

                if (letti <= 0)
                {
                    printf("Client disconnesso.\n");
                    break;
                }

                /* ====================================================
                 *  ESECUZIONE DEL SERVIZIO:
                 *  confronto stringhe → risposta "SI" / "NO"
                 * ==================================================== */
                if (strcmp(stringa1, stringa2) == 0)
                    strncpy(response, "SI", sizeof(response));
                else
                    strncpy(response, "NO", sizeof(response));

                /* Codifica big endian della risposta */
                dim_response = strlen(response);
                len[0] = (dim_response & 0xFF00) >> 8;
                len[1] = (dim_response & 0xFF);

                /* Invio della lunghezza */
                if (write_all(ns, len, 2) < 0)
                {
                    perror("write");
                    exit(EXIT_FAILURE);
                }

                /* Invio della stringa risposta */
                if (write_all(ns, response, dim_response) < 0)
                {
                    perror("write");
                    exit(EXIT_FAILURE);
                }
            }

            /* Chiusura socket attiva */
            close(ns);

            /* Il figlio termina */
            exit(EXIT_SUCCESS);
        }

        /* ===== PADRE =====
         * torna ad accettare nuove connessioni
         */
    }

    return 0;
}
