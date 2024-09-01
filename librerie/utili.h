#ifndef UTIL_H
#define UTIL_H

#include <pthread.h>
#include <semaphore.h>

#define BUFFSIZE 30

#define TOP_NODES 3
#define MAX_ITERATIONS 100
#define DUMPING 0.9
#define MAX_ERROR 1.e-07
#define THREADS 3

typedef struct arco {
    int from;
    int to;
} arco;

typedef struct inmap {
    int N;
    struct inmap *next;
} inmap;

typedef struct {
    int N;
    int *out;
    inmap **in;
} grafo;

typedef struct {
    pthread_mutex_t *mutex;
    pthread_cond_t *cv;
    int index;
} vector_cond;

typedef struct {
    pthread_mutex_t *mutex;
    pthread_cond_t *cv;
    int aux_index;
} aux;

typedef struct {
    pthread_mutex_t *mutex;
    pthread_cond_t *cv;
    int terminated;
} terminated;

typedef struct {
    grafo *g;
    pthread_mutex_t *bmutex;
    pthread_mutex_t *gmutex;
    sem_t *items;
    sem_t *free;
    int *cbindex;
    arco *buffer;
} dati_consumatori;

typedef struct coppia_indice {
    int indice;
    double rank;
} coppia_indice;

typedef struct {
    grafo *g;
    double *x;
    double *y;
    double *y_aux;
    double *xnext;
    double *St;
    double *St_new;
    double term1;
    double dump;
    int *iter;
    double *errore;
    vector_cond *vector_cond;
    terminated *terminated_cond;
    pthread_mutex_t *aux;
    coppia_indice *massimo;
} dati_calcolatori;

typedef struct {
    coppia_indice *massimo;
    int *iterazione;
    pthread_mutex_t *mutex;
} handler_data;

// Funzioni dichiarate in utili.h
grafo* crea_grafo(const char *, int);
void inserisci(grafo *, arco);
void* tbody_scrittura(void *arg);
double *pagerank(grafo *, double, double, int, int, int *);
void* tbody_calcolo(void *arg);
void nodes_dead_end_valid_arcs(grafo *);
int compare(const void *a, const void *b);
void help();
void deallocate(grafo *);
void *handler_body(void *);

#endif // UTIL_H
