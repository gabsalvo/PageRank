#include "../librerie/xerrori.h"
#include "../librerie/utili.h"

// Funzioni ausiliarie per migliorare la leggibilità del main
void parse_arguments(int argc, char *argv[], int *K, int *M, double *D, double *E, int *T);
void print_help_and_exit();
void print_pagerank_results(grafo *graph, double *vector, int numit, int M, int K);

int main(int argc, char *argv[]) {
    int K = TOP_NODES;
    int M = MAX_ITERATIONS;
    double D = DUMPING;
    double E = MAX_ERROR;
    int T = THREADS;

    // Parsing degli argomenti della linea di comando
    parse_arguments(argc, argv, &K, &M, &D, &E, &T);

    if (optind + 1 != argc) {
        print_help_and_exit();
    }

    // Creazione del grafo
    grafo *graph = crea_grafo(argv[argc - 1], T);
    nodes_dead_end_valid_arcs(graph);

    // Calcolo del PageRank
    int numit;
    double *vector = pagerank(graph, D, E, M, T, &numit);

    // Stampa dei risultati
    print_pagerank_results(graph, vector, numit, M, K);

    // Deallocazione delle risorse
    deallocate(graph);
    free(vector);

    return 0;
}

void parse_arguments(int argc, char *argv[], int *K, int *M, double *D, double *E, int *T) {
    int opt;
    while ((opt = getopt(argc, argv, "k:m:d:e:t:")) != -1) {
        switch (opt) {
            case 'k':
                *K = atoi(optarg);
                break;
            case 'm':
                *M = atoi(optarg);
                break;
            case 'd':
                *D = atof(optarg);
                break;
            case 'e':
                *E = atof(optarg);
                break;
            case 't':
                *T = atoi(optarg);
                break;
            default:
                print_help_and_exit();
        }
    }
}

void print_help_and_exit() {
    help();
    exit(1);
}

void print_pagerank_results(grafo *graph, double *vector, int numit, int M, int K) {
    double sum = 0;
    for (int i = 0; i < graph->N; i++) {
        sum += vector[i];
    }

    if (numit < M) {
        printf("Converged after %d iterations\n", numit);
    } else {
        printf("Did not converge after %d iterations\n", M);
    }

    printf("Sum of ranks: %.4f   (should be 1)\n", sum);

    coppia_indice *vector_index = malloc(sizeof(coppia_indice) * graph->N);
    for (int i = 0; i < graph->N; i++) {
        vector_index[i].indice = i;
        vector_index[i].rank = vector[i];
    }

    qsort(vector_index, graph->N, sizeof(coppia_indice), compare);

    printf("Top %d nodes:\n", K);
    for (int i = 0; i < K; i++) {
        printf("  %d %f\n", vector_index[i].indice, vector_index[i].rank);
    }

    free(vector_index);
}
