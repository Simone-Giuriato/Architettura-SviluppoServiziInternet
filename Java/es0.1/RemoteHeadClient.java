
// nc -6 -l 5100 (per simulare di essere un server,netcat)
//altro terminale: java RemoteHeadClient localhost 5100 server.rb
// java RemoteHeadClient    hostname   porta   nomefile
//                          args[0]    args[1] args[2] 

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.Socket;

public class RemoteHeadClient {

    public static void main(String args[]) {

        if(args.length != 3){
            System.err.println("Usage: RemoteHeadClient    hostname   porta   nomefile");
            System.exit(1);
        }
        try { //con Socket BISOGNA gestire eccezioni (checked)
            Socket s = new Socket(args[0], Integer.parseInt(args[1])); // costruttore Socket vuole stringa (hostname) e
                                                                       // intero (porta) quindi devo convertire in
                                                                       // intero la porta che arriva come stringa al
                                                                       // main

            // devo ottenere i 2 estremi di I/O per leggere e scrivere socket

            BufferedReader netIn = new BufferedReader(new InputStreamReader(s.getInputStream(), "UTF-8"));
            // getInputStream() restituisce un flusso di byte (InputStream) dalla
            // connessione TCP (leggo dati che arrivano dal server)
            // InputStreamReader trasforma byte in caratteri (char), usando una codifica
            // (qui "UTF-8"). (Senza questo, leggeresti solo byte grezzi, non caratteri
            // leggibili.)
            // BufferedReader legge più dati in blocco invece di un carattere alla volta
            BufferedWriter netOut = new BufferedWriter(new OutputStreamWriter(s.getOutputStream(), "UTF-8"));

            // mando il nome del file args2
            netOut.write(args[2]); // scrivo nel buffer Buffered Write (write è un suo metodo)
            netOut.newLine(); // aggiungo una riga vuota /n per far capire che è finita
            netOut.flush(); // Con flush(), il buffer viene svuotato immediatamente e i dati vengono mandati
                            // al socket.

            //leggo dati dal server
            String line;
            int line_number=1;  //per mandare massimo 5 parole
            while ((line = netIn.readLine()) != null && line_number<=5) { // leggo finchè c'è qualcosa: readLine() legge una riga completa
                  //e li mando a video                                      // di testo dal server. (Buffer reader fornisce readline)
                System.out.println(line);
                line_number++;

            }

            // ho finito e chiudo la socket:
            s.close();
        } catch (IOException e) {
            System.err.println(e.getMessage());
            System.exit(2);
        }
    }

}
