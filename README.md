# PageRank Laboratorio 2B aa. 23/24

## Utilizzo dei Thread per la Parallelizzazione

Nel progetto, i thread sono stati utilizzati per parallelizzare il processo di lettura del grafo e il calcolo del vettore PageRank, al fine di migliorare l'efficienza complessiva dell'algoritmo.

### Fase di Lettura del Grafo

La lettura del grafo avviene utilizzando un modello produttore-consumatore. Il thread principale legge gli archi dal file di input e li inserisce in un buffer circolare. Contemporaneamente, più thread consumatori prelevano questi archi dal buffer e li inseriscono nella struttura dati del grafo.

~~~c
// Lettura degli archi e inserimento nel buffer
while (getline(&line, &len, fd) != -1) {
    arco arch;
    sscanf(line, "%d %d", &arch.from, &arch.to);

    // Normalizzazione degli archi
    arch.from--;
    arch.to--;

    // Inserimento dell'arco nel buffer
    xsem_wait(&free_slots, QUI);
    xpthread_mutex_lock(&bmutex, QUI);
    Buffer[pbindex % BUFFSIZE] = arch;
    pbindex++;
    xpthread_mutex_unlock(&bmutex, QUI);
    xsem_post(&items, QUI);
}
~~~

- Il thread principale legge ogni arco dal file e lo inserisce in un buffer circolare protetto da un mutex (`bmutex`).
- I thread consumatori prelevano gli archi dal buffer, li normalizzano e li inseriscono nel grafo, utilizzando mutex e semafori per gestire la concorrenza.

### Fase di Calcolo del PageRank

Il calcolo del vettore PageRank è stato parallelizzato creando più thread calcolatori, ciascuno dei quali è responsabile di aggiornare una porzione del vettore xnext per ogni iterazione dell'algoritmo.

~~~c
// Creazione dei thread calcolatori
create_calculator_threads(threads, data, g, x, y, xnext, y_aux, &St, &St_new, term1, d, &iter, &errore, &v_cond, &t_cond, &aux, &massimo_next, taux);

// Loop principale di calcolo del PageRank
do {
    xpthread_mutex_lock(&t_mutex, QUI);
    while (t_cond.terminated != g->N) {
        xpthread_cond_wait(&t_cv, &t_mutex, QUI);
    }
    // Sincronizzazione dei thread
    t_cond.terminated = 0;
    xpthread_mutex_unlock(&t_mutex, QUI);

    if (errore < eps) {
        break;
    }

    iter++;
} while (iter <= maxiter);
~~~

- I thread calcolatori lavorano in parallelo per aggiornare le componenti del vettore `xnext`.
- Dopo ogni iterazione, i thread si sincronizzano per aggiornare variabili condivise come `errore` e `St_new`.

### Gesione dei segnali

Un thread separato viene utilizzato per gestire i segnali SIGUSR1 e SIGTERM. Questo thread consente di monitorare lo stato dell'algoritmo durante l'esecuzione e di terminare il programma in modo sicuro.

~~~c
void *handler_body(void *d) {
    handler_data *dati = (handler_data *)d;
    int s;
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGTERM);
    
    while (true) {
        sigwait(&mask, &s);
        if (s == SIGUSR1) {
            // Mostra lo stato corrente
            xpthread_mutex_lock(dati->mutex, QUI);
            fprintf(stderr, "Iterazione corrente %d: nodo massimo %d con rank %f\n", *(dati->iterazione), dati->massimo->indice, dati->massimo->rank);
            xpthread_mutex_unlock(dati->mutex, QUI);
        } else if (s == SIGTERM) {
            return NULL;
        }
    }
}
~~~

- Il thread gestore cattura i segnali e risponde mostrando lo stato corrente o terminando il programma.

## Gestione dei Thread in `graph_server.py` e `graph_client.py`

Di seguito viene descritto come i thread sono gestiti all'interno di ciascuno di questi programmi.

### `graph_client.py`

Il client è progettato per inviare più file di grafi a un server, dove ogni file viene gestito in un thread separato. Questo approccio consente al client di inviare più richieste in parallelo, migliorando l'efficienza quando si devono processare molti file.

~~~python
def main(file_list):
    """Avvia i thread per elaborare ciascun file."""
    threads = []
    for filename in file_list:
        thread = threading.Thread(target=process_file, args=(filename,))
        thread.start()
        threads.append(thread)

    for thread in threads:
        thread.join()
~~~

- **Creazione dei Thread:** Per ogni file nella lista `file_list`, viene creato un thread che esegue la funzione `process_file`. Questo permette al client di inviare e processare più file contemporaneamente.
- **Sincronizzazione:** Dopo aver avviato tutti i thread, il client attende che tutti i thread finiscano l'esecuzione utilizzando `thread.join()`, garantendo che tutte le operazioni siano completate prima di terminare il programma.

### `graph_server.py`

Il server è implementato per gestire connessioni multiple dai client utilizzando un `ThreadPoolExecutor`. Questo approccio consente al server di accettare e processare simultaneamente più richieste dai client, migliorando la scalabilità.

~~~python
def setup_server(host=HOST, port=PORT):
    """Configura il server e accetta connessioni."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
        server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server_socket.bind((host, port))
        server_socket.listen()
        logging.info(f"Server in ascolto su {host}:{port}")

        with concurrent.futures.ThreadPoolExecutor() as executor:
            while True:
                logging.info("In attesa di un client...")
                conn, addr = server_socket.accept()
                logging.info(f"Connessione accettata da {addr}")
                executor.submit(handle_client, conn, addr)
~~~

- **ThreadPoolExecutor:** Il server utilizza un `ThreadPoolExecutor` per gestire le connessioni client in modo concorrente. Ogni nuova connessione viene gestita da un thread separato, che esegue la funzione `handle_client`.
- **Scalabilità:**  Questo modello permette al server di scalare facilmente e gestire numerose connessioni simultanee, ciascuna in un thread separato.

