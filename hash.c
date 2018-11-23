#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "hash.h"

// struct DataItem {
//    unsigned char *nome;
//    unsigned char *tel;
// };

struct DataItem* hashArray[SIZE];
struct DataItem* dummyItem;
struct DataItem* item;

unsigned int hashCode(unsigned char *str)
{
    unsigned int hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

// void initHash() {
//     for (int i = 0; i < SIZE; ++i) {
//         hashArray[i]->nome = NULL;
//         hashArray[i]->tel = NULL;
//     }
// }

struct DataItem *search(unsigned char *nome) {
   //get the hash 
   int hashIndex = hashCode(nome) % SIZE;  

	
   //move in array until an empty 
   while(hashArray[hashIndex] != NULL) {
	
      if(strcmp((const char *)hashArray[hashIndex]->nome,(const char *)nome) == 0)
         return hashArray[hashIndex]; 
			
      //go to next cell
      ++hashIndex;
		
      //wrap around the table
      hashIndex %= SIZE;
   }        
	
   return NULL;        
}

void insert(unsigned char *nome,unsigned char *tel) {

   struct DataItem *item = (struct DataItem*) malloc(sizeof(struct DataItem));
   for (int i = 0; i < 15; ++i) {
       item->nome[i] = nome[i];
   }

   for (int i = 0; i < 9; ++i) {
       item->tel[i] = tel[i];
   }
   

   //get the hash 
   int hashIndex = hashCode(nome) % SIZE;

   //move in array until an empty or deleted cell
   while(hashArray[hashIndex] != NULL) {
      //go to next cell
      ++hashIndex;
		
      //wrap around the table
      hashIndex %= SIZE;
   }
	
   hashArray[hashIndex] = item;
}

struct DataItem* delete(struct DataItem* item) {
   unsigned char *nome = item->nome;

   //get the hash 
   int hashIndex = hashCode(nome);

   //move in array until an empty
   while(hashArray[hashIndex] != NULL) {
	
      if(strcmp((const char *)hashArray[hashIndex]->nome,(const char *)nome) == 0) {
         struct DataItem* temp = hashArray[hashIndex]; 
			
         //assign a dummy item at deleted position
         hashArray[hashIndex] = dummyItem; 
         return temp;
      }
		
      //go to next cell
      ++hashIndex;
		
      //wrap around the table
      hashIndex %= SIZE;
   }      
	
   return NULL;        
}

void display() {
   int i = 0;
	
   for(i = 0; i<SIZE; i++) {
	
      if(hashArray[i] != NULL)
         printf(" (%s,%s)",hashArray[i]->nome,hashArray[i]->tel);
      else
         printf(" ~~ ");
   }
	
   printf("\n");
}