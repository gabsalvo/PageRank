#ifndef PAGERANK_FUNCTIONS_H
#define PAGERANK_FUNCTIONS_H

#include "../librerie/xerrori.h"
#include "../librerie/utili.h"

// Dichiarazioni delle funzioni ausiliarie
void initialize_pagerank(grafo *g, double **x, double **y, double **xnext, double **y_aux, double nodes_number);
void create_calculator_threads(pthread_t *threads, dati_calcolatori *data, grafo *g, double *x, double *y, double *xnext, double *y_aux, double *St, double *St_new, double term1, double d, int *iter, double *errore, vector_cond *v_cond, terminated *t_cond, pthread_mutex_t *aux, coppia_indice *massimo_next, int taux);
void join_calculator_threads(pthread_t *threads, int taux);
void cleanup_pagerank(double *x, double *y, double *y_aux, pthread_mutex_t *aux, pthread_mutex_t *t_mutex, pthread_mutex_t *v_mutex, pthread_cond_t *v_cv, pthread_cond_t *t_cv);

#endif // PAGERANK_FUNCTIONS_H
