/// @file defines.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del progetto.
#pragma once

#define SEMSH 0         // Semaforo per sincronizzare l'accesso alla Shared Memory
#define SEMHCK 1        // Semaforo per sincronizzare l'accesso dell'hackler ai file F8 e F9
#define SEMIPC 2        // Semaforo per sincronizzare la creazione delle ipc
#define SEMCLS1 3       // Semaforo per sincronizzare la chiusura di SM/RM/HCK
#define SEMCLS2 4       // Semaforo per sincronizzare la chiusura di SM/RM/HCK
#define SEMCLS3 5       // Semaforo per sincronizzare la chiusura di SM/RM/HCK
#define WRITEF10 6      // Semaforo per sincronizzare la scrittura del file F10
#define HACKSTART 7     // Semaforo utilizzato per sbloccare l'hackler dopo la lettura di F1 da parte di S1
#define WRITEF1 8       // Semaforo per sincronizzare la scrittura del file F1
#define WRITEF2 9       // Semaforo per sincronizzare la scrittura del file F2
#define WRITEF3 10      // Semaforo per sincronizzare la scrittura del file F3
#define WRITEF4 11      // Semaforo per sincronizzare la scrittura del file F4
#define WRITEF5 12      // Semaforo per sincronizzare la scrittura del file F5
#define WRITEF6 13      // Semaforo per sincronizzare la scrittura del file F6


//-------------SENDER MANAGER-------------
#define P_NUM 3 
#define DIM_STRUCT 20
#define FILE_NAME "OutputFiles/F8.csv"

//--------------HACKLER--------------------
#define MAX 250
#define DIM_HACK 20

//----------------------RECEIVER MANAGER------------------
#define F9 "OutputFiles/F9.csv"

//--------------------------SENDER MANAGER----------------------------------
struct message {
    long type;
    char id[DIM_STRUCT];
    char message[DIM_STRUCT];
    char idsender[DIM_STRUCT];
    char idreceiver[DIM_STRUCT];
    char DELS1[DIM_STRUCT];
    char DELS2[DIM_STRUCT];
    char DELS3[DIM_STRUCT];
    char TYPE[DIM_STRUCT];
};

// Nodo per la lista concatenata 
struct messageNode { 
    struct message msg;
    struct messageNode *next;
};


// Struttura contenente i file descriptor dei file e delle pipe e gli id delle ipcs
struct ids {
    int fd[11];
    int pipe_fd1[2];
    int pipe_fd2[2];
    int fd_fifo;
    int shmid;
    int i_shm;
    struct message *ptr_sh;
    int msgqid;
    int *ptr_sh2;
};

// Struttura utilizzata per salvare i time stamp
struct timeStamp {
    int hours;
    int minutes;
    int seconds;
};

// Struttura utilizzata per salvare i momenti di creazione e distruzione delle varie ipcs
struct ipcStruct {
    char name[50];
    char id[50];
    char creator[50];
    struct timeStamp creationTime;
};

// Dichiarazione delle funzioni utilizzate 
void writeF10();
void fillStruct();
void calcola_dpt();
void saveMessage();
void sendMessage();
int handleMessage();
int handleMessageR();
void writeMessage();
struct messageNode *addMessageNode();
void printList();
struct messageNode *deleteAllMessageNode();
struct messageNode *deleteNode();

//---------------------------------------------------------------------------

//------------------------------HACKLER--------------------------------------

// Struttura dei messaggi di disturbo dell'hackler 
struct hackler {
    char ID[DIM_HACK];    
    char Delay[DIM_HACK];
    char Target[DIM_HACK];
    char Action[DIM_HACK];
};

// Struttura contenente i pid e gli id dei processi target (letti da f8 ed f9)
struct targetPid {
	char id[3];
	int pid;
};


void pauseHandler();
int handleAction();
void sendSignal();
