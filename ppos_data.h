// PingPongOS - PingPong Operating System
// Laura Silva Lopes
// GRR20163048

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#define READY 0
#define RUNNING 1
#define TERMINATED -1
#define QUANTUM 20

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>		// biblioteca POSIX de trocas de contexto
#include "queue.h"		// biblioteca de filas genéricas
#include <sys/time.h>
#include <signal.h>

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
   struct task_t *prev, *next ;		// ponteiros para usar em filas
   int id ;				// identificador da tarefa
   ucontext_t context ;			// contexto armazenado da tarefa
   void *stack ;			// aponta para a pilha da tarefa
   int state;
   int static_prio;
   int dinamic_prio;
   int tasKernel;
   unsigned int execution, processor, activations, wakeup; //tempo
   int exitCode;
   queue_t *suspended;
} task_t ;

// estrutura que define um semáforo
typedef struct
{
  queue_t *suspended;
  int contador;
  int state;
} semaphore_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  //buffer circular
  void* buffer;
  int head; //indices do buffer
  int tail;
  int max_msg;
  int msg_size;
  semaphore_t s_vaga, s_buffer, s_item;
} mqueue_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

#endif
