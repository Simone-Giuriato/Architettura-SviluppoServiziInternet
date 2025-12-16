import java.io.*;
import java.net.*;

public class Client {

    public static void main(String args[]) {

        if (args.length != 2) {
            System.err.println("Errore! uso java Client hostname porta");
            System.exit(1);
        }

        try {
            Socket s = new Socket(args[0], Integer.parseInt(args[1]));

            BufferedReader formUser = new BufferedReader(new InputStreamReader(System.in));
            BufferedReader netIn = new BufferedReader(new InputStreamReader(s.getInputStream(), "UTF-8"));
            BufferedWriter netOut = new BufferedWriter(new OutputStreamWriter(s.getOutputStream(), "UTF-8"));

            for (;;) {
                System.out.println("Inserisci nazione (fine per uscire):");
                String nazione = formUser.readLine();

                if (nazione.equals("fine")) {
                    break;
                }

                System.out.println("Inserisci tipologia (fine per uscire):");
                String tipologia = formUser.readLine();

                if (tipologia.equals("fine")) {
                    break;
                }

                System.out.println("Inserisci livello di budget (fine per uscire):");
                String budget = formUser.readLine();

                if (budget.equals("fine")) {
                    break;
                }

                // mando al server
                netOut.write(nazione);
                netOut.newLine();
                netOut.flush();
                netOut.write(tipologia);
                netOut.newLine();
                netOut.flush();
                netOut.write(budget);
                netOut.newLine();
                netOut.flush();

                String response;
                for (;;) {
                    response = netIn.readLine();

                    if (response == null) {
                        System.err.println("Errore! il server ha chiuso connessione");
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
            System.exit(3);
        }

    }

}