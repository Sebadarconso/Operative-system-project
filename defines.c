/// @file defines.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche del progetto.

#include "defines.h"
#include "semaphore.h"
#include "err_exit.h"


#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <signal.h>
#include <stdbool.h>

bool sigQuit = false;


void semOp (int semid, unsigned short sem_num, short sem_op) {
    struct sembuf sop = {.sem_num = sem_num, .sem_op = sem_op, .sem_flg = 0};

    if (semop(semid, &sop, 1) == -1)
        ErrExit("semop failed");
}



//---------------------------------------SENDER MANAGER----------------------------------------

struct messageNode *deleteNode(struct messageNode *head, struct message m){

    struct messageNode *curr = head->next;
	struct messageNode *prev = head;
	if (head->msg.id == m.id) {
		free(head);
		return curr;
	}
		
    while (curr->msg.id != m.id) {
		prev = curr;
        curr = curr->next;
    }

	prev->next = curr->next;
	free(curr);

	return head;
}

struct messageNode *addMessageNode (struct message message, struct messageNode *head) {
    struct messageNode *newNode = (struct messageNode *) malloc(sizeof(struct messageNode));
    
    if (newNode == NULL) {
      ErrExit("Errore di allocazione");
    }
    struct messageNode *tmp = head;
    newNode->msg = message;
    newNode->next = NULL;
    
    
    if (head == NULL){
      head = newNode;
    } else {
      while (tmp->next != NULL) {
        tmp = tmp -> next;
      }
      tmp->next = newNode;
    }

    return head;
}


struct messageNode *deleteAllMessageNode (struct messageNode *head) {
    struct messageNode *curr;
    struct messageNode *prev;

    for (curr = head; curr != NULL; curr = curr -> next) {
        if (curr != head) {
            prev = curr;
            free(prev);
        }
    }
    head->next = NULL;
    return head;
}

void printList (struct messageNode *head) {
    struct messageNode *curr = head;
    while (curr != NULL) {

        printf("%s\t%s\t%s\t%s\t%s\t%s\t%s\n", curr->msg.id, curr->msg.idsender, curr->msg.idreceiver,
         curr->msg.DELS1, curr->msg.DELS2, curr->msg.DELS3, curr->msg.TYPE);

        curr = curr->next;
    }
}

//Funzione che salva nella struttura message il rispettivo campo passato come stringa

void saveMessage(struct message *s_message, int fieldCounter, char *buffer) {

    //controllo sui campi della struttura
    switch (fieldCounter) {

        case 0:
            strcpy(s_message->id, buffer);
            break;

        case 1:
            strcpy(s_message->message, buffer);
            break;

        case 2:
            strcpy(s_message->idsender, buffer);
            break;

        case 3:
            strcpy(s_message->idreceiver, buffer);
            break;

        case 4:
            strcpy(s_message->DELS1, buffer);
            break;

        case 5:
            strcpy(s_message->DELS2, buffer);
            break;

        case 6:
            strcpy(s_message->DELS3, buffer);
            break;

        case 7:
            strcpy(s_message->TYPE, buffer);
            break;

        default:
            break;
    }
}

//Funzione che gestice ogni messaggio singolarmente assegnato ad un nuovo processo figlio nel Sender

int handleMessage(struct message msg, int sender, int semid, struct ids *ids){

	pid_t pid = fork();

	int nByte;
    int tempo;

	if (pid == 0) {
        sigset_t mySet;
        sigfillset(&mySet);
        sigdelset(&mySet, SIGALRM);
        sigdelset(&mySet, SIGQUIT);
        sigprocmask(SIG_SETMASK, &mySet, NULL);

        struct timeStamp t_arr, t_dep;

		time_t t;                                     
		t = time(NULL);
    
		struct tm *tm_info = localtime(&t);


        t_arr.seconds = tm_info->tm_sec;
        t_arr.minutes = tm_info->tm_min;
        t_arr.hours = tm_info->tm_hour;


		switch (sender) {
			
			case 1:

                tempo = sleep(atoi(msg.DELS1));
                while (sigQuit == true) {
                    sigQuit = false;	
                    tempo = sleep(tempo);
                }

                time(&t);
                tm_info = localtime(&t);

                t_dep.seconds = tm_info->tm_sec;
                t_dep.minutes = tm_info->tm_min;
                t_dep.hours = tm_info->tm_hour;

                
				semOp(semid, WRITEF1, -1);
				writeMessage(msg, sender, ids, t_arr, t_dep);
				semOp(semid, WRITEF1, 1);
                
				if (strcmp(msg.idsender,"S1") == 0){
					sendMessage(msg, semid, ids);
				} else {
					nByte = write(ids->pipe_fd1[1], &msg, sizeof(struct message));
					
					if (nByte != sizeof(struct message)) {
						ErrExit("Errore di write nella pipe");
					}
				}
                
				break;
				
			case 2:

				tempo = sleep(atoi(msg.DELS2));
                while (sigQuit == true) {
                    sigQuit = false;	
                    tempo = sleep(tempo);
                }

                time(&t);
                tm_info = localtime(&t);

                t_dep.seconds = tm_info->tm_sec;
                t_dep.minutes = tm_info->tm_min;
                t_dep.hours = tm_info->tm_hour;

				semOp(semid, WRITEF2, -1);
				writeMessage(msg, sender, ids, t_arr, t_dep);
				semOp(semid,WRITEF2,1);
                
				if (strcmp(msg.idsender,"S2") == 0){
					sendMessage(msg, semid, ids);
				} else {
					nByte = write(ids->pipe_fd2[1], &msg , sizeof(struct message));
					if (nByte != sizeof(struct message)) {
						ErrExit("Errore di write nella pipe");
					}
				}
				break;
			
			case 3: 
				tempo = sleep(atoi(msg.DELS3));
                while (sigQuit == true) {
                    sigQuit = false;	
                    tempo = sleep(tempo);
                }

                time(&t);
                tm_info = localtime(&t);

                t_dep.seconds = tm_info->tm_sec;
                t_dep.minutes = tm_info->tm_min;
                t_dep.hours = tm_info->tm_hour;

                semOp(semid, WRITEF3, -1);
				writeMessage(msg, sender, ids, t_arr, t_dep);
				semOp(semid, WRITEF3, 1);
                
				if (strcmp(msg.idsender,"S3") == 0){
					sendMessage(msg, semid, ids);
				} else {
					ErrExit("Nessun destinatario");
				}
				break;
			
			default: 
				break;
		}
		//Scrittura dei messaggi
		
		exit(0);
	}
	
    return pid;

}

// Funzione che scrive un messaggio inviato o ricevuto sul rispettivo file

void writeMessage(struct message msg, int sender, struct ids *ids, struct timeStamp t_arr, struct timeStamp t_dep) {
    
    char buffer[200];

    char delay[50];
    switch (sender) {
        case 1:
            strcpy(delay, msg.DELS1);
            break;
        case 2:
            strcpy(delay, msg.DELS2);
            break;

        case 3:
            strcpy(delay, msg.DELS3);
            break;

        case 4:
			strcpy(delay, msg.DELS3);
			break;

        case 5:
			strcpy(delay, msg.DELS2);
			break;
            
        case 6:
			strcpy(delay, msg.DELS1);
			break;

        default:
            break;
    }
	
	sprintf(buffer, "%s;%s;%c;%c;%d:%d:%d;%d:%d:%d\n", msg.id, msg.message, 
                            msg.idsender[1], msg.idreceiver[1], 
                            t_arr.hours, t_arr.minutes, t_arr.seconds,
                            t_dep.hours, t_dep.minutes, t_dep.seconds);

    if (write(ids->fd[sender], buffer, strlen(buffer)) == -1) {
        ErrExit("Errore nella write nella write defines");
    }
	

}

// Funzione che invia il messaggio attraverso l'opportuna IPC

void sendMessage(struct message msg, int semid, struct ids *ids) {
    char type = msg.idreceiver[1];
    msg.type = atoi(&type);

    if (strcmp(msg.TYPE, "Q") == 0 ) {


        size_t mSize = sizeof(struct message) - sizeof(long);

        if (msgsnd(ids->msgqid, &msg, mSize, 0) != 0)
            ErrExit("Errore nella msgsnd");


    } else if (strcmp(msg.TYPE, "SH") == 0) {
        
        
        semOp(semid, SEMSH, -1);

        ids->ptr_sh[ids->ptr_sh2[0]++] = msg;

        semOp(semid, SEMSH, 1);
        
    } else if (strcmp(msg.TYPE, "FIFO") == 0){

        ssize_t numWrite = write(ids->fd_fifo, &msg, sizeof(struct message));
        if ( numWrite != sizeof(struct message)) {
            ErrExit("Errore nella write");
        }
    }

}

// Funzione che riempie la struttura ipcStruct con le informazioni sulla IPC

void fillStruct(char *name, char *id, char *creator, struct ipcStruct *ipc, int i) {
    time_t creationTime;
    struct tm *tm_info;
    struct timeStamp ipcCreationTime;

    
    time(&creationTime);
    tm_info = localtime(&creationTime);
    ipcCreationTime.hours = tm_info->tm_hour;
    ipcCreationTime.minutes = tm_info->tm_min;
    ipcCreationTime.seconds = tm_info->tm_sec;

	
    strcpy(ipc[i].name, name);
    strcpy(ipc[i].id, id);
    strcpy(ipc[i].creator, creator);
	
    ipc[i].creationTime = ipcCreationTime;

}

void writeF10(struct ipcStruct *ipc, char *name, int semid, int fd10, int nIpc) {
    time_t creationTime;
    struct tm *tm_info;
    char buffer[250];

    
    time(&creationTime);
    tm_info = localtime(&creationTime);

    for (int i = 0; i < nIpc; i++) {
		
        if (strcmp(name, ipc[i].name) == 0) {
            sprintf(buffer, "%s;%s;%s;%d:%d:%d;%d:%d:%d\n", ipc[i].name,ipc[i].id, ipc[i].creator, ipc[i].creationTime.hours, ipc[i].creationTime.minutes, ipc[i].creationTime.seconds, tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);

            semOp(semid, WRITEF10, -1);
            
            if (write(fd10, buffer, strlen(buffer)) == -1)
                ErrExit("Errore nella write di F10");
    
            semOp(semid, WRITEF10, 1);

            break;
        }
    }
}
//---------------------------------------RECEIVER-------------------------------------------------------


//Funzione che gestice ogni messaggio singolarmente assegnato ad un nuovo processo figlio nel Receiver

int handleMessageR(struct message msg, int receiver, int semid, struct ids *ids) {
	
	pid_t pid = fork();
	int nByte;
    int tempo;
	if (pid == 0) {

        sigset_t mySet;
        sigfillset(&mySet);
        sigdelset(&mySet, SIGALRM);
        sigdelset(&mySet, SIGQUIT);
        sigprocmask(SIG_SETMASK, &mySet, NULL);
		time_t t;                                                       //tipo non specificato, genericamente definito come long int

        struct timeStamp t_arr, t_dep;
		time(&t); 
		struct tm *tm_info = localtime(&t);
        t_arr.seconds = tm_info->tm_sec;
        t_arr.minutes = tm_info->tm_min;
        t_arr.hours = tm_info->tm_hour;
			
		switch (receiver) {

			case 1:
				
				tempo = sleep(atoi(msg.DELS1));
                while (sigQuit == true) {

                    sigQuit = false;	
                    tempo = sleep(tempo);
                }

                time(&t);
                tm_info = localtime(&t);

                t_dep.seconds = tm_info->tm_sec;
                t_dep.minutes = tm_info->tm_min;
                t_dep.hours = tm_info->tm_hour;
                
                semOp(semid, WRITEF6, -1);
				writeMessage(msg, 6, ids, t_arr, t_dep);
				semOp(semid, WRITEF6, 1);
                
                
				break;
				
			case 2:

				tempo = sleep(atoi(msg.DELS2));
                while (sigQuit == true) {

                    sigQuit = false;	
                    tempo = sleep(tempo);
                }
                time(&t);
                tm_info = localtime(&t);

                t_dep.seconds = tm_info->tm_sec;
                t_dep.minutes = tm_info->tm_min;
                t_dep.hours = tm_info->tm_hour;

                semOp(semid, WRITEF5, -1);
				writeMessage(msg, 5, ids, t_arr, t_dep);
				semOp(semid, WRITEF5, 1);

				nByte = write(ids->pipe_fd2[1], &msg , sizeof(struct message));
				if (nByte != sizeof(struct message)) {
					ErrExit("Errore di write nella pipe");
				}
				break;
			
			case 3: 

				tempo = sleep(atoi(msg.DELS3));

                while (sigQuit == true) {

                    sigQuit = false;	
                    tempo = sleep(tempo);
                }
                time(&t);
                tm_info = localtime(&t);

                t_dep.seconds = tm_info->tm_sec;
                t_dep.minutes = tm_info->tm_min;
                t_dep.hours = tm_info->tm_hour;
                
                semOp(semid, WRITEF4, -1);
				writeMessage(msg, 4, ids, t_arr, t_dep);
				semOp(semid, WRITEF4, 1);

				nByte = write(ids->pipe_fd1[1], &msg , sizeof(struct message));
				if (nByte != sizeof(struct message)) {
					ErrExit("Errore di write nella pipe");
				}
				break;
			
			default: 
				break;
		}
		//Scrittura dei messaggi
		
		exit(0);
	}

    return pid;

    }

//------------------------------------------HACKLER---------------------------------------------

// Funzione per la gestione dei segnali da parte dell'hackler

int handleAction(struct hackler action, struct targetPid *idToPid){
	pid_t pid = fork();
	if (pid == 0) {
		sleep(atoi(action.Delay));
        if (strcmp(action.Action, "IncreaseDelay") == 0) {
			for (int counter = 0; counter < 6; counter++) {
                if (strcmp(action.Target, idToPid[counter].id) == 0)
                    kill(idToPid[counter].pid, SIGINT);
            }
            
        } else if(strcmp(action.Action, "RemoveMsg") == 0) {
            for (int counter = 0; counter < 6; counter++) {
                if (strcmp(action.Target, idToPid[counter].id) == 0)
                    kill(idToPid[counter].pid, SIGUSR2);
            }

		} else if(strcmp(action.Action, "SendMsg") == 0) {
            for (int counter = 0; counter < 6; counter++) {
                if (strcmp(action.Target, idToPid[counter].id) == 0)
                    kill(idToPid[counter].pid, SIGUSR1);
            }

		} else if(strcmp(action.Action, "ShutDown") == 0) {
            for (int counter = 0; counter < 6; counter++) {

                kill(idToPid[counter].pid, SIGTERM);
            }
		}
        exit(0);  
	}

    if (strcmp(action.Action, "ShutDown") == 0)
        return 1;
    return 0;

}

void pauseHandler(int signal) {

    if (signal == SIGQUIT) {

        sleep(5);
        sigQuit = true;
    }
}
