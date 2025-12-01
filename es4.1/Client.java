import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.Socket;

public class Client {

    public static void main(String[] argv) {

        if (argv.length != 2) {
            System.err.println("Errore!, Uso: java Client hostname porta");
            System.exit(1);
        }

        try {

            Socket s = new Socket(argv[0], Integer.parseInt(argv[1]));

            BufferedReader netIn = new BufferedReader(new InputStreamReader(s.getInputStream(), "UTF-8"));
            BufferedWriter netOut = new BufferedWriter(new OutputStreamWriter(s.getOutputStream(), "UTF-8"));

            BufferedReader fromUser = new BufferedReader(new InputStreamReader(System.in));

            for (;;) {
                System.out.println("Inserisci categoria (fine per uscire)");
                String categoria = fromUser.readLine();

                if (categoria.equalsIgnoreCase("fine")) {
                    break;
                }

                netOut.write(categoria);
                netOut.newLine();
                netOut.flush();

                String line;
                for (;;) {
                    line = netIn.readLine();
                    if (line == null) {
                        System.err.println("Erroe il server ha chiuso la connessione");
                        System.exit(2);
                    }

                    System.out.println(line);

                    if (line.equals("--- END REQUEST ---")) {
                        break;
                    }
                }

            }
            s.close();

        } catch (IOException e) {
            System.err.println(e.getMessage());
            e.printStackTrace();
            System.exit(2);

        }

    }

}
