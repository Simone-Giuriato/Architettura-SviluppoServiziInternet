import java.io.*;
import java.net.*;

public class Client {

    public static void main(String argv[]) {
        if (argv.length != 2) {

            System.err.println("errore! Uso: java  Client hostname porta");
            System.exit(1);

        }

        try {

            Socket s = new Socket(argv[0], Integer.parseInt(argv[1]));

            BufferedReader netIn = new BufferedReader(new InputStreamReader(s.getInputStream(), "UTF-8"));
            BufferedWriter netOut = new BufferedWriter(new OutputStreamWriter(s.getOutputStream(), "UTF-8"));

            BufferedReader fromUser = new BufferedReader(new InputStreamReader(System.in));

            for (;;) {

                System.out.println("Inserisci username (fine per terminare)");
                String username = fromUser.readLine();

                if (username.equals("fine")) {
                    break;
                }

                System.out.println("Inserisci password (fine per terminare)");
                String password = fromUser.readLine();

                if (password.equals("fine")) {
                    break;
                }

                // invio al server
                netOut.write(username);
                netOut.newLine();
                netOut.flush();
                netOut.write(password);
                netOut.newLine();
                netOut.flush();

                String response;

                for (;;) {
                    response = netIn.readLine();

                    if (response == null) {
                        System.err.println("Server ha chiuso la connessione");
                        System.exit(3);
                    }
                    System.out.println(response);

                    if (response.equals("--- END REQUEST ---")) {
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
