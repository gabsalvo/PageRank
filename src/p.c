#include "../librerie/xerrori.h"
#include "../librerie/utili.h"

// Funzioni ausiliarie per migliorare la leggibilità e la modularizzazione
void initialize_pagerank(grafo *g, double **x, double **y, double **xnext, double **y_aux, double nodes_number);
void create_calculator_threads(pthread_t *threads, dati_calcolatori *data, grafo *g, double *x, double *y, double *xnext, double *y_aux, double *St, double *St_new, double term1, double d, int *iter, double *errore, vector_cond *v_cond, terminated *t_cond, pthread_mutex_t *aux, coppia_indice *massimo_next, int taux);
void join_calculator_threads(pthread_t *threads, int taux);
void cleanup_pagerank(double *x, double *y, double *y_aux, pthread_mutex_t *aux, pthread_mutex_t *t_mutex, pthread_mutex_t *v_mutex, pthread_cond_t *v_cv, pthread_cond_t *t_cv);

double *pagerank(grafo *g, double d, double eps, int maxiter, int taux, int *numiter) {
    fprintf(stderr, "Inizio calcolo pagerank...\n");

    double errore = eps;
    int iter = 0;

    pthread_mutex_t aux = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t t_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t t_cv = PTHREAD_COND_INITIALIZER;

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    pthread_t gestore;
    double nodes_number = (double)g->N;
    double *x, *y, *xnext, *y_aux;

    initialize_pagerank(g, &x, &y, &xnext, &y_aux, nodes_number);

    coppia_indice massimo = { .indice = 0, .rank = 0.0 };
    coppia_indice massimo_next = { .indice = -1, .rank = -1.0 };

    handler_data dati_gestore = { .massimo = &massimo, .iterazione = &iter, .mutex = &t_mutex };
    xpthread_create(&gestore, NULL, handler_body, &dati_gestore, QUI);

    pthread_t threads[taux];
    dati_calcolatori data[taux];

    pthread_mutex_t v_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t v_cv = PTHREAD_COND_INITIALIZER;

    vector_cond v_cond = { .cv = &v_cv, .mutex = &v_mutex, .index = 0 };
    terminated t_cond = { .mutex = &t_mutex, .cv = &t_cv, .terminated = 0 };

    double term1 = (1 - d) / nodes_number;
    double St = 0;
    double St_new = 0;

    create_calculator_threads(threads, data, g, x, y, xnext, y_aux, &St, &St_new, term1, d, &iter, &errore, &v_cond, &t_cond, &aux, &massimo_next, taux);

    do {
        xpthread_mutex_lock(&t_mutex, QUI);
        while (t_cond.terminated != nodes_number) {
            xpthread_cond_wait(&t_cv, &t_mutex, QUI);
        }
        t_cond.terminated = 0;
        massimo = massimo_next;
        massimo_next.rank = -1;
        xpthread_mutex_unlock(&t_mutex, QUI);

        if (errore < eps) {
            xpthread_mutex_lock(&v_mutex, QUI);
            v_cond.index = -1;
            xpthread_cond_signal(&v_cv, QUI);
            xpthread_mutex_unlock(&v_mutex, QUI);
            break;
        }

        if (iter > 0) {
            St = St_new;
            for (int i = 0; i < nodes_number; i++) {
                y[i] = y_aux[i];
                x[i] = xnext[i];
            }
        }

        xpthread_mutex_lock(&v_mutex, QUI);
        while (v_cond.index < nodes_number) {
            xpthread_cond_wait(&v_cv, &v_mutex, QUI);
        }
        St_new = 0;
        errore = 0;
        v_cond.index = 0;
        xpthread_cond_signal(&v_cv, QUI);
        xpthread_mutex_unlock(&v_mutex, QUI);

        iter++;
    } while (iter <= maxiter);

    *numiter = iter;
    v_cond.index = -1;

    join_calculator_threads(threads, taux);
    pthread_kill(gestore, SIGTERM);
    xpthread_join(gestore, NULL, QUI);

    cleanup_pagerank(x, y, y_aux, &aux, &t_mutex, &v_mutex, &v_cv, &t_cv);

    return xnext;
}

void initialize_pagerank(grafo *g, double **x, double **y, double **xnext, double **y_aux, double nodes_number) {
    *x = malloc(nodes_number * sizeof(double));
    *y = malloc(nodes_number * sizeof(double));
    *xnext = malloc(nodes_number * sizeof(double));
    *y_aux = malloc(nodes_number * sizeof(double));
}

void create_calculator_threads(pthread_t *threads, dati_calcolatori *data, grafo *g, double *x, double *y, double *xnext, double *y_aux, double *St, double *St_new, double term1, double d, int *iter, double *errore, vector_cond *v_cond, terminated *t_cond, pthread_mutex_t *aux, coppia_indice *massimo_next, int taux) {
    for (int i = 0; i < taux; i++) {
        data[i] = (dati_calcolatori){
                .g = g, .x = x, .y = y, .St = St, .xnext = xnext,
                .term1 = term1, .dump = d, .iter = iter,
                .errore = errore, .vector_cond = v_cond,
                .y_aux = y_aux, .St_new = St_new, .terminated_cond = t_cond,
                .aux = aux, .massimo = massimo_next
        };
        xpthread_create(&threads[i], NULL, tbody_calcolo, &data[i], QUI);
    }
}

void join_calculator_threads(pthread_t *threads, int taux) {
    for (int i = 0; i < taux; i++) {
        xpthread_join(threads[i], NULL, QUI);
    }
}

void cleanup_pagerank(double *x, double *y, double *y_aux, pthread_mutex_t *aux, pthread_mutex_t *t_mutex, pthread_mutex_t *v_mutex, pthread_cond_t *v_cv, pthread_cond_t *t_cv) {
    free(x);
    free(y);
    free(y_aux);
    xpthread_mutex_destroy(aux, QUI);
    xpthread_mutex_destroy(t_mutex, QUI);
    xpthread_mutex_destroy(v_mutex, QUI);
    xpthread_cond_destroy(v_cv, QUI);
    xpthread_cond_destroy(t_cv, QUI);
}
