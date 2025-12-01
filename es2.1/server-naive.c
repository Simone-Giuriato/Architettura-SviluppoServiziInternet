/*
Tipo di server:

Persistente e concorrente per client sequenziali: il server rimane sempre in ascolto di nuove connessioni e gestisce un client alla volta.

Ogni client può inviare più richieste all’interno della stessa connessione.
NON È UN SERVER ITERATIVO, ma PERSISTENTE più simile a quello che avremo agli esami 
Anche se il client inserisce fine, e quindi termina, il server irmane in ascolto (fatto apposta) per altri client
*/

#define _POSIX_C_SOURCE 200809L
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char **argv)
{

    if (argc != 2) // SEMPRE COSÌ
    {
        fprintf(stderr, "Uso: %s <porta>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int sd, ns, err, on;             // SEMPRE COSÌ
    struct addrinfo hints, *res;     // SEMPRE COSÌ

    /* Ignora segnale SIGPIPE:
       Se il client chiude il socket e il server prova a scriver --> SIGPIPE --> default = kill del server.
       Ignorandolo evita la terminazione del processo.
    */
    signal(SIGPIPE, SIG_IGN); // SEMPRE COSÌ

    /* Prepare getaddrinfo */ // SEMPRE COSÌ
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     // compatibile IPv4 e IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;     // produce un indirizzo "bindabile" (equivalende a 0.0.0.0 o ::)

    /* Passando NULL come hostname, AI_PASSIVE fa sì che il server si vincoli a tutte le interfacce di rete. */
    if ((err = getaddrinfo(NULL, argv[1], &hints, &res)) != 0) // SEMPRE COSÌ
    {
        fprintf(stderr, "Error setting up bind address: %s\n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }

    /* Crea un passivo (listening) socket TCP nella famiglia fornita da getaddrinfo (IPv4 o IPv6) */
    if ((sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) // SEMPRE COSÌ
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    /* Serve per:
        - permettere di riavviare il server subito dopo la chiusura
        - evitare errori "address already in use"
        Altrimenti TCP mantiene la porta in stato TIME_WAIT per 2 minuti
    */
    on = 1;   // SEMPRE COSÌ                                               
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)  // SEMPRE COSÌ
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    /* Associa il socket all'indirizzo IP scelto (tutti, con AI_PASSIVE) e alla porta indicata da argv[1] */
    if (bind(sd, res->ai_addr, res->ai_addrlen) < 0)  // SEMPRE COSÌ
    {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }

    /* At this point, I can free the memory allocated by getaddrinfo */
    freeaddrinfo(res); // SEMPRE COSÌ

    /*
       Trasformo il socket in listening socket: riceve richieste di connessione.
       SOMAXCONN è la massima dimensione possibile della coda di connessioni pendenti consentita dal sistema.
    */
    if (listen(sd, SOMAXCONN) < 0) // SEMPRE COSÌ
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Attendo i client:
    char request[4096], response[256];
    int nread, length_of_string;

    for (;;) { // ciclo infinito per nuovi client // SEMPRE COSÌ
        printf("Server in ascolto...\n"); // SEMPRE COSÌ

        ns = accept(sd, NULL, NULL); // accetta una nuova connessione // SEMPRE COSÌ
        if (ns < 0) // SEMPRE COSÌ
        {
            /* I have not installed SIGCHLD handler with SA_RESTART,
             * so I have to explicitly check and handle the EINTR case
             */
            if (errno == EINTR) continue; // interruzione da segnale    //cambierà poichè gestiremo sigchild
            perror("accept");
            exit(EXIT_FAILURE);
        }

        //--- DA QUA CAMBIERÀ TOTALE perchè introdurremmo la concorrenza e quindi pid,figli,padri...

        while (1) { // ciclo per più richieste dallo stesso client
            memset(request, 0, sizeof(request)); // azzero buffer richiesta

            nread = read(ns, request, sizeof(request) - 1); // leggo dal client
            if (nread <= 0) break; // client chiuso o errore
            request[nread] = '\0'; // assicuro terminatore null

            // rimuovo eventuale newline
            if (nread > 0 && request[nread - 1] == '\n') request[nread - 1] = '\0';

            // se il client invia "fine", chiudo la connessione
            if (strcmp(request, "fine") == 0) break;

            // calcolo lunghezza della stringa
            length_of_string = strlen(request);

            // preparo la risposta
            snprintf(response, sizeof(response), "%d\n", length_of_string);

            // invio la risposta al client
            if (write(ns, response, strlen(response)) < 0) 
            {
                perror("write");
                break; // esce dal ciclo interno per passare al prossimo client
            }
        }

        close(ns); // chiudo la connessione corrente
    }

    /* Close passive socket (just in case) */
    close(sd);  //SEMPRE COSÌ

    return 0;
}


