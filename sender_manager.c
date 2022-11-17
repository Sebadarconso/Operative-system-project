#include "err_exit.h"
#include "defines.h"
#include "shared_memory.h"
#include "semaphore.h"
#include "fifo.h"
#include "pipe.h"

#include <sys/shm.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <stdbool.h>
#include <sys/types.h>

#define FILE_NAME "OutputFiles/F8.csv"
#define FILE_NAME2 "OutputFiles/F10.csv"
#define ipc 50


struct messageNode *head = NULL;

pid_t childrenPid[P_NUM];                               // vettore di pid dei figli

pid_t pidS[50] = {0};                  
int childS = 0;
bool shutdown = false;


//Gestione action Shutdown
void shutdownHandler(int signal) {
    int counter = 0;
    if (signal == SIGTERM) {
		
        while(pidS[counter] != 0) {
            kill(pidS[counter++], SIGKILL);
       }
       shutdown = true;
    }
}
//Gestione action SendMSG
void sendMessageHandler(int signal) {
    int counter = 0;
    
    if (signal == SIGUSR1) {
        while (pidS[counter] != 0) {
            kill(pidS[counter++], SIGALRM);
       }       
    }
}
//Gestione action removeMSG
void removeMessageHandler(int signal) {
    int counter = 0;
    if (signal == SIGUSR2) {
        while(pidS[counter] != 0) {
            kill(pidS[counter++], SIGKILL);
       }
       
    }
}
//Gestione action IncreaseDelay
void increaseDelayHandler(int signal) {
    int counter = 0;

    if (signal == SIGINT) {
        while(pidS[counter] != 0) {            
            kill(pidS[counter++], SIGQUIT);
       }
    }
}

void wakeUpHandler(int signal) {
    //printf("Svegliato %d\n", getpid());
}

int main (int argc, char * argv[]) {
	
    struct ipcStruct ipcs[50];
    int nIpc = 0;
    char tmpID[50];

    int nByte;
    struct message message;
    struct message messageRead;
	struct ids ids;
    int charAt = 0;                                     // scorre il buffer_write di lettura
    char buffer_write[250];                                             //buffer per le write
    char buffer_read[200];
    int i_shm = 0;
    pid_t pid;

    sigset_t mySet;
    sigfillset(&mySet);
    sigprocmask(SIG_SETMASK, &mySet, NULL);

    //creazione del set set di semafori
    union semun arg;
    int semid = semget('s', 14, IPC_CREAT | S_IRUSR | S_IWUSR);
    if (semid == -1) {
        ErrExit("Errore nella creazione del set di semafori nel sender\n");
    }
    
	//Salvataggio informazioni creazione ipc
    sprintf(tmpID, "%d", semid);
    fillStruct("SM", tmpID, "SM", ipcs, nIpc++);

	//Inizializzazione semafori
    unsigned short s[14] = {1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1};
    arg.array = s;

    if(semctl(semid, 0, SETALL, arg) == -1) {
        ErrExit("Errore nella semctl");
    }

    //creazione della message queue
    int msgqid = msgget('q', IPC_CREAT | S_IRUSR | S_IWUSR);
    if (msgqid == -1) {
        ErrExit("Errore nella creazione della coda di messaggi");
    }

	//Salvataggio informazioni creazione ipc
    sprintf(tmpID, "%d", msgqid);
    fillStruct("Q", tmpID, "SM", ipcs, nIpc++);


    //creazione della shared shared_memory
    int shmid = alloc_shared_memory('m', sizeof(struct message) * 15);
    struct message *ptr_sh = (struct message *) get_shared_memory(shmid, 0);

	//Salvataggio informazioni creazione ipc
    sprintf(tmpID, "%d", shmid);
    fillStruct("SH", tmpID, "SM", ipcs, nIpc++);

	//Creazione shared memory nella quale passiamo il contatore alla cella di SH attuale
    int shmid2 = alloc_shared_memory('h', sizeof(int));
    int *ptr_sh2 = (int *) get_shared_memory(shmid2, 0);
    ptr_sh2[0] = 0;
    
	//Salvataggio informazioni creazione ipc
    sprintf(tmpID, "%d", shmid2);
    fillStruct("SH2", tmpID, "SM", ipcs, nIpc++);
    
	//Creazione FIFO 
    if (mkfifo("OutputFiles/my_fifo.txt", S_IRUSR | S_IWUSR | S_IWGRP ) == -1) {
		ErrExit("Errore nella creazione della fifo");
    }
	
	//Questo semaforo si assicura che il sender_manager abbia generato le IPC come da specifica  
    semOp(semid, SEMIPC, 1);
	
	//Apertura file F10
    int fd10 = open(FILE_NAME2, O_RDWR | O_CREAT | O_APPEND, S_IRWXU);
    if (fd10 == -1) {
      ErrExit("Errore nell'apertura/creazione del file F10.csv");
    }
    
	//Scrittura intestazione F10
	sprintf(buffer_write,("IPC;IDKey;Creator;CreationTime;DestructionTime\n"));
	if (write(fd10, buffer_write, strlen(buffer_write)) == -1) {
          ErrExit("Errore nella scrittura del file F10.csv");
    }
    
    //Apertura/Creazione/Sovrascrittura del f8.csv
    int fd8 = open(FILE_NAME, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU); 
    if (fd8 == -1) {
        ErrExit("Errore nell'apertura del file F8.csv");
    }
    
	//Scrittura intestazione F8
	sprintf(buffer_write, "SenderID; PID\n");
    if (write(fd8, buffer_write, strlen(buffer_write)) == -1) {
          ErrExit("Errore nella scrittura del file F8.csv");
    }

    //creazione delle pipe
    int pipe_fd1[2], pipe_fd2[2];

    if (pipe(pipe_fd1) == -1){
        ErrExit("Errore nella creazione della pipe 1");
	}
    if (pipe(pipe_fd2) == -1){
        ErrExit("Errore nella creazione della pipe 2");
	}

	//Salvataggio informazioni creazione pipe per file F10
    sprintf(tmpID, "%d/%d", pipe_fd1[0], pipe_fd1[1]);
    fillStruct("PIPE1", tmpID, "SM", ipcs, nIpc++);

    sprintf(tmpID, "%d/%d", pipe_fd2[0], pipe_fd2[1]);
    fillStruct("PIPE2", tmpID, "SM", ipcs, nIpc++);

	//Ids contiene tutte le informazioni utili all'accesso di IPC
    ids.fd[8] = fd8;
    ids.ptr_sh2 = ptr_sh2;
    ids.ptr_sh = ptr_sh;
    ids.i_shm = i_shm;
    ids.msgqid = msgqid;
    ids.fd[10] = fd10;
    ids.pipe_fd1[0] = pipe_fd1[0];
	ids.pipe_fd1[1] = pipe_fd1[1];
	ids.pipe_fd2[0] = pipe_fd2[0];
	ids.pipe_fd2[1] = pipe_fd2[1];

    //Creazione processi figli S1,S2,S3
    for (int i = 0; i < P_NUM; i++) {
        pid = fork();

        childrenPid[i] = pid;
        if (pid == -1) {
            ErrExit("figlio non creato!");
        //-------------------------ESECUZIONE PROCESSI FIGLI--------------------------------

        } else if (pid == 0) {
            childrenPid[i] = getpid();

			//Ridefinizione degli handler dei segnali necessari a gestire le action dell'hackler
            if (signal(SIGTERM, shutdownHandler) == SIG_ERR) {
                ErrExit("Errore nell'assegnazione dell'handler di SIGTERM");
            }

            if (signal(SIGINT, increaseDelayHandler) == SIG_ERR) {
                ErrExit("Errore nell'assegnazione dell'handler di SIGINT");
            }

            if (signal(SIGUSR1, sendMessageHandler) == SIG_ERR) {
                ErrExit("Errore nell'assegnazione dell'handler di SIGUSR1");
            }

            if (signal(SIGUSR2, removeMessageHandler) == SIG_ERR) {
                ErrExit("Errore nell'assegnazione dell'handler di SIGUSR2");
            }

            if (signal(SIGQUIT, pauseHandler) == SIG_ERR) {
                ErrExit("Errore nell'assegnazione dell'handler di SIGQUIT");
            }

            if (signal(SIGALRM, wakeUpHandler) == SIG_ERR) {
                ErrExit("Errore nell'assegnazione dell'handler di SIGALRM");
            }

			//Rimozione dei segnali necessari dal set dei segnali ignorati
            sigdelset(&mySet, SIGTERM);
            sigdelset(&mySet, SIGUSR1);
            sigdelset(&mySet, SIGUSR2);
            sigdelset(&mySet, SIGINT);
            sigprocmask(SIG_SETMASK, &mySet, NULL);
            
			
            //-------------------------ESECUZIONE PROCESSO S1--------------------------------

            if (getpid() == childrenPid[0]) {
                
                
                if(close(pipe_fd1[0]) == -1) 
                    ErrExit("Close read pipe_fd1 child 1");

                if(close(pipe_fd2[0]) == -1) 
                    ErrExit("Close read pipe_fd2 child 1");
                
                if(close(pipe_fd2[1]) == -1) 
                    ErrExit("Close write pipe_fd2 child 1");

                //Apertura in lettura del file F0.csv
                ssize_t numRead = 0;
                int field_counter = 0;                                  //Contatore del campo del F0.csv 
                int line_counter = 0;                                   // Contatore delle linee della struttura message
                int fd0 = open(argv[1], O_RDONLY, S_IRWXU);

                if (fd0 == -1) {
                    ErrExit("Errore nell'apertura del file F0.csv");
                }
                ids.fd[0] = fd0;
            
                char c;                                 //buffer per la read
                while ((numRead = read(fd0, &c, sizeof(char)) > 0)) {
                    //Lettura carattere per carattere del file F0.csv
                    if (c == ';' || c == ',' || c == '\n') {            //controllo sulle tabulazioni
						
                        buffer_read[charAt] = '\0';                     //definisce la fine della stringa da copiare nella struttura
                        charAt = 0;                                     // pronta a riempire un nuovo buffer_read da 0
        
                        saveMessage(&message, field_counter, buffer_read);
						if(c == '\n'){
							head = addMessageNode(message, head);
							strcpy(message.id,"");
							strcpy(message.idsender,"");
							strcpy(message.idreceiver,"");
							strcpy(message.TYPE,"");
							strcpy(message.message,"");
							strcpy(message.DELS1,"");
							strcpy(message.DELS2,"");
							strcpy(message.DELS3,"");
						}
                      
                        if (c == ';' || c == ',')
                            field_counter++;
                        if (c == '\n' || c == EOF) {
                            field_counter = 0;
                            line_counter++;
                        }
    
                    } else if (c != '\n' && (c != ';' || c == ',')) {
                        buffer_read[charAt++] = c;
                    }
                }
				buffer_read[charAt] = '\0';
				saveMessage(&message, field_counter, buffer_read);
                head = addMessageNode(message, head);           // Usiamo una lista per contenere i messaggi letti da F0
        
                close(fd0);

				//Questo semaforo abilita l'hackler a partire per evitare di ricevere segnali di disturbo a sistema non del tutto funzionante
                semOp(semid, HACKSTART, 1);

				
                //Apertura F1.csv in scrittura
				int fd1 = open("OutputFiles/F1.csv", O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
                if (fd1 == -1) {
                    ErrExit("Errore nell'apertura/creazione del file F1.csv");
                }
                ids.fd[1] = fd1;

				//Scrittura intestazione file F1
                sprintf(buffer_write, "ID;Message;IDSender;IDReceiver;TimeArrival;TimeDeparture;\n");
            
                if (write(fd1, buffer_write, strlen(buffer_write)) == -1) {
                    ErrExit("Errore nella scrittura del file F1.csv");
                }
				struct messageNode *curr = head;
				
				while (curr->next != NULL && !shutdown) {
					curr = curr->next;											
					pidS[childS] = handleMessage(curr->msg, 1, semid, &ids);                    
                    childS++;

				}

                deleteAllMessageNode(head);
                //Busy waiting per non far terminare S1
                while (!shutdown){}

				if (close(pipe_fd1[1]) == -1) { 
                    ErrExit("Close write pipe_fd1 child 1");
				}
				close(fd1);
            }
            //---------------------------------PROCESSO S2--------------------------------------------------
            else if (getpid() == childrenPid[1]) {
                
				//Chiusura lati read/write inutilizzati delle Pipe				
                if(close(pipe_fd1[1]) == -1) 
                    ErrExit("Close write pipe_fd1 child 2");

                if(close(pipe_fd2[0]) == -1) 
                    ErrExit("Close read pipe_fd2 child 2");

				//Apertura e creazione file F2
                int fd2 = open("OutputFiles/F2.csv", O_RDWR  | O_CREAT | O_TRUNC, S_IRWXU);
                if (fd2 == -1) {
                    ErrExit("Errore nell'apertura/creazione del file F2.csv");
                }
                ids.fd[2] = fd2;

				//Scrittura intestazione file F2
                sprintf(buffer_write, "ID;Message;IDSender;IDReceiver;TimeArrival;TimeDeparture\n");
                if (write(fd2, buffer_write, strlen(buffer_write)) == -1) {
                    ErrExit("Errore nella scrittura del file F2.csv\n");
                }

				//Il processo cicla fino a che non arriva l'action Shutdown
				while (!shutdown) {

					//Lettura dalla pipe1
                    nByte = read(pipe_fd1[0], &messageRead, sizeof(struct message));                    
				    if (nByte == sizeof(struct message)) {                        
				    	pidS[childS] = handleMessage(messageRead, 2, semid, &ids);
                        childS++;
					} else if (nByte == 0) {
                        continue;
					} else {
						ErrExit("Errore nella read della pipe di S2");
					}
				}
              
                if (close(pipe_fd1[0]) == -1) {
                    ErrExit("Close read pipe_fd1 child 2");
                }

                if (close(pipe_fd2[1]) == -1) {
                    ErrExit("Close write pipe_fd2 child 2");
                }
				close(fd2);
            //-------------------------------------------PROCESSO S3----------------------------------------------------

            } else if (getpid() == childrenPid[2]) {

				//Apertura FIFO                
                int fd_fifo = open("OutputFiles/my_fifo.txt", O_WRONLY);
                if (fd_fifo == -1) {
                    ErrExit("Errore nell'apertura in scrittura della FIFO");
                }

				//Salvataggio dati FIFO per file F10
                sprintf(tmpID, "%d", fd_fifo);
                fillStruct("FIFO", tmpID, "SM", ipcs, nIpc++);
                                
                ids.fd_fifo = fd_fifo;
                
                if (close(pipe_fd2[1]) == -1) 
                    ErrExit("Close write pipe_fd2 child 3");
                
                if (close(pipe_fd1[0]) == -1) 
                    ErrExit("Close read pipe_fd1 child 3");
                
                if (close(pipe_fd1[1]) == -1) 
                    ErrExit("Close write pipe_fd1");

				//Apertura file F3
                int fd3 = open("OutputFiles/F3.csv", O_RDWR | O_CREAT |O_TRUNC, S_IRWXU);
                if (fd3 == -1) {
                    ErrExit("Errore nell'apertura/creazione del file F3.csv");
                }
                ids.fd[3] = fd3;

				//Scrittura intestazione file F3
                sprintf(buffer_write, "ID;Message;IDSender;IDReceiver;TimeArrival;TimeDeparture\n");
                if (write(fd3,buffer_write,strlen(buffer_write))==-1) {
                    ErrExit("Errore nella scrittura del file F3.csv\n");
                }

				//Il processo cicla fino a che non arriva l'action Shutdown
				while (!shutdown) {
                    nByte = read(pipe_fd2[0], &messageRead, sizeof(struct message));
				    if (nByte == sizeof(struct message)) {                        
				    	pidS[childS] = handleMessage(messageRead, 3, semid, &ids, &ptr_sh);        
                        childS++;
					} else if (nByte == 0) {
						//busy waiting
					} else {
						ErrExit("Errore nella read della pipe di S3");
					}
				}

				if (close(pipe_fd2[0]) == -1) {
                    ErrExit("Close read pipe_fd2 child 3");
                }
				close(fd_fifo);

                //Chiamata alla funzione che scrive su file F10 (protetta da semafori)
                writeF10(ipcs, "FIFO", semid, fd10, nIpc);
				close(fd3);
            }
            
		//Attesa da parte di S1-S2-S3 della terminazione dei figli creati per gestire i messaggi
        int status = 0;
        while ((pid = wait(&status)) != -1){}
		//terminazione processi S1-S2-S3	 
		   
        exit(0);
        }     

        // Scrittura file f8
		if (i < 2) {
			sprintf(buffer_write, "S%d;%d\n", i + 1, childrenPid[i]);
			if (write(fd8, buffer_write, strlen(buffer_write)) == -1) {
				ErrExit("Errore nella scrittura del file F8.csv");
			}
		} else {
			sprintf(buffer_write, "S%d;%d", i + 1, childrenPid[i]);
			if (write(fd8, buffer_write, strlen(buffer_write)) == -1) {
				ErrExit("Errore nella scrittura del file F8.csv");
			}
		}
    }
	
    //-----------------------------------------------------------PROCESSO PADRE---------------------------------------------------------
    
    //chiusura di tutte le pipe 
    if(close(pipe_fd1[0]) == -1) 
        ErrExit("Close read pipe_fd1 parent");
                
    if(close(pipe_fd1[1]) == -1) 
        ErrExit("Close write pipe_fd1 parent");


    if(close(pipe_fd2[0]) == -1) 
        ErrExit("Close read pipe_fd2 parent");
                
     if(close(pipe_fd2[1]) == -1) 
        ErrExit("Close write pipe_fd2 parent");

	//Ulteriore semaforo per sincronizzare l'avvio dell'hackler
    semOp(semid, SEMHCK, 1);

	//Attesa da parte del processo padre Sender_manager di S1-S2-S3
    int status = 0;
    while ((pid = wait(&status)) != -1){}

	//Scrittura su file F10 delle pipe
    writeF10(ipcs, "PIPE1", semid, fd10, nIpc);
    writeF10(ipcs, "PIPE2", semid, fd10, nIpc);
    
    
	//Una volta terminato l'hackler sender_manager avvia le ultime operazioni di chiusura
    semOp(semid, SEMCLS3, -1);

	//Sblocca receiver_manager
	semOp(semid, SEMCLS2, 1);
    
	//Sbloccato da receiver_manager
    semOp(semid, SEMCLS1, -1);

	//Rimozione message queue
    if (msgctl(msgqid, IPC_RMID, NULL) == -1) {
        ErrExit("Errore nella rimozione della queue");
    }

	//Scrittura su file F10 della message queue
	writeF10(ipcs, "Q", semid, fd10, nIpc);

    //Detach e rimozione della shared memory
    free_shared_memory(ptr_sh);
    remove_shared_memory(shmid);
    writeF10(ipcs, "SH", semid, fd10, nIpc);
    
	//Detach e rimozione della shared memory
    free_shared_memory(ptr_sh2);
    remove_shared_memory(shmid2);
    writeF10(ipcs, "SH2", semid, fd10, nIpc);
    

    //Scrittura su file F10 del set di semafori
    writeF10(ipcs, "SM", semid, fd10, nIpc);

    //Rimozione di tutti i semafori
    if (semctl(semid, 0, IPC_RMID, 0) == -1) {
        ErrExit("Errore nella rimozione del set di semafori nel receiver");
    }

	//Rimozione del file Fifo
    unlink("OutputFiles/my_fifo.txt");

	//Chiusura file rimanenti
    close(fd8);
	close(fd10);
    return 0;
}