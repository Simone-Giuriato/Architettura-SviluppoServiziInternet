import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.ServerSocket;
import java.net.Socket;

public class RemoteHeadServer {
    public static void main(String args[]) {

        if (args.length != 1) {
            System.err.println("Usage: java RemoteHeadServer porta");
            System.exit(1);
        }

        try {
            

            ServerSocket ss = new ServerSocket(Integer.parseInt(args[0])); // ServerSocket è
                                                                           // una classe di
                                                                           // Java che serve
                                                                           // a creare un
                                                                           // server TCP,
                                                                           // rimane in
                                                                           // ascolto (classe
                                                                           // Socket sarebbe
                                                                           // SocketServer)
                                                                           

           
            // ciclo infinito in attesa
            for (;;) {
                // quando un client si connette, accetta la connessione
                // e ti restituisce un oggetto Socket che rappresenta
                // quella comunicazione. (per ogni client una socket mentre ServerSocket serve
                // per rimanere in ascolto)
                Socket s = ss.accept();
               

                BufferedReader netIn = new BufferedReader(new InputStreamReader(s.getInputStream(), "UTF-8"));
                BufferedWriter netOut = new BufferedWriter(new OutputStreamWriter(s.getOutputStream(), "UTF-8"));

                // leggo il filename dalla socket
                String filename = netIn.readLine();
                File f = new File(filename);

                // controllo se file esiste
                if (f.exists()) {
                    BufferedReader bfr = new BufferedReader(new FileReader(f)); // ileReader è una classe Java che legge
                                                                                // caratteri da un file. (non occorre
                                                                                // specifare utf8)
                    String line;
                    int line_number = 1;

                    while ((line = bfr.readLine()) != null && line_number <= 5) {

                        netOut.write(line);
                        netOut.newLine();
                        netOut.flush();
                        line_number++;

                    }

                }

                s.close();
            }

        } catch (IOException e) {
            System.err.println(e.getMessage());
        }
    }

}
