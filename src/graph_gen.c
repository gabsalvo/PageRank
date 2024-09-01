#include "../librerie/xerrori.h"
#include "../librerie/utili.h"

// Funzioni ausiliarie per migliorare la leggibilità e la modularizzazione
void initialize_graph(grafo *graph, int N);
void create_threads(pthread_t *threads, dati_consumatori *data, grafo *graph, arco *Buffer, pthread_mutex_t *bmutex, pthread_mutex_t *gmutex, sem_t *items, sem_t *free_slots, int *cbindex, int T);
void join_threads(pthread_t *threads, int T);
void destroy_synchronization_objects(pthread_mutex_t *bmutex, pthread_mutex_t *gmutex, sem_t *items, sem_t *free_slots);
void handle_invalid_arc(pthread_t *threads, arco *Buffer, pthread_mutex_t *bmutex, pthread_mutex_t *gmutex, sem_t *items, sem_t *free_slots, int *pbindex, int T);
void read_and_process_arcs(FILE *fd, grafo *graph, arco *Buffer, pthread_t *threads, pthread_mutex_t *bmutex, pthread_mutex_t *gmutex, sem_t *items, sem_t *free_slots, int *pbindex, int T);

grafo* crea_grafo(const char *infile, int T) {
    FILE *fd = xfopen(infile, "r", QUI);
    if (fd == NULL) xtermina("Errore apertura infile\n", QUI);

    char *line = NULL;
    size_t len = 0;

    // Legge l'intestazione del file
    while (getline(&line, &len, fd) != -1) {
        if (line[0] == '%') continue;
        break;
    }

    int N1 = 0, N2 = 0, num_archi = 0;
    sscanf(line, "%d %d %d", &N1, &N2, &num_archi);
    if (N1 != N2) xtermina("Dimensione non valida", QUI);

    grafo *graph = malloc(sizeof(grafo));
    initialize_graph(graph, N1);

    arco Buffer[BUFFSIZE];
    pthread_mutex_t bmutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t gmutex = PTHREAD_MUTEX_INITIALIZER;
    sem_t items, free_slots;
    xsem_init(&items, 0, 0, QUI);
    xsem_init(&free_slots, 0, BUFFSIZE, QUI);

    int pbindex = 0, cbindex = 0;
    pthread_t threads[T];
    dati_consumatori data[T];

    create_threads(threads, data, graph, Buffer, &bmutex, &gmutex, &items, &free_slots, &cbindex, T);
    read_and_process_arcs(fd, graph, Buffer, threads, &bmutex, &gmutex, &items, &free_slots, &pbindex, T);

    for (int i = 0; i < T; i++) {
        xsem_wait(&free_slots, QUI);
        xpthread_mutex_lock(&bmutex, QUI);
        Buffer[pbindex % BUFFSIZE] = (arco){.from = -1, .to = -1};
        pbindex += 1;
        xpthread_mutex_unlock(&bmutex, QUI);
        xsem_post(&items, QUI);
    }

    join_threads(threads, T);
    destroy_synchronization_objects(&bmutex, &gmutex, &items, &free_slots);
    fclose(fd);
    free(line);

    return graph;
}

void initialize_graph(grafo *graph, int N) {
    graph->N = N;
    graph->out = malloc(N * sizeof(int));
    graph->in = malloc(N * sizeof(inmap));
    for (int i = 0; i < N; i++) {
        graph->out[i] = 0;
        graph->in[i] = NULL;
    }
}

void create_threads(pthread_t *threads, dati_consumatori *data, grafo *graph, arco *Buffer, pthread_mutex_t *bmutex, pthread_mutex_t *gmutex, sem_t *items, sem_t *free_slots, int *cbindex, int T) {
    for (int i = 0; i < T; i++) {
        data[i].bmutex = bmutex;
        data[i].free = free_slots;
        data[i].items = items;
        data[i].buffer = Buffer;
        data[i].cbindex = cbindex;
        data[i].g = graph;
        data[i].gmutex = gmutex;
        xpthread_create(&threads[i], NULL, &tbody_scrittura, &data[i], QUI);
    }
}

void join_threads(pthread_t *threads, int T) {
    for (int i = 0; i < T; i++) {
        xpthread_join(threads[i], NULL, QUI);
    }
}

void destroy_synchronization_objects(pthread_mutex_t *bmutex, pthread_mutex_t *gmutex, sem_t *items, sem_t *free_slots) {
    xpthread_mutex_destroy(bmutex, QUI);
    xpthread_mutex_destroy(gmutex, QUI);
    xsem_destroy(items, QUI);
    xsem_destroy(free_slots, QUI);
}

void handle_invalid_arc(pthread_t *threads, arco *Buffer, pthread_mutex_t *bmutex, pthread_mutex_t *gmutex, sem_t *items, sem_t *free_slots, int *pbindex, int T) {
    arco fine = {.from = -1, .to = -1};
    for (int i = 0; i < T; i++) {
        xsem_wait(free_slots, QUI);
        xpthread_mutex_lock(bmutex, QUI);
        Buffer[*pbindex % BUFFSIZE] = fine;
        (*pbindex)++;
        xpthread_mutex_unlock(bmutex, QUI);
        xsem_post(items, QUI);
    }
    join_threads(threads, T);
    destroy_synchronization_objects(bmutex, gmutex, items, free_slots);
    xtermina("Arco non valido\n", QUI);
}

void read_and_process_arcs(FILE *fd, grafo *graph, arco *Buffer, pthread_t *threads, pthread_mutex_t *bmutex, pthread_mutex_t *gmutex, sem_t *items, sem_t *free_slots, int *pbindex, int T) {
    char *line = NULL;
    size_t len = 0;
    while (getline(&line, &len, fd) != -1) {
        arco arch;
        sscanf(line, "%d %d", &arch.from, &arch.to);

        if (arch.from > graph->N || arch.to > graph->N || arch.from <= 0 || arch.to <= 0) {
            handle_invalid_arc(threads, Buffer, bmutex, gmutex, items, free_slots, pbindex, T);
        }

        arch.from--;
        arch.to--;
        xsem_wait(free_slots, QUI);
        xpthread_mutex_lock(bmutex, QUI);
        Buffer[*pbindex % BUFFSIZE] = arch;
        (*pbindex)++;
        xpthread_mutex_unlock(bmutex, QUI);
        xsem_post(items, QUI);
    }
    free(line);
}
