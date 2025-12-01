import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.Socket;

public class RemoteSquareClient {

    public static void main(String[] args) {

        if (args.length != 2) {
            System.out.println("Usage: java RemoteSquareClient <hostname> <porta>");
            System.exit(1);

        }

        try {
            // creo socket stream

            Socket s = new Socket(args[0], Integer.parseInt(args[1])); // il costruttore vuole che secondo parametro sia
                                                                       // intero, lo converto perchè me lo passa come
                                                                       // stringa

            // creo i canali (contenitori) I/O per comunicare con la socket
            BufferedReader netIn = new BufferedReader(new InputStreamReader(s.getInputStream(), "UTF-8"));

            // s è il tuo Socket.
            // getInputStream() restituisce un InputStream che permette di leggere i byte
            // inviati dal server. È un flusso di byte grezzi, non ancora testo.
            // InputStreamReader converte i byte in caratteri secondo una codifica (qui
            // "UTF-8").
            // BufferedReader serve per:
            // velocizzare la lettura (usa un buffer interno)
            // fornire metodi più comodi come readLine()

            BufferedWriter netOut = new BufferedWriter(new OutputStreamWriter(s.getOutputStream(), "UTF-8")); // L’OutputStreamWriter
                                                                                                              // serve
                                                                                                              // per
                                                                                                              // convertire:
                                                                                                              // caratteri
                                                                                                              // Java →
                                                                                                              // byte
                                                                                                              // seguendo
                                                                                                              // la
                                                                                                              // codifica
                                                                                                              // "UTF-8".

            // devo comunicare (leggre anche dall utente) creo canale per leggere da
            // tatsitera:
            BufferedReader br = new BufferedReader(new InputStreamReader(System.in)); // System.in È un InputStream che
                                                                                      // legge byte dalla tastiera (lo
                                                                                      // standard input). Ma legge solo
                                                                                      // byte, non testo. quindi input
                                                                                      // streamer converto byte in
                                                                                      // codifica qui non c'è ma è
                                                                                      // sempre UTF-8 predefinita
            String line;
            System.out.println("Inserire un numero:");
            line = br.readLine(); // BUfferedReader non legge un buffer per leggere ho bisogno di associare metodo
                                  // readline che legge un riga, qui associo al buffer da tastiera (è più
                                  // complesso rispetto a scan ma utile)

            // Esco dal ciclo quanod l'utente scrive fine
            while (!line.equalsIgnoreCase("fine")) {

                // se non mette fine allora invio al server il numero da elaborare
                netOut.write(line); // Scrive il contenuto della variabile line nel buffer interno di netOut. non l
                                    // ho ancora mandato al server
                netOut.newLine(); // Aggiunge un carattere di newline (\n) al buffer. Questo è importante perché
                                  // chi legge con readLine() sul socket aspetta proprio il newline per sapere
                                  // dove finisce la riga.
                netOut.flush(); // Forza l’invio immediato dei dati nel buffer verso il server. Invio al server
                                // ciò che ho messo nel buffer netOut

                // Devo stampare a video ciò che mi restituisce il server

                String lineSocket;
                lineSocket = netIn.readLine(); // letto ciò che mi manda il server
                System.out.println("Il quadrato di " + line + " è: " + lineSocket + "\n");

                // chiedo nuovo numero per successiva elaborazione
                System.out.println("Inserire un numero:");
                line = br.readLine();

            }

            s.close();

        } catch (IOException e) {
            System.err.println(e.getMessage());
            e.printStackTrace();    //è usata in Java per stampare dettagliatamente un’eccezione
            System.exit(2);

        }

    }

}


//ANdrebbe controllato/gestita l eccezione qual'ora utente inserisca una stringa che fa crashare il server