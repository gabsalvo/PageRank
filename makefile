CC = gcc
CFLAGS = -std=c11 -Wall -g -O -pthread
LIBS = -lm -lrt -pthread

SRCS = src/main.c src/graph_gen.c src/pagerank.c src/graph_functions.c src/pagerank_functions.c src/xerrori.c
OBJS = src/main.o src/graph_gen.o src/pagerank.o src/graph_functions.o src/pagerank_functions.o src/xerrori.o

all: pagerank

# Regola per creare l'eseguibile
pagerank: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

# Regola per creare i file oggetto
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Regola per pulire i file generati
clean:
	rm -f $(OBJS) pagerank
