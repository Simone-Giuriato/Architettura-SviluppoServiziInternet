import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.ServerSocket;
import java.net.Socket;

public class RemoteSquareServer {

    public static void main(String[] args) {

        if (args.length != 1) {
            System.out.println("Usage: java RemoteSquareServer <port>");
            System.exit(1);
        }

        try {

            // creo socket del server
            ServerSocket ss = new ServerSocket(Integer.parseInt(args[0])); // è diversa dalla classe Socket del client
                                                                           // qui ho ServerSocket che richiede un solo
                                                                           // parametro int--> la porta

            // mi metto in ascolto con una condizione infinita for(;;)
            while (true) {

                // accetto la connesione che arriva

                Socket s = ss.accept(); // Blocca il programma fino a quando un client prova a connettersi
                                        // Quando arriva una connessione, crea un nuovo Socket collegato a quel client
                                        // Restituisce il Socket, che puoi usare per leggere/scrivere dati solo con quel
                                        // client (metodo accept fa parte di classe serverScoket)

                // creo i canali di comunicazione
                BufferedReader netIn = new BufferedReader(new InputStreamReader(s.getInputStream(), "UTF-8")); // uso s.
                                                                                                               // perchè
                                                                                                               // leggo
                                                                                                               // dallo
                                                                                                               // socket
                                                                                                               // che
                                                                                                               // creo,
                                                                                                               // ss mi
                                                                                                               // deve
                                                                                                               // sempre
                                                                                                               // rimanere
                                                                                                               // in
                                                                                                               // ascolto
                                                                                                               // per
                                                                                                               // accettare
                                                                                                               // connessioni
                BufferedWriter netOut = new BufferedWriter(new OutputStreamWriter(s.getOutputStream(), "UTF-8"));

                // Leggo in loop gli input lanciati dal mio client
                String line;
                line = netIn.readLine();
                while (line != null) {
                    int num;
                    num = Integer.parseInt(line); //il clinet con readline mi manda si un numero ma in formato di stringa e quindi devo convertre in numero
                    int square = num * num;

                    // mando al client
                    netOut.write(Integer.toString(square)); //converto numero in strnga perchè se mandassi numero usa formato unicode che non è il mio numero
                    netOut.newLine();
                    netOut.flush();
                    line = netIn.readLine(); //prendo un altra riga che mi manda il client (se no mettevo readline dentro al while)
                }
                s.close(); // chiudo scoket con quel client, ma rimango sempre in ascolto con ss

            }

        } catch (IOException e) {
            System.err.println(e.getMessage());
            e.printStackTrace();    //è usata in Java per stampare dettagliatamente un’eccezione
            System.exit(2);
        }

    }

}
