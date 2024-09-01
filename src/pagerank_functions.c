#include "../librerie/pagerank_functions.h"

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
