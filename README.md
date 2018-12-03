# Sistemas Operacionais Trabalho

Trabalho da matéria de Sistemas Operacionais do segundo semestre de 2018, Universidade Federal do Paraná - UFPR.

## Início

Existe um makefile neste diretório com os seguintes comandos disponíveis:

* make: compila os arquivos
* make clean: limpa os arquivos

### Pré-requisitos

É necessario gcc para compilar o código, a biblioteca pthreads, semaphore, openssl, unistd, entre outras.

## Conteúdo

* put_client - Cliente Unix Socket que gera as entradas a serem
armazenadas. 

* get_client - Cliente Unix Socket que gera requisições de consulta.
Todas as requisições realizadas são supostamente válidas e existentes. A
taxa de envio de requisições começa baixa e aumenta durante o tempo.

* casanova - Implementação usando apenas duas threads, uma para
cada cliente. A biblioteca utilizada GHashTable. Você não deve usar esta
mesma biblioteca no seu programa. 

* casanova.h - Cabeçalho com estruturas comuns aos três programas.

* hash - Contém o código fonte e cabeçalho referente ao hash

* locking - Contém o código fonte e cabeçalho referente ao locking

## Execução

Existe um script para rodar o programa chamado run.sh.
```
./run.sh
```

## Autor

* **Giovanni Rosa** - [giovannirosa](https://github.com/giovannirosa)
* **Roberta Samistraro** - [rosamis](https://github.com/rosamis)
* **Julio Doebeli** - [JulioDoebeli](https://github.com/JulioDoebeli)

## Licença

Código aberto, qualquer um pode usar para qualquer propósito.