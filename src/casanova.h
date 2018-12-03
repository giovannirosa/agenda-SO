#define NOME_SIZE 15 
#define FONE_SIZE 9 
#define ID_SIZE   9
#define PUT_MESSAGE_SIZE (ID_SIZE + NOME_SIZE + FONE_SIZE)
#define GET_MESSAGE_SIZE (ID_SIZE + NOME_SIZE )
#define N_MESSAGE 100
#define SOCK_GET_PATH "server_get_socket"
#define SOCK_PUT_PATH "server_put_socket"
#define DATASET_SIZE 1000*1000*20
#define HASH_SIZE 7000141
#define OUTPUTFILE "telefones"
char charset[] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbcccccccccccccccccccccccccccccccccccddddddddddddddddddddddddddeeeeeeeeeeeeeeeeeeeefffffffffffffffffffffffffffffffffffffgggggggggggggggggggghhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiijjjjjjkkkkkkklllllllllllllllllllllllllllmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmnnnnnnnnnnnnnnnnnnnnnnnoooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooopppppppppppppppppppppppppqqrrrrrrrrrrrrrrrrsssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttuuuuuuuuuuuuuuvvvvvvwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwxyyyyyyyyyyyyyyyyz                                                    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBCCCCCCCCCCCCCCCCCCCCCCCCCDDDDDDDDDDDDDDDDDDDDDDDDDDEEEEEEEEEEEEEEEEEEEEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFGGGGGGGGGGGGGGGGGGHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIJJJJJJJJJKKKKLLLLLLLLLLLLLLLLLLLLLLLLMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMNNNNNNNNNNNNNNNNNNNNNNNNOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOPPPPPPPPPPPPPPPPPPQQQQRRRRRRRRRRRRRRRRRRRRRRRRSSSSSSSSSSSSSSSSSSSSSSSSSSSTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYZZ                  ";
#define N_PUT_THREADS 2
#define N_GET_THREADS 3

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


typedef struct getThread{
  pthread_t thread; // thread rodando get
  sem_t sem;  // semáforo
  int busy; //  se está sendo usada
  int m_avail;  // numero de mensagens inteiras
  char actual_get[ID_SIZE];
  sem_t sem_getahead;
  int waitting;
  char buffer[GET_MESSAGE_SIZE*273]; // mensagem
} getThread;


typedef struct putThread{
  pthread_t thread;
  sem_t sem;
  int busy;
  int m_avail;
  char actual_put[ID_SIZE];
  char buffer[PUT_MESSAGE_SIZE*170]; // mensagem
} putThread;

getThread getThreads[N_GET_THREADS];
putThread putThreads[N_PUT_THREADS];

int getOver; // sinaliza se GET finalizou
int putOver; // sinaliza se PUT finalizou

sem_t mutexWakeUp;
sem_t putThreadsUsed;
sem_t getThreadsUsed;
sem_t mutexOutput;

FILE *fp;

void bestPossiblePut(char *bestPut);
void wakeupGets(void);
void put_entries();
void store(putThread *self);
void get_entries();
void retrieve(getThread *self);
