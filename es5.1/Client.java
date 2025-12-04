import java.io.*;
import java.net.*;

public class Client {

    public static void main(String args[]) {
        if (args.length != 2) {
            System.err.println("Errore! Uso: java Client hostname, porta");
            System.exit(1);
        }

        try {
            Socket s = new Socket(args[0], Integer.parseInt(args[1]));

            BufferedReader netIn = new BufferedReader(new InputStreamReader(s.getInputStream(), "UTF-8"));
            BufferedWriter netOut = new BufferedWriter(new OutputStreamWriter(s.getOutputStream(), "UTF-8"));

            BufferedReader formUser = new BufferedReader(new InputStreamReader(System.in));

            for (;;) {
                System.out.println("Inserisci una regione (fine per terminare)");
                String regione = formUser.readLine();

                if (regione.equalsIgnoreCase("fine")) {
                    break;
                }

                System.out.println("Iserisci un numero di località (fine per terminare)");
                String n_localita = formUser.readLine();

                if (n_localita.equalsIgnoreCase("fine")) {
                    break;
                }

                netOut.write(regione);
                netOut.newLine();
                netOut.flush();

                netOut.write(n_localita);
                netOut.newLine();
                netOut.flush();

                String response;
                for (;;) {
                    response = netIn.readLine();

                    if (response == null) {

                        System.err.println("Errore! Il server ha chiuso la connesione\n");
                        System.exit(4);

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
