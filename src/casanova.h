#define NOME_SIZE 15 
#define FONE_SIZE 9 
#define ID_SIZE	9
#define PUT_MESSAGE_SIZE (ID_SIZE + NOME_SIZE + FONE_SIZE)
#define GET_MESSAGE_SIZE (ID_SIZE + NOME_SIZE )
#define N_MESSAGE 100
#define SOCK_GET_PATH "server_get_socket"
#define SOCK_PUT_PATH "server_put_socket"
#define DATASET_SIZE 1000*1000*20
#define HASH_SIZE 7000141
#define OUTPUTFILE "telefones"
char charset[] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbcccccccccccccccccccccccccccccccccccddddddddddddddddddddddddddeeeeeeeeeeeeeeeeeeeefffffffffffffffffffffffffffffffffffffgggggggggggggggggggghhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiijjjjjjkkkkkkklllllllllllllllllllllllllllmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmnnnnnnnnnnnnnnnnnnnnnnnoooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooopppppppppppppppppppppppppqqrrrrrrrrrrrrrrrrsssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttuuuuuuuuuuuuuuvvvvvvwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwxyyyyyyyyyyyyyyyyz													AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBCCCCCCCCCCCCCCCCCCCCCCCCCDDDDDDDDDDDDDDDDDDDDDDDDDDEEEEEEEEEEEEEEEEEEEEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFGGGGGGGGGGGGGGGGGGHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIJJJJJJJJJKKKKLLLLLLLLLLLLLLLLLLLLLLLLMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMNNNNNNNNNNNNNNNNNNNNNNNNOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOPPPPPPPPPPPPPPPPPPQQQQRRRRRRRRRRRRRRRRRRRRRRRRSSSSSSSSSSSSSSSSSSSSSSSSSSSTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYZZ				 ";
#define N_PUT_THREADS 2
#define N_GET_THREADS 4

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>	
#include <semaphore.h>
#include <sys/ioctl.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/engine.h>

#include "locking/locking.h"
#include "hash/hash.h"

/* Chave utilizada, obviamente não é seguro coloca-la aqui em
	plain text, mas isso é apenas um toy */
unsigned char *key = (unsigned char *)"01234567890123456789012345678901";
extern int encrypte(EVP_CIPHER_CTX *, unsigned char *, int , unsigned char *);
extern int decrypt(EVP_CIPHER_CTX *, unsigned char *, int , unsigned char *);

typedef struct getThread getThread;
typedef struct putThread putThread;

// estrutura de dados para thread GET
struct getThread{
  pthread_t thread; // thread rodando GET
  sem_t sem;  // semáforo
  int busy; // se está sendo usada
  int m_avail;  // numero de mensagens inteiras
  char actual_get[ID_SIZE]; // relógio lógico da thread atual
  sem_t sem_getahead; // sinaliza se GET está a frente do PUT
  int waitting; // se está esperando PUT
  char buffer[GET_MESSAGE_SIZE*273]; // mensagens
};


// estrutura de dados para thread PUT
struct putThread{
  pthread_t thread; // thread rodando PUT
  sem_t sem; // semáforo
  int busy; //  se está sendo usada
  int m_avail; // numero de mensagens inteiras
  char actual_put[ID_SIZE]; // relógio lógico da thread atual
  char buffer[PUT_MESSAGE_SIZE*170]; // mensagens
};

getThread getThreads[N_GET_THREADS]; // pool de threads GET
putThread putThreads[N_PUT_THREADS]; // pool de threads PUT

int getOver; // sinaliza se GET finalizou
int putOver; // sinaliza se PUT finalizou

sem_t mutexWakeUp; // mutex para acordar GETs que estejam dormindo
sem_t putThreadsUsed; // semáforo para controlar a pool de threads PUT
sem_t getThreadsUsed; // semáforo para controlar a pool de threads GET
sem_t mutexOutput; // mutex para escrever no arquivo de saída

FILE *fp; // ponteiro para arquivo de saída

/**
 * Encontra qual o último PUT feito verificando o relógio lógico de cada thread
 * lastPut: relógio lógico do último PUT
 */
void findLastPut(char *bestPut);

/**
 * Acorda threads GET que estejam eventualmente dormindo
 */
void wakeupGets();

/**
 * Função inicial PUT, inicia a pool de threads, le dados do socket e 
 * gerencia as threads para inserir na hash 
 */
void put_entries();

/**
 * Faz a inserção dos dados na hash
 * self: thread atual PUT
 */
void store(putThread *self);

/**
 * Função inicial GET, inicia a pool de threads, le dados do socket e 
 * gerencia as threads para buscar na hash 
 */
void get_entries();

/**
 * Faz a busca dos dados na hash e escreve no arquivo de saída
 * self: thread atual GET
 */
void retrieve(getThread *self);
