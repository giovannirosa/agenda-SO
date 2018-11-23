#ifndef LISTA_H
#define LISTA_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

// declara nó da lista
typedef struct noh *noh;
// nó da lista com conteudo, proximo e anterior
struct noh {
	noh prox, ant;
	char *nome, *tel;
};

// declara lista
typedef struct lista *lista;
// lista com inicio, fim e tamanho
struct lista {
	noh ini, fim;
	long tam;
};

// inicia lista alocando memória e iniciando parâmetros
lista iniciaLista(void);

// libera lista da memória, um nó de cada vez, então a lista
void liberaLista(lista l);

// cria noh para inserir na lista
noh criaNoh(char *nome, char *tel);

// insere nó com conteúdo passado no final da lista
noh insereLista(lista l, char *nome, char *tel);

// remove nó do final da lista
void removeLista(lista l);

#endif