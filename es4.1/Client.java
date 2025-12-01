import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.net.Socket;

public class Client {

    public static void main(String[] argv) {
        if (argv.length != 2) {
            System.err.println("Uso corrett: java Client server porta");
            System.exit(1);
        }

        try {

            BufferedReader fromUser = new BufferedReader(new InputStreamReader((System.in)));

            while (true) {
                System.out.println("Inserisci una categoria (fine per uscire):");
                String categoria = fromUser.readLine();
                if (categoria.equalsIgnoreCase("fine")) {
                    System.out.println("Hai scelto di terminare il programma");
                    break;
                }

                Socket s = new Socket(argv[0], Integer.parseInt(argv[1]));
                BufferedReader netIn = new BufferedReader(new InputStreamReader(s.getInputStream(), "UTF-8"));
                BufferedWriter netOut = new BufferedWriter(new OutputStreamWriter(s.getOutputStream(), "UTF-8"));

                netOut.write(categoria);
                netOut.newLine();
                netOut.flush();

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

            }

        } catch (IOException e) {
            System.err.println(e.getMessage());
            System.exit(1);
        }
    }

}
