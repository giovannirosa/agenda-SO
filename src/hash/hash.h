#ifndef HASH_H
#define HASH_H

#define SIZE 1000*1000*20
/* as entradas são pequenas, o resultado da criptografia terá 
   sempre 16 bytes */
#define CRYPTEDSIZE  16
#define FALSE 0
#define TRUE 1

#include <semaphore.h>

typedef struct noh *noh;
typedef struct hash *hash;

struct noh {
   unsigned char chave[CRYPTEDSIZE];
   unsigned char cont[CRYPTEDSIZE];
   noh ant, prox;
};

struct hash {
   unsigned int tam;
   noh *lista;
};

sem_t *hashMutex;

unsigned int hashCode(hash hashTable, unsigned char key[CRYPTEDSIZE]);
hash initHash(unsigned int tam);
void insertHash(hash hashTable, unsigned char key[CRYPTEDSIZE], unsigned char value[CRYPTEDSIZE]);
unsigned char* searchHash(hash hashTable, unsigned char chave[CRYPTEDSIZE]);
void displayHash(hash hashTable, int showVaues);
void liberaHash(hash hashTable);

#endif