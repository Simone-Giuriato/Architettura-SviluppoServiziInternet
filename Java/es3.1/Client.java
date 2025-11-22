//PER TESTARE SE VA IL CLIENT (senza fare server)
//Digitare in un altro terminale (prima di fare la richiesta col Client):
//nc -l -p 50000

// -l → ascolta in modalità server
// -p 50000 → indica la porta su cui il server “finto” deve ascoltare
// // Tutti i client che si connettono a quella porta saranno accettati
//netacat non permette di simulare appieno un server (ad esempio usando comando ps) 
//ma ciò che scrivo nel client lo scrive nel server e ciò che scrivo nel server lo manda al client così posso testare a garndi linee

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.Socket;

public class Client {

    public static void main(String[] args) {

        if (args.length != 2 && args.length != 3) { // accetto sia 2 sia 3 parametri perchè l opzione può essere omessa
            System.out.println("Usage: java Client <hostname> <porta> <option>");
            System.exit(1);
        }

        try {

            // stream socket

            Socket s = new Socket(args[0], Integer.parseInt(args[1]));

            // canali I/O socket

            BufferedReader netIn = new BufferedReader(new InputStreamReader(s.getInputStream(), "UTF-8"));
            BufferedWriter netOut = new BufferedWriter(new OutputStreamWriter(s.getOutputStream(), "UTF-8"));

            // buffer per leggere da tastiera

            BufferedReader bfr = new BufferedReader(new InputStreamReader(System.in));
            // System.in è un InputStream → legge byte grezzi dalla tastiera.
            // Ma se lavori con stringhe (metodo readLine() ecc.), hai bisogno di gestire
            // caratteri, non byte.
            // InputStreamReader serve proprio a questo: Converte i byte letti da System.in
            // in caratteri (String)/ nella codifica che metto
            String risposta; // risposta dal server

            // Se è presente un terzo parametro (opzione), invialo subito, devo gestire la casistica mi metta subito un parametro
            if (args.length == 3) {
                netOut.write(args[2]);
                netOut.newLine();
                netOut.flush();

                for (;;) {
                    risposta = netIn.readLine();

                    if (risposta == null) { // se non ho dati nella risposta allora server ha chiuso connessione
                        System.out.println("Errore! Il server ha chiuso la connessione");
                        System.exit(3);
                    }

                    // stampo risposta del server
                    System.out.println(risposta);

                    // quando il server ha finito di dare risposto esco da questo for e passo ad un
                    // altra richiesta
                    // "--- END REQUEST ---" sarà compito del server mandarmela come ultima riga...
                    // così che io client capisca--> devo usare la stessa sentinella
                    if (risposta.equals("--- END REQUEST ---")) {
                        break;
                    }
                }
            }

            String line; // comando che mi mette utente (omettendo l opzione)
            // questo for esterno gestisce le richieste del client
            for (;;) {

                System.out.println("Inserisci l'opzione per il comando ps (o finire per uscire) ");
                line = bfr.readLine();

                // se l utente mette "finire" esco
                if (line.equalsIgnoreCase("finire")) {
                    break;
                }

                // nel caso mi metta un comando valido, mando al server

                netOut.write(line);
                netOut.newLine();
                netOut.flush();

                // leggo risposta del server e stampo

                // questo for interno legge tutte le risposte che il server ha dato per quella
                // richiesta
                for (;;) {
                    risposta = netIn.readLine();

                    if (risposta == null) { // se non ho dati nella risposta allora server ha chiuso connessione
                        System.out.println("Errore! Il server ha chiuso la connessione");
                        System.exit(3);
                    }

                    // stampo risposta del server
                    System.out.println(risposta);

                    // quando il server ha finito di dare risposto esco da questo for e passo ad un
                    // altra richiesta
                    // "--- END REQUEST ---" sarà compito del server mandarmela come ultima riga...
                    // così che io client capisca--> devo usare la stessa sentinella
                    if (risposta.equals("--- END REQUEST ---")) {
                        break;
                    }

                }

            }

            // chiudo SEMPRE socket quando ho finito
            s.close();

        } catch (IOException e) {
            System.err.println(e.getMessage());
            e.printStackTrace();
            System.exit(2);
        }

    }

}

// Connection Reuse significa:

// Aprire una sola volta il socket verso il server e riutilizzarlo per più
// richieste consecutive, senza chiudere e riaprire la connessione ogni volta.
// Aprire e chiudere spreco risorse, quando metto finire allora esco

// In Java (e in molti altri linguaggi) il BREAK serve a uscire immediatamente
// dal ciclo più INTERNO in cui si trova :)