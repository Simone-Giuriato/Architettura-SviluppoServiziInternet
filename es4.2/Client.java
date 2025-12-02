import java.io.*; // per InputStreamReader, OutputStreamWriter, BufferedReader, BufferedWriter, IOException
import java.net.*; // per Socket

public class Client {

    public static void main(String args[]) {

        // Controllo degli argomenti passati da riga di comando
        if (args.length != 2) {

            System.err.println("Errore! Uso: java Client hostanme porta");
            System.exit(1); // esco con codice 1 se argomenti non corretti

        }
        /*
         * Commento:
         * args[0] = hostname o indirizzo IP del server
         * args[1] = porta del server
         * Il client deve sapere dove connettersi.
         */

        try {

            // Creo una socket TCP verso il server specificato
            Socket s = new Socket(args[0], Integer.parseInt(args[1]));
            /*
             * Socket(host, port) apre una connessione TCP verso l'host e la porta indicati.
             * Se la connessione fallisce, lancia IOException.
             */

            // Creo reader e writer per la comunicazione con il server, usando UTF-8
            BufferedReader netIn = new BufferedReader(
                    new InputStreamReader(s.getInputStream(), "UTF-8"));
            BufferedWriter netOut = new BufferedWriter(
                    new OutputStreamWriter(s.getOutputStream(), "UTF-8"));

            /*
             * netIn legge dati dal server riga per riga
             * netOut scrive dati al server
             * InputStreamReader/OutputStreamWriter convertono byte <-> char
             * BufferedReader/BufferedWriter gestiscono il buffering per aumentare
             * efficienza
             */

            // Reader per leggere input dall'utente (tastiera)
            BufferedReader fromUser = new BufferedReader(new InputStreamReader(System.in));
            /*
             * fromUser.readLine() legge una riga completa inserita dall'utente
             * fino al newline (\n)
             */

            // Ciclo principale del client: richiede nome e annata al server
            for (;;) {
                System.out.println("Inserisci il nome di un vino (fine per uscire)");
                String nome = fromUser.readLine();
                // Leggo il nome inserito dall'utente

                if (nome.equalsIgnoreCase("fine")) {
                    break; // esco dal ciclo se l'utente vuole terminare
                }

                System.out.println("Inserisci il annata di un vino (fine per uscire)");
                String annata = fromUser.readLine();
                // Leggo l'annata inserita dall'utente

                if (annata.equalsIgnoreCase("fine")) {
                    break; // esco dal ciclo se l'utente vuole terminare
                }

                // Invio nome e annata al server
                netOut.write(nome); // scrivo la stringa del nome sul socket
                netOut.newLine(); // aggiungo newline per indicare la fine della riga
                netOut.flush(); // svuoto il buffer per inviare effettivamente i dati
                netOut.write(annata);
                netOut.newLine();
                netOut.flush();

                /*
                 * flush() è importante: BufferedWriter accumula dati in un buffer
                 * senza flush, i dati potrebbero non essere inviati subito al server
                 */

                // Leggo la risposta del server
                String risposta;
                for (;;) {
                    risposta = netIn.readLine();
                    /*
                     * netIn.readLine() legge una riga dal server
                     * ritorna null se la connessione viene chiusa
                     */

                    if (risposta == null) {
                        // Se il server chiude la connessione inaspettatamente
                        System.err.println("Errore! Il server ha chiuso la connessione");
                        System.exit(2);
                    }

                    System.out.println(risposta); // stampo la riga ricevuta dal server

                    if (risposta.equalsIgnoreCase("--- END REQUEST ---")) {
                        // Segnale di fine risposta dal server: esco dal ciclo di lettura
                        break;
                    }
                }

                // Dopo questo ciclo, si può richiedere un nuovo nome/annata
            }

            s.close(); // chiudo la socket quando l'utente termina il programma
            /*
             * close() libera le risorse della socket e segnala al server
             * che il client ha chiuso la connessione
             */

        } catch (IOException e) {
            // Gestione di tutte le eccezioni di I/O
            System.err.println(e.getMessage());
            e.printStackTrace(); // stampa la traccia dello stack per debug
            System.exit(3); // codice di uscita 3 per errore I/O
        }

    }

}
