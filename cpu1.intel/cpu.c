// cpu.c
// modulo de escalonamento de cpu do OSP usando o algoritmo round robin
// implementado por Rodrigo dos Santos Oliveira
// acadêmico em Ciência da Computação no Instituto de Informática da Universidade Federal de Goiás.

/****************************************************************************/
/*                                                                          */
/* 			     Module CPU                                     */
/* 			External Declarations 				    */
/*                                                                          */
/****************************************************************************/



/* OSP constant      */

#define   MAX_PAGE                   16 /* max size of page tables          */
#include <stdlib.h>
/* OSP enumeration constants */

typedef enum {
    false, true                         /* the boolean data type            */
} BOOL;

typedef enum {
    running, ready, waiting, done       /* types of status                  */
} STATUS;



/* external type definitions */

typedef struct page_entry_node PAGE_ENTRY;
typedef struct page_tbl_node PAGE_TBL;
typedef struct event_node EVENT;
typedef struct pcb_node PCB;



/* external data structures */

extern struct page_entry_node {
    int    frame_id;    /* frame id holding this page                       */
    BOOL   valid;       /* page in main memory : valid = true; not : false  */
    int    *hook;       /* can hook up anything here                        */
};

extern struct page_tbl_node {
    PCB    *pcb;        /* PCB of the process in question                   */
    PAGE_ENTRY page_entry[MAX_PAGE];
    int    *hook;       /* can hook up anything here                        */
};

extern struct pcb_node {
    int    pcb_id;         /* PCB id                                        */
    int    size;           /* process size in bytes; assigned by SIMCORE    */
    int    creation_time;  /* assigned by SIMCORE                           */
    int    last_dispatch;  /* last time the process was dispatched          */
    int    last_cpuburst;  /* length of the previous cpu burst              */
    int    accumulated_cpu;/* accumulated CPU time                          */
    PAGE_TBL *page_tbl;    /* page table associated with the PCB            */
    STATUS status;         /* status of process                             */
    EVENT  *event;         /* event upon which process may be suspended     */
    int    priority;       /* user-defined priority; used for scheduling    */
    PCB    *next;          /* next pcb in whatever queue                    */
    PCB    *prev;          /* previous pcb in whatever queue                */
    int    *hook;          /* can hook up anything here                     */
};


/* external variables */

extern PAGE_TBL *PTBR;		/* page table base register */

extern int    Quantum;		/* global time quantum; contains the value
				   entered at the beginning or changed 
				   at snapshot. Has no effect on timer
				   interrupts, unless passed to set_timer() */



/* external routines */

extern prepage(/* pcb */);
extern int start_cost(/* pcb */);
/*  PCB    *pcb; */    

extern set_timer(/* time_quantum */);
/*  int    time_quantum; */

extern int get_clock();





/****************************************************************************/
/*                                                                          */
/*				Module CPU				    */
/*			     Internal Routines				    */
/*                                                                          */
/****************************************************************************/


// definições da fila
typedef struct no No;
typedef struct fila Fila;

struct no {
    PCB *pcb;
    No *prox;
};

struct fila {
    No *cabeca;
    No *cauda;
};

// funções da fila

//inicializar fila
void inicializar(Fila *fila) {
    fila->cabeca = NULL;
    fila->cauda = NULL;
}

//verificar se esta vazia
BOOL filaVazia(Fila *fila) {
    if(fila->cabeca == NULL && fila->cauda == NULL) 
        return true;
    else
        return false;
}

//criar no
No * criarNo(PCB *pcb) {
    No *aux = malloc(sizeof(struct no));
    if(aux != NULL) {
        aux->prox = NULL;
        aux->pcb = pcb;
    }
    return aux;
}

//inserir no
void inserirNo(Fila *fila, No * no) {
    if(filaVazia(fila) == true) {
        fila->cabeca = no;
        fila->cauda = no;
    } else {
        fila->cauda->prox = no;
        fila->cauda = no;
    }
}

//desenfileirar pcb
PCB* desenfileirar(Fila *fila) {
    PCB *pcb;
    if(filaVazia(fila) == true) {
        return NULL;
    } else 
        if(fila->cabeca == fila->cauda) {
            // Fila com apenas um elemento
            pcb = fila->cabeca->pcb;
            inicializar(fila);
        } else {
            // Fila com mais de um elemento
            No *aux;
            aux = fila->cabeca;
            pcb = aux->pcb;
            fila->cabeca = aux->prox;
            free(aux);
        }
    return pcb;
}

//verificar se o processo já não está presente na fila
BOOL estaNaFila(Fila *fila, PCB *pcb) {
    BOOL achou = false;    
    if(filaVazia(fila) == false) {
        No *aux = fila->cabeca;
        do {
            if(aux->pcb->pcb_id != pcb->pcb_id)
                aux = aux->prox;
            else
                achou = true;

        } while(aux != NULL && achou != true);
    }
    return achou;
}

//fila de pronto
Fila filaDePronto;

/* ----------------------- procedimentos internos ----------------------- */
void cpu_init() {
    set_timer(Quantum);
    inicializar(&filaDePronto);
}

void insert_ready(PCB *pcb) {
    //verifica se o pcb já não está na fila de pronto
    if(estaNaFila(&filaDePronto, pcb) == false) {
        //muda o estado do processo para "ready"
        pcb->status = ready;
        //cria novo no com o pcb passado por parametro
        No *aux = criarNo(pcb);
        //adiciona o no na fila de pronto
        inserirNo(&filaDePronto, aux);
    }
}


void dispatch() {
    //checar se havia algum processo rodando antes da chamada de dispatch()
    if(PTBR != NULL && PTBR->pcb->status == running)
		insert_ready(PTBR->pcb);
    
    //desenfileira proximo processo da fila
    PCB *auxPCB = desenfileirar(&filaDePronto);
    //se conseguir desenfileirar um processo
    if(auxPCB != NULL) {
        //aponta PTBR para a tabela de página do processo a ser escalonado
        PTBR = auxPCB->page_tbl;

        //muda do estado do procisso para "running"
        auxPCB->status = running;

        //prepage table 
        prepage(auxPCB);

        //atualiza o last_dispatch do processo
        auxPCB->last_dispatch = get_clock();

        //set timer
        set_timer(Quantum);

    } else {
        //caso não consiga, setar PTBR para NULL
        PTBR = NULL;
    }
}


/* end of module */