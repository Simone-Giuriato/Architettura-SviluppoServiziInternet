Si progetti un'applicazione distribuita Clien/Server utilizzando Java e le socket di tipo datagram

Il Client deve presentare la seguente interfaccia:

java QuoteClient nodeServer portaServer

dove nodoServer è il nome della macchina server e portaServer è il numero di porta su cui fare le richieste.

Ogni volta che viene lanciato, il client deve inviare una richiesta al server, che a sua volta risponde con una citazione ovverosia una stringa presa da un array predefinito di aforismi, prover e frsai celebri.

Ad esemio, dato l'array:

String[] quotations = {
    "Adoro i piani ben riusciti",
    "Quel tappeto dava veramente un tono all'ambiente",
    "....",
    "....",
}
Alla prima richiesta del client il server restituirà la prima parola alla seconda la seconda ecc ecc...

