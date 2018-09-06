// PingPongOS - PingPong Operating System
// Laura Silva Lopes
// GRR20163048

#include "ppos.h"
#define STACKSIZE 32768		/* tamanho de pilha das threads */

task_t *currentTask, taskMain, taskDispatcher;
queue_t *prontas, *suspensas; //fila de tarefas prontas
int ultimoID = 0, userTasks = 0;
int tempo = 0, numTicks = 0;

// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction action;

// estrutura de inicialização to timer
struct itimerval timer;

// tratador do sinal
void tratador (int signum);

void dispatcher_body();

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init (){

  // cria a main como uma tarefa normal
  getcontext(&taskMain.context);
  taskMain.id = 0;
  taskMain.prev = NULL;
  taskMain.next = NULL;
  taskMain.stack = NULL;
  taskMain.state = READY;
  taskMain.static_prio = 0; //prioridade 0 a principio
  taskMain.dinamic_prio = taskMain.static_prio;
  taskMain.execution = systime();
  taskMain.processor = 0;
  taskMain.activations = 0;
  taskMain.exitCode = 0;
  taskMain.suspended = NULL;
	taskMain.tasKernel = 0;
  taskMain.wakeup = 0;
  currentTask = &taskMain;
  queue_append(&prontas, (queue_t*)&taskMain); //coloca Main na fila de tarefas prontas
  ++userTasks;

  // desativa o buffer da saida padrao (stdout), usado pela função printf
  setvbuf (stdout, 0, _IONBF, 0) ;

  // cria o dispatcher
  task_create (&taskDispatcher, dispatcher_body, NULL);
	(&taskDispatcher)->tasKernel = 1;
  queue_remove(&prontas, (queue_t*)&taskDispatcher);
  --userTasks;

  // registra a ação para o sinal de timer SIGALRM
  action.sa_handler = tratador ;
  sigemptyset (&action.sa_mask) ;
  action.sa_flags = 0 ;
  if (sigaction (SIGALRM, &action, 0) < 0)
  {
    perror ("Erro em sigaction: ") ;
    exit (1) ;
  }

  // ajusta valores do temporizador
  timer.it_value.tv_usec = 1000 ;      // primeiro disparo, em micro-segundos
  timer.it_value.tv_sec  = 0 ;      // primeiro disparo, em segundos
  timer.it_interval.tv_usec = 1000 ;   // disparos subsequentes, em micro-segundos
  timer.it_interval.tv_sec  = 0 ;   // disparos subsequentes, em segundos

  // arma o temporizador ITIMER_REAL (vide man setitimer)
  if (setitimer (ITIMER_REAL, &timer, 0) < 0)
  {
    perror ("Erro em setitimer: ") ;
    exit (1) ;
  }
  task_switch(&taskDispatcher);
}

unsigned int systime (){
  return tempo;
}

void tratador (int signum){
  tempo++;
  if (!currentTask->tasKernel){
    numTicks--;
    if (!numTicks){
      task_switch(&taskDispatcher);
    }
  }
}

task_t *scheduler(){
  task_t *aux = (task_t*)prontas;
  if (aux){
    int maior = aux->dinamic_prio;
    task_t *taskMaior = aux;
    //remove uma tarefa de filas prontas e retorna
    for(int i = 0; i < queue_size(prontas); i++){
      if(aux->dinamic_prio < maior){
          maior = aux->dinamic_prio;
          taskMaior = aux;
      }
      aux = aux->next;
    }
    aux = (task_t*)prontas;
    for(int i = 0; i < queue_size(prontas); i++){
      if(aux != taskMaior)
        aux->dinamic_prio--;
      aux = aux->next;
      }
      taskMaior->dinamic_prio = taskMaior->static_prio; //a task corrente tem prioridade estática
      aux = (task_t*)queue_remove(&prontas, (queue_t*)taskMaior);
    }
  return (aux);
}

void dispatcher_body(){
  task_t *next, *current;
  queue_t *aux;

  while ( userTasks > 0 ){
      next = scheduler() ;  // scheduler é uma função
      if (next){
        queue_append(&prontas, (queue_t*)next);
        numTicks = QUANTUM;
        task_switch (next) ; // transfere controle para a tarefa "next"
        next->processor += QUANTUM; // aumenta em QUANTUM ms
        if (next->state == TERMINATED){
          free(next->stack);
          next->context.uc_stack.ss_sp = NULL;
          next->context.uc_stack.ss_size = 0;

          queue_remove(&prontas, (queue_t*)next);
          userTasks--;
        }
      }
      if (suspensas){
        current = (task_t*)suspensas;
        do{
          if (current->wakeup <= systime()){
            next = current->next;
            aux = queue_remove(&suspensas, (queue_t*)current);
            queue_append(&prontas, aux);
            current = next;
          }
          else{
            current = current->next;
          }
        }while((current != (task_t*)suspensas) && suspensas);
      }
   }
   task_exit(0) ; // encerra a tarefa dispatcher
}

// Cria uma nova tarefa. Retorna um ID> 0 ou erro.
int task_create (task_t *task,			// descritor da nova tarefa
                 void (*start_func)(void *),	// funcao corpo da tarefa
                 void *arg) {			// argumentos para a tarefa

  getcontext (&task->context) ;

  task->stack = malloc (STACKSIZE) ;
  if (task->stack){
    task->context.uc_stack.ss_sp = task->stack ;
    task->context.uc_stack.ss_size = STACKSIZE ;
    task->context.uc_stack.ss_flags = 0 ;
    task->context.uc_link = NULL ;
  }
  else {
    perror ("Erro na criação da pilha: ") ;
    exit (1) ;
  }

  ultimoID++;
  task->id = ultimoID;
  task->state = READY;
  task->static_prio = 0; //prioridade 0 a principio
  task->dinamic_prio = task->static_prio;
  task->execution = systime();
  task->processor = 0;
  task->activations = 0;
  task->exitCode = 0;
  task->suspended = NULL;
	task->tasKernel = 0;
  task->wakeup = 0;

  makecontext (&task->context, (void*)(*start_func), 1, arg) ;
  queue_append(&prontas, (queue_t*)task);
  ++userTasks;

  #ifdef DEBUG
  printf ("task_create: criando contexto %d\n", task->id) ;
  #endif

  return (task->id);
}

void task_print (char *name, queue_t *queue){ //char *name é o nome da fila
	queue_t *aux;

  printf("%s [", name);
  if (queue){
    aux = queue;
    printf("%d", ((task_t*)aux)->id);
    aux = aux->next;
  	while (aux != queue){ //percorre a fila até chegar novamente no início
      printf(" ");
    	printf("%d", ((task_t*)aux)->id);
    	aux = aux->next;
  	}
  }
  printf("]\n");
}

// suspende a tarefa corrente por t milissegundos
void task_sleep (int t) {
  queue_t *aux;

  aux = queue_remove (&prontas, (queue_t*)currentTask); //tira da fila de prontas
  queue_append (&suspensas, aux); //adiciona na fila de suspenso da tarefa Task
  ((task_t*)aux)->wakeup = systime() + t; //pegar tempo e somar com t
  task_switch(&taskDispatcher);
}

// alterna a execução para a tarefa indicada
int task_switch (task_t *task){
  task_t *oldTask = currentTask;
  currentTask = task; // atualiza o currentTask

  if (oldTask->state != TERMINATED){
    oldTask->state = READY;
  }
  currentTask->state = RUNNING;
  currentTask->activations++;

  #ifdef DEBUG
  printf ("task_switch: trocando contexto %d -> %d\n", oldTask->id, currentTask->id) ;
  #endif

  return (swapcontext(&oldTask->context, &currentTask->context));
}

// retorna o identificador da tarefa corrente (main deve ser 0)
int task_id (){
  return (currentTask->id);
}

// Tarefa corrente volta ao final da fila de prontas, devolvendo o processador ao dispatcher
void task_yield (){
  //tarefa atual volta pra fila de prontas
  task_switch(&taskDispatcher);
}

void task_setprio (task_t *task, int prio){

  if ((prio < -20) || (prio > 20))
    return;
  if (task == NULL){
    currentTask->static_prio = prio;
  }
  else{
    task->static_prio = prio;
  }
}

int task_getprio (task_t *task){
  if (task == NULL)
    return(currentTask->static_prio);
  return(task->static_prio);
}

int task_join (task_t *task){
queue_t *aux;

  if (task->state==TERMINATED)
    return task->exitCode;
  else if(!task)
    return -1;
  aux = queue_remove (&prontas, (queue_t*)currentTask); //tira da fila de prontas
  queue_append (&task->suspended, aux); //adiciona na fila de suspenso da tarefa Task

  task_switch(&taskDispatcher);
  return(task->exitCode);
}

// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exitCode){
queue_t *aux, *aux2; //aux é oq eu estou removendo e aux2 é o próximo

  currentTask->state = TERMINATED;
  currentTask->execution = systime() - currentTask->execution;
  printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n", currentTask->id, currentTask->execution, currentTask->processor, currentTask->activations);

  #ifdef DEBUG
  printf ("task_exit: encerrando tarefa %d\n", currentTask->id) ;
  #endif

  if (currentTask->suspended){ //a fila de suspensos não está vazia
    aux = currentTask->suspended;
    aux2 = aux->next;
    while (currentTask->suspended && aux && aux2){ //enquanto tiver tarefas suspensas na fila faça
      aux = queue_remove(&currentTask->suspended, aux);
      queue_append(&prontas, aux);
      aux = aux2;
      aux2 = aux->next;
    }
  }

  task_switch(&taskDispatcher);
}



int sem_create (semaphore_t *s, int value){
  s->suspended = NULL;
  s->contador = value;
  s->state = 0;
  return 0;
}

int sem_down (semaphore_t *s){
	queue_t *aux;

  currentTask->tasKernel = 1;
  if (s && (s->state >= 0)){ //semaforo existe
    if (s->contador <= 0){
      aux = queue_remove (&prontas, (queue_t*)currentTask); //tira da fila de prontas
      queue_append(&s->suspended, aux); //suspende tarefa corrente, coloca na fila do semáforo
      task_switch(&taskDispatcher);
    }
    else{
      s->contador--;
    }
    currentTask->tasKernel = 0;
    return 0;
  }
  return (-1);
}

int sem_up (semaphore_t *s){
task_t *current;
queue_t *aux;

  currentTask->tasKernel = 1;

  if (s && (s->state >= 0)){
    if (s->suspended){
      current = (task_t*)s->suspended;
      aux = queue_remove (&s->suspended, (queue_t*)current); //tira da fila de suspensas
      queue_append(&prontas, aux);
      ((task_t*)aux)->state = 0;
    }
    else{
      s->contador++;
    }
    currentTask->tasKernel = 0;
    return 0;
  }
  return (-1);
}

int sem_destroy (semaphore_t *s){
task_t *current, *next;
queue_t *aux;

  currentTask->tasKernel = 1;
  if (s && (s->state >= 0)){
    // acorda todas dormindo
		if (s->suspended){
		  current = (task_t*)s->suspended;
		  do{
		    next = current->next;
		    aux = queue_remove(&s->suspended, (queue_t*)current);
		    queue_append(&prontas, aux);
		    current = next;
		  }while((current != (task_t*)s->suspended) && s->suspended);
		}

    s->state = -1;
    currentTask->tasKernel = 0;
    return 0;
  }
  return (-1);
}



// cria uma fila para até max mensagens de size bytes cada
int mqueue_create (mqueue_t *queue, int max, int size){
	if(queue && (max > 0) && (size > 0)){
		queue->buffer = (void *)malloc(max * size);
	  queue->head = 0; //indices do buffer
	  queue->tail = 0;
	  queue->max_msg = max;
		queue->msg_size = size;
		sem_create(&queue->s_vaga, queue->max_msg);
		sem_create(&queue->s_item, 0);
		sem_create(&queue->s_buffer, 1); //Control mutual exclusion
		return 0;
	}
	perror("Erro ao criar fila de mensagens\n");
	return -1;
}

// envia uma mensagem para a fila
int mqueue_send (mqueue_t *queue, void *msg){
	if (!queue){
		return -1;
	}
	int next = queue->tail + 1;
	if (next >= queue->max_msg)
			next = 0;

	if(sem_down (&queue->s_vaga) || sem_down (&queue->s_buffer))
    return -1;
	//insere item no buffer
	memcpy((char*)(queue->buffer) + queue->tail * queue->msg_size, msg, queue->msg_size); // Load data and then move
	queue->tail = next;            // head to next data offset.
  if(sem_up (&queue->s_buffer) || sem_up (&queue->s_item))
    return -1;

	return 0;
}

// recebe uma mensagem da fila
int mqueue_recv (mqueue_t *queue, void *msg){
  // next is where tail will point to after this read.
  int next = queue->head + 1;
  if(next >= queue->max_msg)
      next = 0;

  if(sem_down (&queue->s_item) || sem_down (&queue->s_buffer))
    return -1; //algo de arrado aconteceu nos downs dos semáforos
  //retira item do buffer
	memcpy(msg, (char*)(queue->buffer) + queue->head * queue->msg_size, queue->msg_size); // Read data and then move
	queue->head = next;             // tail to next data offset.
	if(sem_up (&queue->s_buffer) || sem_up (&queue->s_vaga))
    return -1;

	return 0;  // return success to indicate successful push
}

// destroi a fila, liberando as tarefas bloqueadas
int mqueue_destroy (mqueue_t *queue){
	if(queue){
		if (sem_destroy(&queue->s_vaga) ||
		    sem_destroy(&queue->s_item) ||
		    sem_destroy(&queue->s_buffer)){ //retorna -1 se der algum erro
      return -1;
    }
		free(queue->buffer);
		return 0;
	}
	perror("Erro ao destruir fila de mensagens");
	return -1;
}

// informa o número de mensagens atualmente na fila
int mqueue_msgs (mqueue_t *queue){
	return (queue->tail - queue->head + 1);
}
