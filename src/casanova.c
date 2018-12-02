#include "casanova.h"

void handleErrors(void)
{
  ERR_print_errors_fp(stderr);
}

void get_entries();
void put_entries();

// GHashTable* hash;
// struct DataItem* hashTable[SIZE];

sem_t sem_mutex_crypt;
sem_t sem_mutex_hash;
sem_t mutex_actual;
sem_t sem_getahead;

char actual_put[ID_SIZE];
char actual_get[ID_SIZE];
int getahead;

EVP_CIPHER_CTX *ctxp, *ctxge, *ctxgd;

hash hashTable;

int main(int argc, char *argv[])
{

  hashTable = initHash(HASH_SIZE);
  sem_init(&mutex_wakeup, 0, 1);
  /* inicialização */
  ERR_load_crypto_strings();
  OpenSSL_add_all_algorithms();
  OPENSSL_config(NULL);
  if (!(ctxp = EVP_CIPHER_CTX_new()))
    handleErrors();
  if (!(ctxge = EVP_CIPHER_CTX_new()))
    handleErrors();
  if (!(ctxgd = EVP_CIPHER_CTX_new()))
    handleErrors();

  pthread_t thread1, thread2;
  thread_setup();
  /* Cria um thread para a entrada e outra para a saída */
  pthread_create(&thread1, NULL, (void *)&put_entries, (void *)0);
  pthread_create(&thread2, NULL, (void *)&get_entries, (void *)0);

  /* espero a morte das threads */
  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);

  fprintf(stdout, "Cleaning up\n");
  /* limpeza geral, hash table, semáforos e biblioteca de criptografia */
  thread_cleanup();
  liberaHash(hashTable);

  FIPS_mode_set(0);
  ENGINE_cleanup();
  CONF_modules_unload(1);
  EVP_cleanup();
  CRYPTO_cleanup_all_ex_data();
  ERR_remove_state(0);
  ERR_free_strings();

  return 0;
}

void bestPossiblePut(char *bestPut)
{
  int i, count = 0;

  memset(bestPut, 0xFF, ID_SIZE - 1);

  for (i = 0; i < N_PUT_THREADS; ++i)
  {
    if (strncmp(bestPut, put_threads[i].actual_put, ID_SIZE - 1) > 0)
    {
      memcpy(bestPut, put_threads[i].actual_put, ID_SIZE - 1);
      count++;
    }
  }
}

void wakeupGets(void)
{
  int i;
  char bestPut[ID_SIZE - 1];
  bestPossiblePut(bestPut);

  sem_wait(&mutex_wakeup);
  for (i = 0; i < N_GET_THREADS; ++i)
  {
    if ((strncmp(get_threads[i].actual_get, bestPut, ID_SIZE - 1) <= 0) && (get_threads[i].waitting))
    {
      get_threads[i].waitting = FALSE;
      sem_post(&get_threads[i].sem_getahead);
    }
  }
  sem_post(&mutex_wakeup);
}

void put_entries()
{
  int server_sockfd, client_sockfd;
  struct sockaddr_un server_address;
  struct sockaddr_un client_address;
  socklen_t addr_size;
  tele_thread_put *actual_thread = NULL;
  putOver = FALSE;

  int i, count = 0, bytesrw = 0;
  int read_ret, read_total, m_avail;

  for (i = 0; i < N_PUT_THREADS; ++i)
  {
    put_threads[i].buffer[PUT_MESSAGE_SIZE] = '\0';
  }

  /* inicializa SOCK_STREAM */
  unlink(SOCK_PUT_PATH);
  server_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
  server_address.sun_family = AF_UNIX;
  strcpy(server_address.sun_path, SOCK_PUT_PATH);
  bind(server_sockfd, (struct sockaddr *)&server_address, sizeof(server_address));

  /* aguarda conexão */
  listen(server_sockfd, 5);

  fprintf(stderr, "PUT WAITTING\n");
  addr_size = sizeof(client_address);
  client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &addr_size);
  fprintf(stderr, "PUT CONNECTED\n");

  // INICIALIZA THREADS
  sem_init(&sem_all_put_threads_busy, 0, N_PUT_THREADS);
  pthread_attr_t attr;
  cpu_set_t cpus;
  pthread_attr_init(&attr);
  int numberOfProcessors = sysconf(_SC_NPROCESSORS_ONLN);

  for (i = 0; i < N_PUT_THREADS; ++i)
  {
    put_threads[i].busy = FALSE;
    sem_init(&put_threads[i].sem, 0, 0);
    memset(put_threads[i].actual_put, 0, ID_SIZE - 1);
    CPU_ZERO(&cpus);
    CPU_SET((i + (int)(numberOfProcessors * 0.5)) % numberOfProcessors, &cpus);
    pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
    pthread_create(&put_threads[i].thread, &attr, (void *)&store, &put_threads[i]);
  }

  do
  {
    sem_wait(&sem_all_put_threads_busy);
    for (i = 0; i < N_PUT_THREADS; ++i)
    {
      if (!put_threads[i].busy)
      {
        actual_thread = &put_threads[i];
      }
    }

    /* numero de mensagens inteiras no put_buffer */
    ioctl(client_sockfd, FIONREAD, &bytesrw);
    m_avail = bytesrw / PUT_MESSAGE_SIZE;
    if (m_avail == 0)
      m_avail = 1;
    if (m_avail > 170)
      m_avail = 170;

    /* le o número de mensagens escolhido */
    read_total = 0;
    do
    {
      read_ret = read(client_sockfd, actual_thread->buffer, PUT_MESSAGE_SIZE * m_avail - read_total);
      read_total += read_ret;
    } while (read_total < PUT_MESSAGE_SIZE * m_avail && read_ret > 0);

    if (read_ret <= 0)
      m_avail = 0;

    /* armazena todas as entradas lidas */
    if (read_ret > 0)
    {
      actual_thread->m_avail = m_avail;
      actual_thread->busy = TRUE;
      sem_post(&actual_thread->sem);
    }

    count += m_avail;
  } while (read_ret > 0);

  close(client_sockfd);

  putOver = TRUE;

  for (i = 0; i < N_PUT_THREADS; ++i)
  {
    memset(put_threads[i].actual_put, 0xFF, ID_SIZE - 1);
  }

  for (i = 0; i < N_GET_THREADS; ++i)
  {
    get_threads[i].waitting = FALSE;
    sem_post(&get_threads[i].sem_getahead);
  }
  //Mata todas as threads
  // int threads_busy;
  for (i = 0; i < N_PUT_THREADS; ++i)
  {
    sem_post(&put_threads[i].sem);
    pthread_join(put_threads[i].thread, NULL);
  }

  fprintf(stderr, "PUT EXITED, %d MESSAGES RECEIVED \n", count);
}

void store(tele_thread_put *self)
{
  unsigned char telefone_crypt[CRYPTEDSIZE];
  unsigned char nome_crypt[CRYPTEDSIZE];
  EVP_CIPHER_CTX *ctx_crypt;
  unsigned int n;

  if (!(ctx_crypt = EVP_CIPHER_CTX_new()))
    handleErrors();

  while (!putOver)
  {
    sem_wait(&self->sem);
    if (putOver)
      return;

    for (n = 0; n < self->m_avail; n++)
    {
      encrypt2(ctx_crypt, (unsigned char *)self->buffer + (n * PUT_MESSAGE_SIZE) + ID_SIZE, NOME_SIZE, nome_crypt);
      encrypt2(ctx_crypt, (unsigned char *)(self->buffer + (n * PUT_MESSAGE_SIZE) + ID_SIZE + NOME_SIZE), FONE_SIZE, telefone_crypt);

      //Verifica se por algum motivo já foi inserido o dado
      unsigned char *telefone_hash = searchHash(hashTable, nome_crypt);

      if (telefone_hash)
      {
        fprintf(stderr, "ERRO: %.16s:", self->buffer + (n * PUT_MESSAGE_SIZE) + ID_SIZE);
        BIO_dump_fp(stderr, (const char *)nome_crypt, CRYPTEDSIZE);
        continue;
      }
      insertHash(hashTable, nome_crypt, telefone_crypt);

      memcpy(self->actual_put, self->buffer + n * PUT_MESSAGE_SIZE, ID_SIZE - 1);
    }

    wakeupGets();

    self->busy = FALSE;
    sem_post(&sem_all_put_threads_busy);
  }

  EVP_CIPHER_CTX_free(ctx_crypt);
}

void get_entries()
{
  int server_sockfd, client_sockfd, i;
  struct sockaddr_un server_address;
  struct sockaddr_un client_address;
  socklen_t addr_size;
  tele_thread_get *actual_thread = NULL;
  getOver = FALSE;

  int count = 0, read_ret, read_total, m_avail, bytesrw;

  /* inicializa SOCK_STREAM */
  unlink(SOCK_GET_PATH);
  server_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
  server_address.sun_family = AF_UNIX;
  strcpy(server_address.sun_path, SOCK_GET_PATH);
  bind(server_sockfd, (struct sockaddr *)&server_address, sizeof(server_address));
  listen(server_sockfd, 5);

  fprintf(stderr, "GET WAITTING\n");
  addr_size = sizeof(client_address);
  client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &addr_size);
  fprintf(stderr, "GET CONNECTED\n");
  fp = fopen(OUTPUTFILE, "w+");

  if (fp == NULL)
  {
    perror("OPEN:");
    exit(1);
  }

  // INICIALIZA THREADS
  sem_init(&sem_all_get_threads_busy, 0, N_GET_THREADS);
  sem_init(&mutex_file, 0, 1);
  pthread_attr_t attr;
  cpu_set_t cpus;
  pthread_attr_init(&attr);
  int numberOfProcessors = sysconf(_SC_NPROCESSORS_ONLN);

  for (i = 0; i < N_GET_THREADS; ++i)
  {
    get_threads[i].busy = FALSE;
    sem_init(&get_threads[i].sem, 0, 0);
    sem_init(&get_threads[i].sem_getahead, 0, 0);
    memset(get_threads[i].actual_get, 0, ID_SIZE - 1);
    get_threads[i].waitting = FALSE;
    CPU_ZERO(&cpus);
    CPU_SET(i % numberOfProcessors, &cpus);
    pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
    pthread_create(&get_threads[i].thread, &attr, (void *)&retrieve, &get_threads[i]);
  }

  do
  {
    sem_wait(&sem_all_get_threads_busy);
    for (i = 0; i < N_GET_THREADS; ++i)
    {
      if (!get_threads[i].busy)
      {
        actual_thread = &get_threads[i];
      }
    }

    /* vejo quantidade de bytes no buffer */
    ioctl(client_sockfd, FIONREAD, &bytesrw);

    /* numero de mensagens inteiras no buffer */
    m_avail = bytesrw / GET_MESSAGE_SIZE;
    if (m_avail == 0)
      m_avail = 1;
    if (m_avail > 273)
      m_avail = 273;

    read_total = 0;
    do
    {
      read_ret = read(client_sockfd, actual_thread->buffer, GET_MESSAGE_SIZE * m_avail - read_total);
      read_total += read_ret;
    } while (read_total < GET_MESSAGE_SIZE * m_avail && read_ret > 0);

    if (read_ret <= 0)
    {
      m_avail = 0;
    }

    if (read_ret > 0)
    {
      actual_thread->m_avail = m_avail;
      actual_thread->busy = TRUE;

      sem_post(&actual_thread->sem);
    }

    count += m_avail;
  } while (read_ret > 0);

  close(client_sockfd);
  getOver = TRUE;
  //Mata todas as threads
  for (i = 0; i < N_GET_THREADS; ++i)
  {
    sem_post(&get_threads[i].sem);
    pthread_join(get_threads[i].thread, NULL);
  }
  fclose(fp);
  fprintf(stderr, "GET EXITED, %d MESSAGES RECEIVED\n", count);
}

void retrieve(tele_thread_get *self)
{
  int telefoneint, n;
  unsigned char *telefone_crypt;
  unsigned char telefone_decrypt[FONE_SIZE + 1];
  unsigned char nome_crypt[CRYPTEDSIZE + 1];
  EVP_CIPHER_CTX *ctx_crypt, *ctx_decrypt;
  char actual_put[ID_SIZE];

  telefone_decrypt[FONE_SIZE] = '\0';
  nome_crypt[CRYPTEDSIZE] = '\0';

  if (!(ctx_crypt = EVP_CIPHER_CTX_new()))
    handleErrors();
  if (!(ctx_decrypt = EVP_CIPHER_CTX_new()))
    handleErrors();

  while (!getOver)
  {
    sem_wait(&self->sem);
    if (getOver)
      return;

    for (n = 0; n < self->m_avail; n++)
    {
      memcpy(self->actual_get, self->buffer + n * GET_MESSAGE_SIZE, ID_SIZE - 1);

      bestPossiblePut(actual_put);
      if (strncmp(self->actual_get, actual_put, ID_SIZE - 1) > 0)
      {
        self->waitting = TRUE;
        sem_wait(&self->sem_getahead);
      }

      encrypt2(ctx_crypt, (unsigned char *)self->buffer + (n * GET_MESSAGE_SIZE) + ID_SIZE, NOME_SIZE, nome_crypt);

      //Verifica se por algum motivo já foi inserido o dado
      telefone_crypt = searchHash(hashTable, nome_crypt);

      if (!telefone_crypt)
      {
        BIO_dump_fp(stdout, (const char *)nome_crypt, CRYPTEDSIZE);
        continue;
      }
      else
      {
        decrypt(ctx_decrypt, telefone_crypt, CRYPTEDSIZE, telefone_decrypt);
        telefone_decrypt[FONE_SIZE] = '\0';
        /* se quiser ver as entradas descomente */
        telefoneint = atoi((char *)telefone_decrypt);
        sem_wait(&mutex_file);
        fwrite((void *)&telefoneint, sizeof(int), 1, fp);
        sem_post(&mutex_file);
      }
    }

    self->busy = FALSE;
    sem_post(&sem_all_get_threads_busy);
  }

  EVP_CIPHER_CTX_free(ctx_crypt);
  EVP_CIPHER_CTX_free(ctx_decrypt);
}

/* seguem as funções de criptografia. Eu estou usando elas no sabor não
   threadsafe, por isso o mutex. Na versão paralela é de se cogitar o
   uso thread safe desta mesma biblioteca */
inline int encrypt2(EVP_CIPHER_CTX *ctxe, unsigned char *plaintext, int plaintext_len, unsigned char *ciphertext)
{

  int len;
  int ciphertext_len;

  if (1 != EVP_EncryptInit_ex(ctxe, EVP_aes_256_ecb(), NULL, key, NULL))
    handleErrors();
  //  EVP_CIPHER_CTX_set_padding(ctxe, 1);
  if (1 != EVP_EncryptUpdate(ctxe, ciphertext, &len, plaintext, plaintext_len))
    handleErrors();
  ciphertext_len = len;
  if (1 != EVP_EncryptFinal_ex(ctxe, ciphertext + len, &len))
    handleErrors();
  ciphertext_len += len;

  return ciphertext_len;
}

inline int decrypt(EVP_CIPHER_CTX *ctxd, unsigned char *ciphertext, int ciphertext_len, unsigned char *plaintext)
{
  int plaintext_len, len;

  if (1 != EVP_DecryptInit_ex(ctxd, EVP_aes_256_ecb(), NULL, key, NULL))
    handleErrors();
  //  EVP_CIPHER_CTX_set_padding(ctxd, 0);
  if (1 != EVP_DecryptUpdate(ctxd, plaintext, &len, ciphertext, ciphertext_len))
  {
    BIO_dump_fp(stdout, (const char *)ciphertext, ciphertext_len);
    handleErrors();
  }
  plaintext_len = len;
  if (1 != EVP_DecryptFinal_ex(ctxd, plaintext + len, &len))
  {
    BIO_dump_fp(stdout, (const char *)ciphertext, ciphertext_len);
    handleErrors();
  }
  plaintext_len += len;
  return plaintext_len;
}
