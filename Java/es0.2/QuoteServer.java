import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;

public class QuoteServer {

    static final String[] quotations = {
            "Adoro i piani ben riusciti",
            "Quel tappeto dava veramente un tono all'ambiente",
            "Se ci riprovi ti stacco un braccio",
            "Questo è un colpo di genio, Leonard",
            "La vita è cosi, ma a me piace così"

    };

    public static void main(String[] args) {

        if (args.length != 1) {
            System.err.println("Usage: java Quoteserver porta");
            System.exit(1);
        }

        try {

            // indice corrente per ciclare le quote
            int index = 0;
            // Creo socket
            DatagramSocket ds = new DatagramSocket(Integer.parseInt(args[0])); // vuole intero

            while (true) {

                byte[] reqBuf = new byte[2048]; // creo array di byte buffer

                DatagramPacket reqPacket = new DatagramPacket(reqBuf, reqBuf.length); // Un DatagramPacket è un oggetto
                                                                                      // della libreria Java che
                                                                                      // rappresenta un pacchetto di
                                                                                      // dati da inviare o ricevere
                                                                                      // tramite il protocollo UDP

                // leggo il pacchetto (il messaggio di richiesta)
                ds.receive(reqPacket);

                // estraggo la stringa dal messaggio
                String request = new String(reqPacket.getData(), 0, reqPacket.getLength(), "UTF-8"); // costruttore
                                                                                                     // datagram packet
                                                                                                     // vuole così (è
                                                                                                     // piu difficile
                                                                                                     // datagramsocket

                if (request.equals("QUOTE")) {
                    // ottengo prossima quote
                    String quote = quotations[index % quotations.length];

                    // converto le quote da stringa a byte
                    byte[] respBuf = quote.getBytes("UTF-8");

                    // preparo datagram packet di risposta
                    DatagramPacket resPacket = new DatagramPacket(respBuf, respBuf.length, reqPacket.getAddress(),
                            reqPacket.getPort());

                    // mando datagram packet
                    ds.send(resPacket);

                }
                index += 1; // incremento

            }

        } catch (IOException e) {
            System.err.println(e.getMessage());
            e.printStackTrace(); // tampa la traccia dello stack dell'eccezione e, cioè mostra dove si è
                                 // verificato l’errore e da quali metodi è passato il programma prima di
                                 // arrivare lì.
            System.exit(2);
        }
    }

}
