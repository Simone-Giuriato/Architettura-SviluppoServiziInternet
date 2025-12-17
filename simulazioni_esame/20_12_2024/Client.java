import java.io.*;
import java.net.*;

public class Client {

    public static void main(String args[]) {
        if (args.length != 2) {
            System.err.println("Errore! Uso: java Client hostanme porta");
            System.exit(1);
        }

        try {

            Socket s = new Socket(args[0], Integer.parseInt(args[1]));

            BufferedReader netIn = new BufferedReader(new InputStreamReader(s.getInputStream(), "UTF-8"));
            BufferedWriter netOut = new BufferedWriter(new OutputStreamWriter(s.getOutputStream(), "UTF-8"));

            BufferedReader fromUser = new BufferedReader(new InputStreamReader(System.in));

            for (;;) {
                System.out.println("Inserisci nome cliente (fine per uscire):");
                String nome = fromUser.readLine();

                if (nome.equals("fine")) {
                    break;
                }
                System.out.println("Inserisci la data (fine per uscire):");
                String data = fromUser.readLine();

                if (data.equals("fine")) {
                    break;
                }

                System.out.println("Inserisci numero da visualizzare (fine per uscire):");
                String numero = fromUser.readLine();

                if (numero.equals("fine")) {
                    break;
                }

                // mando al server
                netOut.write(nome);
                netOut.newLine();
                netOut.flush();
                netOut.write(data);
                netOut.newLine();
                netOut.flush();
                netOut.write(numero);
                netOut.newLine();
                netOut.flush();

                String response;
                for (;;) {

                    response = netIn.readLine();

                    if (response == null) {
                        System.err.println("Errore! Il server ha chiuso la connessione");
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
