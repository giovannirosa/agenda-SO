#include "casanova.h"

void handleErrors(void){
	ERR_print_errors_fp(stderr);
}

hash_table_t hashTable;

int main(int argc, char *argv[]) {
	hashTable = hash_table_malloc(HASH_SIZE);
	sem_init(&mutexWakeUp, 0, 1);
	/* inicialização */
	ERR_load_crypto_strings();
	OpenSSL_add_all_algorithms();
	OPENSSL_config(NULL);

	pthread_t thread1, thread2;
	thread_setup();
	/* Cria um thread para a entrada e outra para a saída */
	pthread_create(&thread1, NULL, (void *)&put_entries, (void *)0);
	pthread_create(&thread2, NULL, (void *)&get_entries, (void *)0);

	/* espero a morte das threads */
	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);

	fprintf(stdout, "Cleaning up\n");
	/* limpeza geral, hash_table_t table, semáforos e biblioteca de criptografia */
	thread_cleanup();
	hash_table_free(hashTable);

	FIPS_mode_set(0);
	ENGINE_cleanup();
	CONF_modules_unload(1);
	EVP_cleanup();
	CRYPTO_cleanup_all_ex_data();
	ERR_remove_state(0);
	ERR_free_strings();

	return 0;
}

/**
 * Encontra qual o último PUT feito verificando o relógio lógico de cada thread
 * lastPut: relógio lógico do último PUT
 */
void findLastPut(char *lastPut) {
	int i, count = 0;
	memset(lastPut, 0xFF, ID_SIZE - 1);
	for (i = 0; i < N_PUT_THREADS; ++i) {
		if (strncmp(lastPut, putThreads[i].actual_put, ID_SIZE - 1) > 0) {
		 memcpy(lastPut, putThreads[i].actual_put, ID_SIZE - 1);
		count++;
		}
	}
}

/**
 * Acorda threads GET que estejam eventualmente dormindo
 */
void wakeupGets() {
	char lastPut[ID_SIZE - 1];
	findLastPut(lastPut);
	sem_wait(&mutexWakeUp);
	
	for (int i = 0; i < N_GET_THREADS; ++i) {
		if ((strncmp(getThreads[i].actual_get, lastPut, ID_SIZE - 1) <= 0) && (getThreads[i].waitting)) {
			 getThreads[i].waitting = FALSE;
			 sem_post(&getThreads[i].sem_getahead);
		}
	}
	sem_post(&mutexWakeUp);
}

/**
 * Função inicial PUT, inicia a pool de threads, le dados do socket e 
 * gerencia as threads para inserir na hash 
 */
void put_entries()
{
	int server_sockfd, client_sockfd;
	struct sockaddr_un server_address;
	struct sockaddr_un client_address;
	socklen_t addr_size;
	putThread *actual_thread = NULL;
	putOver = FALSE;

	int i, count = 0, bytesrw = 0;
	int read_ret, read_total, m_avail;

	for (i = 0; i < N_PUT_THREADS; ++i) {
		putThreads[i].buffer[PUT_MESSAGE_SIZE] = '\0';
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
	sem_init(&putThreadsUsed, 0, N_PUT_THREADS);
	pthread_attr_t attr;
	cpu_set_t cpus;
	pthread_attr_init(&attr);
	// le o numero de processadores do pc atual
	int numberOfProcessors = sysconf(_SC_NPROCESSORS_ONLN);

	for (i = 0; i < N_PUT_THREADS; ++i) {
		putThreads[i].busy = FALSE;
		sem_init(&putThreads[i].sem, 0, 0);
		memset(putThreads[i].actual_put, 0, ID_SIZE - 1);
		CPU_ZERO(&cpus);
		CPU_SET(i%numberOfProcessors, &cpus);
		// define afinidade da thread para determinado núcleo
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
		pthread_create(&putThreads[i].thread, &attr, (void *)&store, &putThreads[i]);
	}

	do {// pool de threads PUT
		sem_wait(&putThreadsUsed);
		for (i = 0; i < N_PUT_THREADS; ++i) {
			if (!putThreads[i].busy) {
				// escolhe uma thread livre
				actual_thread = &putThreads[i];
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

		do {
			read_ret = read(client_sockfd, actual_thread->buffer, PUT_MESSAGE_SIZE * m_avail - read_total);
			read_total += read_ret;
		} while (read_total < PUT_MESSAGE_SIZE * m_avail && read_ret > 0);

		// reseta variável m_avail
		if (read_ret <= 0)
			m_avail = 0;

		// libera a thread atual para rodar método store
		if (read_ret > 0) {
				 actual_thread->m_avail = m_avail;
				 actual_thread->busy = TRUE;
				 sem_post(&actual_thread->sem);
		}

		count += m_avail;
	} while (read_ret > 0);

	close(client_sockfd);
	
	// sinaliza que o PUT terminou
	putOver = TRUE;
	
	// reseta relógio lógico das threads PUT
	for (i = 0; i < N_PUT_THREADS; ++i) {
		memset(putThreads[i].actual_put, 0xFF, ID_SIZE - 1);
	}
	
	// libera as threads GET que estão eventualmente esperando
	for (i = 0; i < N_GET_THREADS; ++i) {
		getThreads[i].waitting = FALSE;
		sem_post(&getThreads[i].sem_getahead);
	}
	
	// libera as threads PUT e finaliza
	for (i = 0; i < N_PUT_THREADS; ++i) {
		sem_post(&putThreads[i].sem);
		pthread_join(putThreads[i].thread, NULL);
	}

	fprintf(stderr, "PUT EXITED, %d MESSAGES RECEIVED \n", count);
}


/**
 * Faz a inserção dos dados na hash
 * self: thread atual PUT
 */
void store(putThread *self) {
	unsigned char telefone_crypt[CRYPTEDSIZE];
	unsigned char nome_crypt[CRYPTEDSIZE];
	EVP_CIPHER_CTX *ctx_crypt;
	unsigned int n;

	if (!(ctx_crypt = EVP_CIPHER_CTX_new()))
		handleErrors();
	
	// roda enquanto PUT não terminar
	while (!putOver) {
		// aguarda sinal para começar PUT
		sem_wait(&self->sem);
    // saída rápida
		if (putOver)
			return;

		for (n = 0; n < self->m_avail; n++) {
			// encripta nome para usar como chave na tabela hash
			encrypte(ctx_crypt, (unsigned char *)self->buffer + (n * PUT_MESSAGE_SIZE) + ID_SIZE, NOME_SIZE, nome_crypt);
			// encripta telefone para inserir na tabela hash
			encrypte(ctx_crypt, (unsigned char *)(self->buffer + (n * PUT_MESSAGE_SIZE) + ID_SIZE + NOME_SIZE), FONE_SIZE, telefone_crypt);

			//Verifica se por algum motivo já foi inserido o dado
			unsigned char *telefone_hash = hash_table_get(hashTable, nome_crypt);

			if (telefone_hash) {
				// ERRO: se foi inserido
				fprintf(stderr, "ERRO: %.16s:", self->buffer + (n * PUT_MESSAGE_SIZE) + ID_SIZE);
				BIO_dump_fp(stderr, (const char *)nome_crypt, CRYPTEDSIZE);
				continue;
			}
		// insere na hash
		hash_table_put(hashTable, nome_crypt, telefone_crypt);
		// atualiza relógio lógico da thread PUT atual
		memcpy(self->actual_put, self->buffer + n * PUT_MESSAGE_SIZE, ID_SIZE - 1);
		}
		
		// acorda threads GET que eventualmente estejam dormindo
		wakeupGets();
		
		// libera thread PUT na pool
		self->busy = FALSE;
		sem_post(&putThreadsUsed);
	}

	EVP_CIPHER_CTX_free(ctx_crypt);
}


/**
 * Função inicial GET, inicia a pool de threads, le dados do socket e 
 * gerencia as threads para buscar na hash 
 */
void get_entries() {
	int server_sockfd, client_sockfd, i;
	struct sockaddr_un server_address;
	struct sockaddr_un client_address;
	socklen_t addr_size;
	getThread *actual_thread = NULL;
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

	if (fp == NULL) {
		perror("OPEN:");
		exit(1);
	}

	// inicia threads
	sem_init(&getThreadsUsed, 0, N_GET_THREADS);
	sem_init(&mutexOutput, 0, 1);
	pthread_attr_t attr;
	cpu_set_t cpus;
	pthread_attr_init(&attr);
	// le o numero de processadores do pc atual
	int numberOfProcessors = sysconf(_SC_NPROCESSORS_ONLN);

	for (i = 0; i < N_GET_THREADS; ++i) {
		getThreads[i].busy = FALSE;
		sem_init(&getThreads[i].sem, 0, 0);
		sem_init(&getThreads[i].sem_getahead, 0, 0);
		memset(getThreads[i].actual_get, 0, ID_SIZE - 1);
		getThreads[i].waitting = FALSE;
		CPU_ZERO(&cpus);
		CPU_SET(i%numberOfProcessors, &cpus);
		// define afinidade da thread para determinado núcleo
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
		pthread_create(&getThreads[i].thread, &attr, (void *)&retrieve, &getThreads[i]);
	}

	do {// pool de threads GET
		sem_wait(&getThreadsUsed);
		for (i = 0; i < N_GET_THREADS; ++i) {
			if (!getThreads[i].busy) {
				// escolhe thread livre	
				actual_thread = &getThreads[i];
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
		do {
			read_ret = read(client_sockfd, actual_thread->buffer, GET_MESSAGE_SIZE * m_avail - read_total);
			read_total += read_ret;
		} while (read_total < GET_MESSAGE_SIZE * m_avail && read_ret > 0);

		// reseta variável m_avail
		if (read_ret <= 0) {
			m_avail = 0;
		}
		
		// libera a thread atual para rodar método retrieve
		if (read_ret > 0) {
			actual_thread->m_avail = m_avail;
			actual_thread->busy = TRUE;
			sem_post(&actual_thread->sem);
		}
		count += m_avail;
	} while (read_ret > 0);

	close(client_sockfd);
	
	// sinaliza que o GET terminou
	getOver = TRUE;
	
	// libera as threads GET e finaliza
	for (i = 0; i < N_GET_THREADS; ++i) {
	sem_post(&getThreads[i].sem);
	pthread_join(getThreads[i].thread, NULL);
	}
	
	fclose(fp);
	fprintf(stderr, "GET EXITED, %d MESSAGES RECEIVED\n", count);
}


/**
 * Faz a busca dos dados na hash e escreve no arquivo de saída
 * self: thread atual GET
 */
void retrieve(getThread *self) {
	unsigned char *telefone_crypt;
	unsigned char telefone_decrypt[FONE_SIZE + 1];
	unsigned char nome_crypt[CRYPTEDSIZE + 1];
	EVP_CIPHER_CTX *ctx_crypt, *ctx_decrypt;
	char lastPut[ID_SIZE];
	
	telefone_decrypt[FONE_SIZE] = '\0';
	nome_crypt[CRYPTEDSIZE] = '\0';
	
	if (!(ctx_crypt = EVP_CIPHER_CTX_new()))
		handleErrors();
	
	if (!(ctx_decrypt = EVP_CIPHER_CTX_new()))
		handleErrors();
	
	// roda enquanto GET não terminar
	while (!getOver) {
		// aguarda sinal para começar PUT
		sem_wait(&self->sem);
		// saída rápida
		if (getOver)
			return;

		for (int n = 0; n < self->m_avail; n++) {
			// atualiza relógio lógico da thread GET atual
			memcpy(self->actual_get, self->buffer + n * GET_MESSAGE_SIZE, ID_SIZE - 1);
			
			findLastPut(lastPut);
			 // dorme se o relógio lógico da thread GET atual for maior do que o relógio lógico do último PUT
			if (strncmp(self->actual_get, lastPut, ID_SIZE - 1) > 0) {
				self->waitting = TRUE;
				sem_wait(&self->sem_getahead);
			}
			
			// encripta nome para usar de chave e buscar na hash
			encrypte(ctx_crypt, (unsigned char *)self->buffer + (n * GET_MESSAGE_SIZE) + ID_SIZE, NOME_SIZE, nome_crypt);
			
			// busca telefone na hash
			telefone_crypt = hash_table_get(hashTable, nome_crypt);
			
			if (!telefone_crypt)
      {
        // ERRO: telefone não encontrado
        BIO_dump_fp(stdout, (const char *)nome_crypt, CRYPTEDSIZE);
        continue;
      }
      else
      {
		// decripta telefone retornado da hash
        decrypt(ctx_decrypt, telefone_crypt, CRYPTEDSIZE, telefone_decrypt);
        telefone_decrypt[FONE_SIZE] = '\0';
		// faz o cast para inteiro
        int telefoneint = atoi((char *)telefone_decrypt);
        sem_wait(&mutexOutput);
		// escreve no arquivo de saída
        fwrite((void *)&telefoneint, sizeof(int), 1, fp);
        sem_post(&mutexOutput);
      }
		}
		
		// sinaliza que terminou de usar a thread e a torna livre
		self->busy = FALSE;
		sem_post(&getThreadsUsed);
	}

	EVP_CIPHER_CTX_free(ctx_crypt);
	EVP_CIPHER_CTX_free(ctx_decrypt);
}

/* seguem as funções de criptografia. Eu estou usando elas no sabor não
   threadsafe, por isso o mutex. Na versão paralela é de se cogitar o
   uso thread safe desta mesma biblioteca */
inline int encrypte(EVP_CIPHER_CTX *ctxe, unsigned char *plaintext, int plaintext_len, unsigned char *ciphertext){
	int len;
	int ciphertext_len;

	if (1 != EVP_EncryptInit_ex(ctxe, EVP_aes_256_ecb(), NULL, key, NULL))
		handleErrors();
		
	//	EVP_CIPHER_CTX_set_padding(ctxe, 1);
	if (1 != EVP_EncryptUpdate(ctxe, ciphertext, &len, plaintext, plaintext_len))
		handleErrors();
		
	ciphertext_len = len;
	if (1 != EVP_EncryptFinal_ex(ctxe, ciphertext + len, &len))
		handleErrors();
		
	ciphertext_len += len;

	return ciphertext_len;
}

inline int decrypt(EVP_CIPHER_CTX *ctxd, unsigned char *ciphertext, int ciphertext_len, unsigned char *plaintext) {
	int plaintext_len, len;

	if (1 != EVP_DecryptInit_ex(ctxd, EVP_aes_256_ecb(), NULL, key, NULL))
		handleErrors();
	
	//	EVP_CIPHER_CTX_set_padding(ctxd, 0);
	if (1 != EVP_DecryptUpdate(ctxd, plaintext, &len, ciphertext, ciphertext_len)) {
		BIO_dump_fp(stdout, (const char *)ciphertext, ciphertext_len);
		handleErrors();
	}
	
	plaintext_len = len;
	if (1 != EVP_DecryptFinal_ex(ctxd, plaintext + len, &len)) {
		BIO_dump_fp(stdout, (const char *)ciphertext, ciphertext_len);
		handleErrors();
	}
	
	plaintext_len += len;
	return plaintext_len;
}
