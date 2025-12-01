#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>

int main(int argc, char *argv[])
{

    int sd, err;
    struct addrinfo hints, *res, *ptr;

    if (argc != 3)  //SEMPRE COSÌ
    {
        fprintf(stderr, "Usage %s <hostname> <porta>", argv[0]); // ricordo che rispetto a java anche il nome del programma in c è un argomento, uso stderr perchè deicatoa gli errori ma stampa a video come stdout/printf
        exit(EXIT_FAILURE);                                      // Termina immediatamente il programma da un codice diverso da 0
    }

    /* Ignora SIGPIPE
       Quando un client scrive su un socket chiuso dal server il kernel invia SIGPIPE:
       - Comportamento di default --> terminazione del processo
       - Qui si vuole evitare crash improvvisi
       */
      //SEMPRE COSÌ
    signal(SIGPIPE, SIG_IGN); // SIGPIPE è un segnale del sistema operativo generato dal kernel quando un processo tenta di scrivere su un socket o pipe già chiuso dall’altra parte. Comportamento di default: il processo termina immediatamente con un errore

    //SEMPRE COSÌ
    // preparo getaddrinfo (tradurre un hostname (es. www.google.com) o un indirizzo letterale (es. 127.0.0.1) e una porta (es. 80, 5000) in una lista di strutture struct addrinfo che contengono)
    memset(&hints, 0, sizeof(hints)); // Inizializza tutti i campi della struttura hints a zero.
    hints.ai_family = AF_UNSPEC;      // ai_family indica la famiglia di indirizzi IP che vogliamo: AF_UNSPEC qualsiasi, compatibile IPv4 e IPv6
    hints.ai_socktype = SOCK_STREAM;  // ai_socktype indica il tipo di socket richiesto: SOCK_STREAM (Tcp, affidabile)

    //SEMPRE COSÌ
    // invoco getaddrinfo
    err = getaddrinfo(argv[1], argv[2], &hints, &res); // Serve a risolvere un hostname e una porta in indirizzi IP pronti per la connessione TCP/UDP. (traduce in nomi in indirizzi Ip/porta)
    if (err != 0)
    {
        fprintf(stderr, "errore risoluzione nome: %s\n", gai_strerror(err));
        exit(EXIT_FAILURE); // Se getaddrinfo fallisce, restituisce un codice di errore non-zero.
    }

    // procedura di connessione (e creazione socket) che dura per ogni richiesta
    // connesione con fallback Fallback = provare un indirizzo dopo l’altro fino a trovare quello che funziona.
   //SEMPRE COSÌ
    for (ptr = res; ptr != NULL; ptr = ptr->ai_next)
    {
        sd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

        if (sd < 0)
        { // se socket fallisce slto direttamente a prossima iterazione ( prossimo indirizzo)
            fprintf(stderr, "creazione socket fallita\n");
            continue;
        }

        // ora faccio la connect e se funziona esco dal ciclo:
        if (connect(sd, ptr->ai_addr, ptr->ai_addrlen) == 0)
        {
            printf("connesione riusicta\n");
            break; // esco dal ciclo (attenzione che non chiudo questa socket)
        }
       
        close(sd); // se arrivo qua la connessione è fallita ne devo provare un altra, chiudo questa socket inutile--> concetto connessione con fallback
    }

    /* Control connection, verifico risulato getaddrinfo */
    /* Significa che ho esaurito tutti gli indirizzi senza riuscire a connettermi */
    //SEMPRE COSÌ
    if (ptr == NULL)
    {
        fprintf(stderr, "Errore di connessione: impossibile contattare il server\n");
        freeaddrinfo(res);  // libero la memoria allocata da getaddrinfo()
        exit(EXIT_FAILURE);
    }

    char line[4096];
    char len[2048];
    int nread;
    //SEMPRE COSÌ cambierà qualcosina ma la struttura è questa
    while (1) // ciclo infinito finchè non troverò un break
    {

        // leggo input dell'utente:
        printf("Inserisci una stringa:");
        if (fgets(line, sizeof(line), stdin) == NULL)
        { // gets legge una riga di testo da uno stream e la salva in un buffer.
            perror("fgets");
            exit(EXIT_FAILURE);
        }
        /* fgets inserts also \n into the buffer,
       so I replace it with a null byte (string terminator \0) */
        line[strlen(line) - 1] = '\0';

        // se utente digita fine--> esco
        if (strcmp(line, "fine") == 0)
        {
            break;
        }

        // Invio al server
        if (write(sd, line, strlen(line)) < 0)  //CAMBIERÀ
        {
            perror("write");
            exit(EXIT_FAILURE);
        }
     

        // leggo da server

        memset(len, 0, sizeof(len));
        nread = read(sd, len, sizeof(len) - 1);
        if (nread < 0)
        {
            perror("read");
            exit(EXIT_FAILURE);
        }
        printf("numeri di caratteri nella stringa: %s\n", len);
    }
    /* Chiusura socket --> fine della sessione TCP */
    close(sd);
}

