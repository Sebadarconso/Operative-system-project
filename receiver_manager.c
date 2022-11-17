
/// @file receiver_manager.c
/// @brief Contiene l'implementazione del receiver_manager.


#include "err_exit.h"
#include "defines.h"
#include "shared_memory.h"
#include "semaphore.h"
#include "fifo.h"
#include "pipe.h"

#include <sys/shm.h>
#include <errno.h>
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



#define FILE_NAME2 "OutputFiles/F10.csv"

pid_t childrenPid[P_NUM];
pid_t pidR[250] = {0};
int childR = 0;
bool shutdown = false;

void shutdownHandler(int signal) {
    int counter = 0;    
    if (signal == SIGTERM) {
        while(pidR[counter] != 0) {
            kill(pidR[counter++], SIGKILL);
       }
       shutdown = true;
    }
}

void sendMessageHandler(int signal) {
    int counter = 0;
    if (signal == SIGUSR1) {
        while (pidR[counter] != 0) {            
            kill(pidR[counter++], SIGALRM);
       }
       
    }
}

void removeMessageHandler(int signal) {
    int counter = 0;    
    if (signal == SIGUSR2) {
        while(pidR[counter] != 0) {            
            kill(pidR[counter++], SIGKILL);
       }
    }
}

void increaseDelayHandler(int signal) {
    int counter = 0;

    if (signal == SIGINT) {
        while(pidR[counter] != 0) {
            kill(pidR[counter++], SIGQUIT);
       }
    }
}




void wakeUpHandler(int signal) {
    //printf("Svegliato %d\n", getpid());
}

int main(int argc, char * argv[]) {

    struct ipcStruct ipcs[50];
    int nIpc = 0;
    char tmpID[50];
    pid_t pid;    
    char buffer[250];
    struct message msg1, msg2, msg3;
    struct ids ids;
    int counter = 0;
    int i;
    size_t nByte;

	//Il processo padre ignora tutti i segnali
    sigset_t mySet;
    sigfillset(&mySet);
    sigprocmask(SIG_SETMASK, &mySet, NULL);   
	//Attesa per garantire che sender_manager abbia inizializzato i semafori 

	sleep(3);
    
    int semid = semget('s', 14, IPC_CREAT | S_IRUSR | S_IWUSR);
    if (semid == -1) {
        ErrExit("Errore nella creazione del set di semafori nel receiver\n");
    }

	//Se sender_manager ha finito di generare le IPC allora receiver_manager passa
    semOp(semid, SEMIPC, -1);

    int msqid = msgget('q', S_IRUSR | S_IWUSR);
    if (msqid == -1) {
        ErrExit("Errore nella creazione della Message Queue\n");
    }
    

    int shmid = alloc_shared_memory('m', sizeof(struct message) * 15);
    struct message *bufferMessage = (struct message*) get_shared_memory(shmid, 0);

    int fd10 = open(FILE_NAME2, O_RDWR | O_CREAT | O_APPEND , S_IRWXU);
    if (fd10 == -1) {
      ErrExit("Errore nell'apertura/creazione del file f10.csv");
    }
	
   
    //Apertura/Creazione/Sovrascrittura del file F9.csv
    int fd9 = open(F9, O_RDWR | O_CREAT | O_TRUNC , S_IRWXU);
    if(fd9 == -1) {
        ErrExit("Errore nell'apertura del file F9.csv");
    }

	//Scrittura intestazione file F9
	sprintf(buffer, "ReceiverID;PID\n");
    int dimensione = strlen(buffer);
    if (write(fd9, buffer, dimensione) == -1) {
            ErrExit("Errore nelle scrittura del file F9.csv");
    }
    
    int fd_fifo = open("OutputFiles/my_fifo.txt", O_RDONLY | O_NONBLOCK);
    if (fd_fifo == -1) {
        ErrExit("Errore nell'apertura in lettura della fifo");
    }
	
    int pipe_fd1[2], pipe_fd2[2];

    if (pipe(pipe_fd1) == -1){
        ErrExit("Errore nella creazione della pipe 1");
	}
    if (pipe(pipe_fd2) == -1){
        ErrExit("Errore nella creazione della pipe 2");
	}
	//Salvataggio informazioni IPC
    sprintf(tmpID, "%d/%d", pipe_fd1[0], pipe_fd1[1]);
    fillStruct("PIPE3", tmpID, "RM", ipcs, nIpc++);

    sprintf(tmpID, "%d/%d", pipe_fd2[0], pipe_fd2[1]);
    fillStruct("PIPE4", tmpID, "RM", ipcs, nIpc++);

	//In ids ci sono tutte le informazioni utili all'accesso alle IPC
    ids.pipe_fd1[0] = pipe_fd1[0];
	ids.pipe_fd1[1] = pipe_fd1[1];
	ids.pipe_fd2[0] = pipe_fd2[0];
	ids.pipe_fd2[1] = pipe_fd2[1];
    ids.fd[9] = fd9;
    ids.fd_fifo = fd_fifo;
    ids.shmid = shmid;
    ids.msgqid = msqid;

    //Creazione processi figli R1,R2,R3

    for (i = 0; i < P_NUM; i++) {
        pid = fork();
        childrenPid[i] = pid;
        if (pid == -1) {
            ErrExit("Figlio non creato!");
        //Esecuzione processi figli
        } else if (pid == 0) {

            childrenPid[i] = getpid();
            //Ridefinizione degli handler dei segnali da gestire per le Action dell'hackler
            
            if (signal(SIGTERM, shutdownHandler) == SIG_ERR) {
                ErrExit("Errore nell'assegnazione dell'handler");
            }

            if (signal(SIGINT, increaseDelayHandler) == SIG_ERR) {
                ErrExit("Errore nell'assegnazione dell'handler");
            }

            if (signal(SIGUSR1, sendMessageHandler) == SIG_ERR) {
                ErrExit("Errore nell'assegnazione dell'handler");
            }

            if (signal(SIGUSR2, removeMessageHandler) == SIG_ERR) {
                ErrExit("Errore nell'assegnazione dell'handler");
            }

            if (signal(SIGQUIT, pauseHandler) == SIG_ERR) {
                ErrExit("Errore nell'assegnazione dell'handler");
            }

            if (signal(SIGALRM, wakeUpHandler) == SIG_ERR) {
                ErrExit("Errore nell'assegnazione dell'handler");
            }

			//Rimozione dal set dei segnali ignorati dei segnali necessari alla gestione delle action dell'hackler
            sigdelset(&mySet, SIGTERM);
            sigdelset(&mySet, SIGUSR1);
            sigdelset(&mySet, SIGUSR2);
            sigdelset(&mySet, SIGINT);
            sigprocmask(SIG_SETMASK, &mySet, NULL);

            //PROCESSO R1
            if (getpid() == childrenPid[0]) {                

				if (close(pipe_fd1[0]) == -1) 
			        ErrExit("Close read pipe_fd1 R1");
					          
		 		if (close(pipe_fd1[1]) == -1) 
			    	ErrExit("Close write pipe_fd1 R1");

		  		if (close(pipe_fd2[1]) == -1) 
			    	ErrExit("Close write pipe_fd2 R1");																									
                int fd6 = open("OutputFiles/F6.csv", O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
                if (fd6 == -1)
                    ErrExit("Errore nell'apertura/creazione del file F6.csv");

                ids.fd[6] = fd6;
                sprintf(buffer, "ID;Message;IDSender;IDReceiver;TimeArrival;TimeDeparture\n");
                if (write(fd6,buffer,strlen(buffer)) == -1) 
                    ErrExit("Errore nella scrittura del file F6.csv");
                
                fcntl(pipe_fd2[0], F_SETFL, O_NONBLOCK);
                
                while (!shutdown) {
                    
                    // Message Queue
					size_t mSize = sizeof(struct message) - sizeof(long);
					if (msgrcv(msqid, &msg1, mSize, 1, IPC_NOWAIT) != -1) {                        
						pidR[childR] = handleMessageR(msg1, 1, semid, &ids);                        
                        childR++;
					} 

                    if (shutdown)  // controllo periodico dell'arrivo del segnale di SIGTERM
                        break;

                    // Shared Memory
                    while (counter < 15) {
                        if (bufferMessage[counter].type == 1) {
                            bufferMessage[counter].type = 0;                            
                            pidR[childR] = handleMessageR(bufferMessage[counter], 1, semid, &ids);                            
                            childR++;
                        
                        }
                        counter++;
                    }

                    if (shutdown)
                        break;
                        
                    // Pipe
                    nByte = read(pipe_fd2[0], &msg1, sizeof(msg1));
                    if (nByte == -1 && errno != EAGAIN)
                        ErrExit("Errore nella read della pipe");
                    else if (nByte == sizeof(msg1)) {
                        pidR[childR] = handleMessageR(msg1, 1, semid, &ids);                        
                        childR++;
                
                    }
                    counter = 0;

                }

                if (close(pipe_fd2[0]) == -1) {
			        ErrExit("Close read pipe_fd2 R1");
                }

				close(fd6);
                //PROCESSO R2
            } else if (getpid() == childrenPid[1]) {


		 		if(close(pipe_fd1[1]) == -1) 
			    	ErrExit("Close write pipe_fd1 R2");

		  		if(close(pipe_fd2[0]) == -1) 
			    	ErrExit("Close read pipe_fd2 R2");
                								
                int fd5 = open("OutputFiles/F5.csv", O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
                if (fd5 == -1)
                    ErrExit("Errore nell'apertura/creazione del file fd5.csv");
                
                ids.fd[5] = fd5;
                sprintf(buffer, "ID;Message;IDSender;IDReceiver;TimeArrival;TimeDeparture\n");
                if (write(fd5,buffer,strlen(buffer)) == -1) 
                    ErrExit("Errore nella scrittura del file F5.csv");

                fcntl(pipe_fd2[0], F_SETFL, O_NONBLOCK);

                while (!shutdown) {
                
                    // Message Queue
					size_t mSize = sizeof(struct message) - sizeof(long);
					if (msgrcv(msqid, &msg2, mSize, 2, IPC_NOWAIT) != -1) {                        
						pidR[childR] = handleMessageR(msg2, 2, semid, &ids);                        
                        childR++;
              
					} 

                    if (shutdown)
                        break;

                    // Shared Memory
                    while (counter < 15) {
                        if (bufferMessage[counter].type == 2) {
                            bufferMessage[counter].type = 0;                            
                            pidR[childR] = handleMessageR(bufferMessage[counter], 2, semid, &ids);
                            childR++;
                            
                        }
                        counter++;
                    }

                    if (shutdown)
                        break;

                    // Pipe
                    nByte = read(pipe_fd1[0], &msg2, sizeof(msg2));
                    if (nByte == -1 && errno != EAGAIN)
                        ErrExit("Errore nella read della pipe");
                    else if (nByte == sizeof(msg2)) {
                        pidR[childR] = handleMessageR(msg2, 2, semid, &ids);                        
                        childR++;
                    
                    }
                    counter = 0;

                }
				close(fd5);
                if (close(pipe_fd1[0]) == -1) 
			        ErrExit("Close read pipe_fd1 R2");

                if (close(pipe_fd2[1]) == -1) 
			        ErrExit("Close write pipe_fd2 R2");
                 //PROCESSO R3
            } else if (getpid() == childrenPid[2]) {                
                
                if (close(pipe_fd1[0]) == -1) {
			        ErrExit("Close read pipe_fd1 R3");
				}
		 		if (close(pipe_fd2[0]) == -1) {
			    	ErrExit("Close write pipe_fd2 R3");
				}
		  		if(close(pipe_fd2[1]) == -1) {
			    	ErrExit("Close write pipe_fd2 R3");
				}
				int counter = 0;
				//Apertura del file F4
                int fd4 = open("OutputFiles/F4.csv", O_RDWR  | O_CREAT | O_TRUNC, S_IRWXU);
                if (fd4 == -1) 
                    ErrExit("Errore nell'apertura/creazione del file F4.csv");
                

                ids.fd[4] = fd4;

				//Scrittura intestazione file F4
                sprintf(buffer, "ID;Message;IDSender;IDReceiver;TimeArrival;TimeDeparture\n");
                if (write(fd4,buffer,strlen(buffer)) == -1) 
                    ErrExit("Errore nella scrittura del file F4.csv");

                //Flag O_NONBLOCK rende non bloccante la pipe
                fcntl(pipe_fd2[0], F_SETFL, O_NONBLOCK);

				//Fino a sopraggiungimento della action Shutdown
                while (!shutdown) {

                    // Message Queue
					size_t mSize = sizeof(struct message) - sizeof(long);
					if (msgrcv(msqid, &msg3, mSize, 3, IPC_NOWAIT) != -1) {                        
						pidR[childR] = handleMessageR(msg3, 3, semid, &ids);                        
                        childR++;
                                       
					} 

                    if (shutdown) //controllo periodico dell'arrivo del segnale SIGTERM
                        break;

                    // Shared Memory
                    while (counter < 15) {
                        if (bufferMessage[counter].type == 3) {
                            bufferMessage[counter].type = 0;                            
                            pidR[childR] = handleMessageR(bufferMessage[counter], 3, semid, &ids);       
                            childR++;
                        }
                        counter++;
                    }

                    if (shutdown) //controllo periodico dell'arrivo del segnale SIGTERM
                        break;

                    // Fifo
                    nByte = read(fd_fifo, &msg3, sizeof(struct message));
                    if (nByte == -1 && errno != EAGAIN) {
                        ErrExit("Errore nella read della FIFO");
                    } else if (nByte == sizeof(msg3)) {
                        pidR[childR] = handleMessageR(msg3, 3, semid, &ids);                        
                        childR++;
                    } 
                    nByte = 0;
                    counter = 0;

                }

				close(fd4);
                if (close(pipe_fd1[1]) == -1) {
			    	ErrExit("Close write pipe_fd2 R3");
				}
				
			}
            //Attesa da parte di R1-R2-R3 dei figli creati per gestire i messaggi
            int status = 0;
            while((pid = wait(&status)) != -1);            
            exit(0);
        }

        // Scrittura file f8
		if (i < 2){
			sprintf(buffer, "R%d;%d\n", i + 1, childrenPid[i]);
			dimensione = strlen(buffer);
			
			if (write(fd9, buffer, dimensione) == -1) {
				ErrExit("Errore nelle scrittura del file F9.csv");
			}
		} else {
			sprintf(buffer, "R%d;%d", i + 1, childrenPid[i]);
			dimensione = strlen(buffer);
			
			if (write(fd9, buffer, dimensione) == -1) {
				ErrExit("Errore nelle scrittura del file F9.csv");
			}
		}
	}

	//Chiusura delle pipe
    if(close(pipe_fd1[0]) == -1) 
        ErrExit("Close read pipe_fd1 parent");
                
    if(close(pipe_fd1[1]) == -1) 
        ErrExit("Close write pipe_fd1 parent");

    if(close(pipe_fd2[0]) == -1) 
        ErrExit("Close read pipe_fd2 parent");
                
     if(close(pipe_fd2[1]) == -1) 
        ErrExit("Close write pipe_fd2 parent");

    //Semaforo che abilita l'hackler a terminare
    semOp(semid, SEMHCK, 1);

	//Attesa della terminazione dei figli R1-R2-R3
    int status = 0;    
    while ((pid = wait(&status)) != -1){}   

	//Scrittura su file F10 delle pipe
    writeF10(ipcs, "PIPE3", semid, fd10, nIpc);
    writeF10(ipcs, "PIPE4", semid, fd10, nIpc);
    
	//Semaforo abilitato da Hackler
    semOp(semid, SEMCLS3, -1);
	//Semaforo abilitato da Sender_manager
    semOp(semid, SEMCLS2, -1);
	//Semaforo che abilita Sender_manager
    semOp(semid, SEMCLS1, 1);

	close(fd10);
    close(fd9);
    
    return 0;
}

