Si progetti un'applicazione Client/Server che, utilizzando le socket,
implementi un sistema di peer-review per la gestione degli articoli per una
collezione di riviste scientifiche. Più precisamente, l'applicazione deve
permettere ai revisori, incaricati di giudicare gli articoli sottomessi a una
rivista e verificare se siano meritevoli di pubblicazione, di stampare la lista
di articoli a essi assegnati per la revisione.

Il Client deve presentare la seguente interfaccia:

	lista_articoli   server   porta

dove server è il nome logico del Server e porta il numero di porta del
servizio.

Per prima cosa, il Client si interfaccia con il revisore, da cui riceve (via
terminale) indirizzo email, password, e nome rivista di interesse. (Si noti che
la stessa persona può svolgere il ruolo di revisore per più riviste,
possibilmente con diversi account.) Per ogni richiesta, il Client deve
comunicare le informazioni inserite dal revisore al Server. 

Il Server deve quindi verificare l'autorizzazione del revisore invocando
un'apposita funzione, che si suppone essere già implementata e con il seguente
prototipo:

	int autorizza(const char *email_revisore, const char *password);

Se la funzione autorizza restituisce un valore diverso da 1, l'utente non è
autorizzato e il Server dovrà rifiutarsi di eseguire il servizio. Altrimenti,
l'utente è autorizzato ad accedere al servizio. Il Server deve quindi
selezionare le informazioni sugli articoli da revisionare assegnati al revisore
e restituirle al Client, elencando gli articoli in ordine di data di scadenza
decrescente. Opzionalmente, il Server deve anche indicare all'utente quanti
articoli al momento deve revisionare per la rivista di interesse.

A tal proposito, si supponga che, sul Server, le informazioni su tutti gli
articoli da revisionare siano salvate in un file che contenga una riga (con
data di scadenza per la revisione in formato YYYYMMDD, ID dell'articolo,
titolo dell'articolo, indirizzo e-mail revisore, ecc.) per ciascuno degli
articoli registrati nel sistema.

Una volta ricevute le informazioni dal Server, il Client le stampa a video e si
mette in attesa della richiesta successiva. L'applicazione deve terminare
quando l'utente digita la stringa "fine".


ATTENZIONE: Si realizzino il Client e il Server in C, ma il Client deve anche
essere realizzato in Java.

NOTA: Ai fini della realizzazione di questo esercizio, si usi pure la seguente
implementazione per la funzione autorizza:

	int autorizza(const char *email_revisore, const char *password) {
		return 1;
	}
