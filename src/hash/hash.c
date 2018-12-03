#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "hash.h"

/**
 * Função hash para gerar a chave
 * hashTable: hash
 * key: chave
 */
unsigned int hashCode(hash hashTable, unsigned char key[CRYPTEDSIZE]) {
  unsigned int magic = 0x9f3a5d;

  for(int i = 0; i < CRYPTEDSIZE; ++i) {
      magic = ((magic << 5) - magic) +  key[i];
  }

  magic = magic % hashTable->tam;

  return magic;
}

/**
 * Inicializa hash alocando memória
 * tam: tamanho da hash
 */
hash initHash(unsigned int tam) {
   hash hashTable = (hash)malloc(sizeof(struct hash));
   hashMutex = (sem_t*)malloc(sizeof(sem_t)*tam);
   hashTable->lista = (noh*)malloc(sizeof(struct noh)*tam);

   hashTable->tam = tam;
   for(int i = 0; i < tam; ++i) {
      sem_init(&hashMutex[i], 0, 1);

      hashTable->lista[i] = (noh)malloc(sizeof(struct noh));
      hashTable->lista[i]->ant = NULL;
      hashTable->lista[i]->prox = NULL;
      for(int j = 0; j < CRYPTEDSIZE; ++j) {
         hashTable->lista[i]->chave[j] = '\0';
         hashTable->lista[i]->cont[j] = '\0';
      }
   }
   return hashTable;
}

/**
 * Insere valor na hash
 * hashTable: hash
 * key: chave
 * value: valor a ser inserido
 */
void insertHash(hash hashTable, unsigned char key[CRYPTEDSIZE], unsigned char value[CRYPTEDSIZE]) {
   unsigned int index = hashCode(hashTable, key);
   noh auxNoh = hashTable->lista[index];

   sem_wait(&hashMutex[index]);

   noh newNoh = (noh)malloc(sizeof(struct noh));
   newNoh->ant = NULL;
   newNoh->prox = auxNoh;
   for(int j = 0; j < CRYPTEDSIZE; ++j) {
      newNoh->chave[j] = key[j];
      newNoh->cont[j] = value[j];
   }

   hashTable->lista[index] = newNoh;

   sem_post(&hashMutex[index]);
}

/**
 * Busca na hash valor com chave especificada
 * hashTable: hash
 * chave: chave a ser buscada
 */
unsigned char* searchHash(hash hashTable, unsigned char chave[CRYPTEDSIZE]) {
    unsigned int index = hashCode(hashTable, chave);
    noh auxNoh = hashTable->lista[index];

    sem_wait(&hashMutex[index]);
    while (auxNoh->prox != NULL) {
        if(!memcmp(auxNoh->chave, chave, CRYPTEDSIZE * sizeof(unsigned char))) {
          sem_post(&hashMutex[index]);
          return auxNoh->cont;
        }
        else auxNoh = auxNoh->prox;
    }

    sem_post(&hashMutex[index]);
    return NULL;
}

/**
 * Mostra conteúdo da hash
 * hashTable: hash
 * showValues: mostrar valores ou apenas tamanho utilizado
 */
void displayHash(hash hashTable, int showVaues) {
   noh auxNoh;
   unsigned int i, cont;

   for(i = 0; i < hashTable->tam; ++i) {
      auxNoh = hashTable->lista[i];
      printf("Hash[%d]: ", i);
      cont = 0;

      if(showVaues){
         while (auxNoh->prox != NULL) {
            printf("\n---> key: %s - value: %s\n", auxNoh->chave, auxNoh->cont);
            auxNoh = auxNoh->prox;
         }
      } else {
         while ( auxNoh->prox != NULL ) {
            cont++;
            auxNoh = auxNoh->prox;
         }
         printf("%d\n", cont);
      }
   }
}

/**
 * Libera memória alocada da hash
 * hashTable: hash
 */
void liberaHash(hash hashTable) {
   for(int i = 0; i < hashTable->tam; i++) {
      noh auxNoh = hashTable->lista[i];
      noh proxNoh = auxNoh->prox;
      free(auxNoh);
      while(proxNoh) {
         auxNoh = proxNoh;
         proxNoh = auxNoh->prox;
         free(auxNoh);
      }
   }
   free(hashTable->lista);
   free(hashTable);
   free(hashMutex);
}