#ifndef HASH_H
#define HASH_H

#define SIZE 1000*1000*20
/* as entradas são pequenas, o resultado da criptografia terá 
   sempre 16 bytes */
#define CRYPTEDSIZE	16
#define FALSE 0
#define TRUE 1

#include <semaphore.h>

typedef struct linked_list_t *linked_list_t;
typedef struct hash_table_t *hash_table_t;

struct linked_list_t {
	unsigned char key[CRYPTEDSIZE]; // chave da tabela hash
	unsigned char value[CRYPTEDSIZE]; // conteúdo da tabela hash
	linked_list_t previous, next; // lista duplamente encadeada
};

struct hash_table_t {
	unsigned int size; // tamanho da hash
	linked_list_t *list; // posições da hash
};

sem_t *hash_table_mutex; // vetor de mutex para cada posição da hash

/**
 * Função hash para gerar a chave
 * hashTable: hash
 * key: chave
 */
unsigned int hash_table_index(hash_table_t hashTable, unsigned char key[CRYPTEDSIZE]);


/**
 * Inicializa hash alocando memória
 * tam: tamanho da hash
 */
hash_table_t hash_table_malloc(unsigned int size);


/**
 * Insere valor na hash
 * hashTable: hash
 * key: chave
 * value: valor a ser inserido
 */
void hash_table_put(hash_table_t hashTable, unsigned char key[CRYPTEDSIZE], unsigned char value[CRYPTEDSIZE]);


/**
 * Busca na hash valor com chave especificada
 * hashTable: hash
 * chave: chave a ser buscada
 */
unsigned char* hash_table_get(hash_table_t hashTable, unsigned char key[CRYPTEDSIZE]);


/**
 * Mostra conteúdo da hash
 * hashTable: hash
 * showValues: mostrar valores ou apenas tamanho utilizado
 */
void hash_table_print(hash_table_t hashTable, int showVaues);

/**
 * Libera memória alocada da hash
 * hashTable: hash
 */
void hash_table_free(hash_table_t hashTable);

#endif
