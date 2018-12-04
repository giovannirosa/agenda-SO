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
unsigned int hash_table_index(hash_table_t hashTable, unsigned char key[CRYPTEDSIZE]) {
	unsigned int magic = 0x9f3a5d;

	for(int i = 0; i < CRYPTEDSIZE; ++i) {
		magic = ((magic << 5) - magic) +	key[i];
	}

	magic = magic % hashTable->size;

	return magic;
}

/**
 * Inicializa hash alocando memória
 * tam: tamanho da hash
 */
hash_table_t hash_table_malloc(unsigned int size) {
	hash_table_t hashTable = (hash_table_t)malloc(sizeof(struct hash_table_t));
	hash_table_mutex = (sem_t*)malloc(sizeof(sem_t)*size);
	hashTable->list = (linked_list_t*)malloc(sizeof(struct linked_list_t)*size);

	hashTable->size = size;
	for(int i = 0; i < size; ++i) {
		sem_init(&hash_table_mutex[i], 0, 1);

		hashTable->list[i] = (linked_list_t)malloc(sizeof(struct linked_list_t));
		hashTable->list[i]->previous = NULL;
		hashTable->list[i]->next = NULL;
		for(int j = 0; j < CRYPTEDSIZE; ++j) {
			hashTable->list[i]->key[j] = '\0';
			hashTable->list[i]->value[j] = '\0';
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
void hash_table_put(hash_table_t hashTable, unsigned char key[CRYPTEDSIZE], unsigned char value[CRYPTEDSIZE]) {
	unsigned int index = hash_table_index(hashTable, key);
	linked_list_t auxNoh = hashTable->list[index];

	sem_wait(&hash_table_mutex[index]);

	linked_list_t newNoh = (linked_list_t)malloc(sizeof(struct linked_list_t));
	newNoh->previous = NULL;
	newNoh->next = auxNoh;
	for(int j = 0; j < CRYPTEDSIZE; ++j) {
		newNoh->key[j] = key[j];
		newNoh->value[j] = value[j];
	}

	hashTable->list[index] = newNoh;

	sem_post(&hash_table_mutex[index]);
}


/**
 * Busca na hash valor com chave especificada
 * hashTable: hash
 * chave: chave a ser buscada
 */
unsigned char* hash_table_get(hash_table_t hashTable, unsigned char key[CRYPTEDSIZE]) {
	unsigned int index = hash_table_index(hashTable, key);
	linked_list_t auxNoh = hashTable->list[index];

	sem_wait(&hash_table_mutex[index]);
	while (auxNoh->next != NULL) {
		 if(!memcmp(auxNoh->key, key, CRYPTEDSIZE * sizeof(unsigned char))) {
			sem_post(&hash_table_mutex[index]);
			return auxNoh->value;
		 }
		 else auxNoh = auxNoh->next;
	}
	
	sem_post(&hash_table_mutex[index]);
	return NULL;
}

/**
 * Mostra conteúdo da hash
 * hashTable: hash
 * showValues: mostrar valores ou apenas tamanho utilizado
 */
void hash_table_print(hash_table_t hashTable, int showVaues) {
	linked_list_t auxNoh;
	unsigned int i, cont;

	for(i = 0; i < hashTable->size; ++i) {
		auxNoh = hashTable->list[i];
		printf("Hash[%d]: ", i);
		cont = 0;

		if(showVaues){
			while (auxNoh->next != NULL) {
				printf("\n---> key: %s - value: %s\n", auxNoh->key, auxNoh->value);
				auxNoh = auxNoh->next;
			}
		} else {
			while ( auxNoh->next != NULL ) {
				cont++;
				auxNoh = auxNoh->next;
			}
			printf("%d\n", cont);
		}
	}
}

/**
 * Libera memória alocada da hash
 * hashTable: hash
 */
void hash_table_free(hash_table_t hashTable) {
	for(int i = 0; i < hashTable->size; i++) {
		linked_list_t auxNoh = hashTable->list[i];
		linked_list_t nextNoh = auxNoh->next;
		free(auxNoh);
		while(nextNoh) {
			auxNoh = nextNoh;
			nextNoh = auxNoh->next;
			free(auxNoh);
		}
	}
	free(hashTable->list);
	free(hashTable);
	free(hash_table_mutex);
}
