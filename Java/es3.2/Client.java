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

    public static void main(String args[]) {

        if (args.length != 4) {
            System.out.println("Usage: Client hostname porta stringa nomefile ");
            System.exit(1);
        }

        try {

            // creo socket
            Socket s = new Socket(args[0], Integer.parseInt(args[1]));

            // canali della socket
            BufferedReader netIn = new BufferedReader(new InputStreamReader(s.getInputStream(), "UTF-8"));
            BufferedWriter netOut = new BufferedWriter(new OutputStreamWriter(s.getOutputStream(), "UTF-8"));

            // invio al server il nome del file args[3]
            netOut.write(args[3]);
            netOut.newLine();
            netOut.flush();

            // leggo risposta del server se il file esiste
            String risposta;
            risposta = netIn.readLine();
            if (risposta == null || risposta.equalsIgnoreCase("FILE NOT FOUND")) {

                System.out.println("Errore il file non esiste"); // se non esiste chiudo ed esco
                s.close();
                System.exit(3);
            }

            // mando stringa al server con la quale fare la grep

            netOut.write(args[2]);
            netOut.newLine();
            netOut.flush();

            // Il server mi manda ciò che trova e faccio stamapre ciò al client
            String line;
            line = netIn.readLine();
            // finchè leggo cose che mi ha mandato il server
            while (line != null) {

                if (line.equals("---END REQUEST---")) { // se manda ---END REQUEST--- allora ha finito e termino
                    break;
                }

                // stampo

                System.out.println(line);

                line = netIn.readLine();// leggo nuova riga per successiva trascrittura

            }

            // chiudo socket
            s.close();
        } catch (IOException e) {
            System.err.println(e.getMessage());
            e.printStackTrace();
            System.exit(1);
        }

    }

}

// QUando testo con nc esso mi stamperà il nome file e non subito la striga
// perchè attende risposta dal server se file esiste
// Se esistesse il file allora digitando tutto che non sia un "file not found"
// procedo col comunicare la stringa per la grep
// Una volta ricevute le informazioni dal Server, il Client le stampa a video e
// quindi termina.” non mi chiede di farlo con reuse
