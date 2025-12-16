import java.io.*;
import java.net.*;

public class Client {

    public static void main(String args[]) {

        if (args.length != 2) {
            System.err.println("Errore! Uso: java Client hostname porta");
            System.exit(1);
        }
        try {

            Socket s = new Socket(args[0], Integer.parseInt(args[1]));

            BufferedReader netIn = new BufferedReader(new InputStreamReader(s.getInputStream(), "UTF-8"));
            BufferedWriter netOut = new BufferedWriter(new OutputStreamWriter(s.getOutputStream(), "UTF-8"));

            BufferedReader fromUser = new BufferedReader(new InputStreamReader(System.in));

            for (;;) {
                System.out.println("Inserisci username (fine per uscire)");
                String username = fromUser.readLine();

                if (username.equals("fine")) {
                    break;
                }

                System.out.println("Inserisci password (fine per uscire)");
                String password = fromUser.readLine();

                if (password.equals("fine")) {
                    break;
                }

                System.out.println("Inserisci giorno (fine per uscire)");
                String giorno = fromUser.readLine();

                if (giorno.equals("fine")) {
                    break;
                }

                // mando al server
                netOut.write(username);
                netOut.newLine();
                netOut.flush();
                netOut.write(password);
                netOut.newLine();
                netOut.flush();
                netOut.write(giorno);
                netOut.newLine();
                netOut.flush();

                String response;
                for (;;) {
                    response = netIn.readLine();

                    if (response == null) {
                        System.err.println("Errore!il server ha chiuso la connessione");
                        System.exit(2);
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
            System.exit((3));
        }

    }
}