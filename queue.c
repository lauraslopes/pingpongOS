// PingPongOS - PingPong Operating System
// Laura Silva Lopes
// GRR20163048

// Implementação de operações em uma fila genérica.

#include "queue.h"
#include <stdio.h>

//------------------------------------------------------------------------------
// estrutura de uma fila genérica, sem conteúdo definido.
// Veja um exemplo de uso desta estrutura em testafila.c
/*
typedef struct queue_t
{
   struct queue_t *prev ;  // aponta para o elemento anterior na fila
   struct queue_t *next ;  // aponta para o elemento seguinte na fila
} queue_t ;
*/
//------------------------------------------------------------------------------
// Insere um elemento no final da fila.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - o elemento deve existir
// - o elemento nao deve estar em outra fila

void queue_append (queue_t **queue, queue_t *elem){ // ponteiro para o inicio da fila e para o elemento que se quer inserir
  queue_t *aux;

  if (queue && elem && !elem->next && !elem->prev){  //a fila e o elemento existe, o elemento não está em outra fila
    if (*queue){ //a cabeça da fila está apontando para um elemento
  		aux = (*queue)->prev; //auxiliar recebe o ultimo elemento da fila
  		elem->next = *queue;
  		aux->next = elem;
  		(*queue)->prev = elem;
  		elem->prev = aux;
  	}
  	else { //fila vazia
      elem->next = elem;
      elem->prev = elem;
      *queue = elem;
  	}
  }
}

//------------------------------------------------------------------------------
// Remove o elemento indicado da fila, sem o destruir.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - a fila nao deve estar vazia
// - o elemento deve existir
// - o elemento deve pertencer a fila indicada.
// Retorno: apontador para o elemento removido, ou NULL se erro

queue_t *queue_remove (queue_t **queue, queue_t *elem) {
  queue_t *aux;

	if (queue && *queue && elem){  //a fila e o elemento existem
    aux = *queue;
    if (aux != elem){
      aux = aux->next;
    	while ((aux != elem) && (aux != *queue)){
        aux = aux->next;
      }
    }
    if (aux == elem){
      if (aux == *queue){
        if (queue_size(*queue) == 1){
     			*queue = NULL;
        }
      	else{
        	*queue = aux->next;
        }
      }
      aux->prev->next = aux->next;
      aux->next->prev = aux->prev;
      aux->next = NULL;
      aux->prev = NULL;
      return aux;
    }
  }
  return NULL; //fila vazia ou não achou o elemento
}

//------------------------------------------------------------------------------
// Conta o numero de elementos na fila
// Retorno: numero de elementos na fila

int queue_size (queue_t *queue){ //ponteiro para o inicio da fila
  int cont = 0;
  queue_t *aux;

  if (queue){ //se a fila não está vazia
    cont++; //temos um elemento
    aux = queue->next; //aux é uma variável global
    while (aux != queue){ //percorre a fila até chegar novamente no inicio
      cont++;
      aux = aux->next;
    }
  }

  return cont;
}

//------------------------------------------------------------------------------

void queue_print (char *name, queue_t *queue, void print_elem (void*) ){ //char *name é o nome da fila
	queue_t *aux;

  printf("%s [", name);
  if (queue){
    aux = queue;
    print_elem(aux);
    aux = aux->next;
  	while (aux != queue){ //percorre a fila até chegar novamente no início
      printf(" ");
    	print_elem(aux);
    	aux = aux->next;
  	}
  }
  printf("]\n");
}
