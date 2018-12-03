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
   unsigned char chave[CRYPTEDSIZE]; // chave da hash
   unsigned char cont[CRYPTEDSIZE]; // conteúdo da hash
   noh ant, prox; // lista duplamente encadeada
};

struct hash {
   unsigned int tam; // tamanho da hash
   noh *lista; // posições da hash
};

sem_t *hashMutex; // vetor de mutex para cada posição da hash

/**
 * Função hash para gerar a chave
 * hashTable: hash
 * key: chave
 */
unsigned int hashCode(hash hashTable, unsigned char key[CRYPTEDSIZE]);

/**
 * Inicializa hash alocando memória
 * tam: tamanho da hash
 */
hash initHash(unsigned int tam);

/**
 * Insere valor na hash
 * hashTable: hash
 * key: chave
 * value: valor a ser inserido
 */
void insertHash(hash hashTable, unsigned char key[CRYPTEDSIZE], unsigned char value[CRYPTEDSIZE]);

/**
 * Busca na hash valor com chave especificada
 * hashTable: hash
 * chave: chave a ser buscada
 */
unsigned char* searchHash(hash hashTable, unsigned char chave[CRYPTEDSIZE]);

/**
 * Mostra conteúdo da hash
 * hashTable: hash
 * showValues: mostrar valores ou apenas tamanho utilizado
 */
void displayHash(hash hashTable, int showVaues);

/**
 * Libera memória alocada da hash
 * hashTable: hash
 */
void liberaHash(hash hashTable);

#endif