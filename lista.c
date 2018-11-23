#include "lista.h"

// inicia lista alocando memória e iniciando parâmetros
lista iniciaLista(void) {
  lista l = malloc(sizeof(struct lista));

  l->tam = 0;
  l->ini = NULL;
  l->fim = NULL;

  return l;
}

// libera lista da memória, um nó de cada vez, então a lista
void liberaLista(lista l) { 
  noh aux;
  while((aux = l->fim)) {
    l->fim = aux->ant;
    free(aux);
  }
  free(l);
}

// cria noh para inserir na lista
noh criaNoh(char *nome, char *tel) {
  noh aux = malloc(sizeof(struct noh));
  aux->nome = nome;
  aux->tel = tel;
  aux->ant = NULL;
  aux->prox = NULL;

  return aux;
}

// insere nó com conteúdo passado no final da lista
noh insereLista(lista l, char *nome, char *tel) {
  noh n = criaNoh(nome,tel);

  if (l->tam == 0) {
    l->ini = n;
    l->fim = n;
  } else {
    l->fim->prox = n;
    n->ant = l->fim;
    l->fim = n;
  }
  l->tam++;

  return n;
}

// remove nó do final da lista
void removeLista(lista l) {
  noh aux = l->fim;
  if (!aux)
    return;

  noh ant = aux->ant;
  if (ant) {
    l->fim = ant;
    ant->prox = NULL;
  } else {
    l->ini = NULL;
    l->fim = NULL;
  }

  l->tam--;
  free(aux);
}

