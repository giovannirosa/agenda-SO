#ifndef HASH_H
#define HASH_H

#define SIZE 1000*1000*20

struct DataItem {
   unsigned char nome[15];   
   unsigned char tel[9];
};

unsigned int hashCode(unsigned char *str);
struct DataItem *search(unsigned char *nome);
void insert(unsigned char *nome,unsigned char *tel);
struct DataItem* delete(struct DataItem* item);
void display();
// struct DataItem* initHash();

#endif