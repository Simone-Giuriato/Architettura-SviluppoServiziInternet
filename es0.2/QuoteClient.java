//esercizio con socket datagram-->UDP

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;

public class QuoteClient {

    public static void main(String[] args) {
        if (args.length != 2) {
            System.err.println("Usage: java QuoteClient hostname porta");
            System.exit(1);
        }

        try {

            // creo socket:
            DatagramSocket ds = new DatagramSocket();
            byte[] reqBuf = new String("QUOTE").getBytes("UTF-8");
            DatagramPacket reqPacket = new DatagramPacket(reqBuf, reqBuf.length, InetAddress.getByName(args[0]),
                    Integer.parseInt(args[1]));

            // invio richiesta
            ds.send(reqPacket);

            // Preparo a ricevere risposta
            byte[] respBuf = new byte[2048];
            DatagramPacket respPacket = new DatagramPacket(respBuf, respBuf.length);

            // ricevo la risposta
            ds.receive(respPacket);

            // la converto in stringa di risposta e stampo
            String quote = new String(respPacket.getData(), 0, respPacket.getLength(), "UTF-8");
            System.out.println(quote);

            ds.close();

        } catch (IOException e) {
            System.err.println(e.getMessage());
            e.printStackTrace();
            System.exit(2);

        }
    }

}
