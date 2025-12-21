import java.io.*;
import java.net.*;

public class Client {
    public static void main(String args[]) {

        if (args.length != 2) {
            System.err.println("Errore! Uso java Client hostname porta");
            System.exit(1);
        }

        try {
            Socket s = new Socket(args[0], Integer.parseInt(args[1]));  //creo oggetto socket TCP, passo come nome server e porta del server per la connessione

            //inizializzo buffer per gestione input/output della socket
            BufferedReader netIn = new BufferedReader(new InputStreamReader(s.getInputStream(), "UTF-8"));
            BufferedWriter netOut = new BufferedWriter(new OutputStreamWriter(s.getOutputStream(), "UTF-8"));

            //inizializzo buffer per input dall'utente
            BufferedReader fromUser = new BufferedReader(new InputStreamReader(System.in));

            for (;;) {
                System.out.println("Inserisci categoria del regalo (fine per uscire):");
                String categoria = fromUser.readLine(); //leggo dall'utente

                if (categoria.equals("fine")) {
                    break;
                }

                System.out.println("Inserisci produttore del regalo (fine per uscire):");
                String produttore = fromUser.readLine();

                if (produttore.equals("fine")) {
                    break;
                }

                System.out.println("Inserisci ordine di visualizzaione [crescente o decsrescente] (fine per uscire):");
                String ordinamento = fromUser.readLine();

                if (ordinamento.equals("fine")) {
                    break;
                }

                // mando al server

                netOut.write(categoria);
                netOut.newLine();
                netOut.flush();
                netOut.write(produttore);
                netOut.newLine();
                netOut.flush();
                netOut.write(ordinamento);
                netOut.newLine();
                netOut.flush();

                String response;
                //ciclo infinito per leggere le risposte che server ha messo in socket (ora leggo con netIn)
                for (;;) {
                    response = netIn.readLine();

                    if (response == null) { //se null significa che server ha chiuso connessione
                        System.err.println("Errore! Il server ha chiuso connessione");
                        System.exit(2);
                    }

                    System.out.println(response);
                    if (response.equals("--- END REQUEST ---")) {
                        break;
                    }
                }
            }
            s.close();

        } catch (IOException e) {   //gestisco eccezzione che può essere sollevata durante la comunicazione con il server
            System.err.println(e.getMessage());
            e.printStackTrace();
            System.exit(3);
        }
    }
}