/// @file client.c
/// @brief Contiene l'implementazione del client.

#include "defines.h"
#include "semaphore.h"
#include "err_exit.h"
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <signal.h>



int main(int argc, char * argv[]) {
	
    struct hackler hack1[MAX] = {0};            // Struttura contenente le azioni di disturbo da eseguire
	struct targetPid idToPid[MAX];              // Struttura contenente id e pid del processo equivalente
    int j = 0;
    char buffer_read[MAX];
    char c;
    int fd = open(argv[1], O_RDONLY);
    int numRead;
    int field_counter = 0;
    int line_counter = 0;
    int shutdown = 0;
    int nAction;

    if (fd == -1) {
        ErrExit("Errore nell'apertura del file F7");
    }
    
    // Parsing del file F7

    do {
        numRead = read(fd, &c, sizeof(char));
        if (c == ';' || c == '\n'||c == ',') {
            buffer_read[j] = '\0';
            j = 0;

            switch(field_counter) {

                case 0:
                    strcpy(hack1[line_counter].ID, buffer_read);
                    break;

                case 1:
                    strcpy(hack1[line_counter].Delay, buffer_read);
                    break;

                case 2:
                    strcpy(hack1[line_counter].Target, buffer_read);
                    break;

                case 3:
                    strcpy(hack1[line_counter].Action, buffer_read);
                    break;
                
                default: 
                    break;
            }
        
            if (c == ';' || c == ',')
                field_counter++;
            if (c == '\n') {
                field_counter = 0;
                line_counter++;
            }

        } else if (c != '\n' && (c != ';' || c != ','))
            buffer_read[j++] = c;
    
    } while (numRead != 0);
    buffer_read[--j] = '\0';
    strcpy(hack1[line_counter].Action, buffer_read);
    nAction = line_counter + 1;


    field_counter = 0;
    line_counter = 0;
    j = 0;

	int semid = semget('s', 14, IPC_CREAT | S_IRUSR | S_IWUSR);
    if (semid == -1) {
        ErrExit("Errore nella creazione del set di semafori nell'hackler\n");
    }

	//./sender_manager InputFiles/F0.csv & ./receiver_manager & ./hackler InputFiles/F7.csv 

	semOp(semid, SEMHCK, -2);
	semOp(semid, HACKSTART, -1);

	int fd8 = open("OutputFiles/F8.csv", O_RDWR | O_CREAT , S_IRWXU);
	if(fd8 == -1){
		ErrExit("Errore apertura f8 nell'hackler");
	}

    int fd9 = open("OutputFiles/F9.csv", O_RDWR | O_CREAT , S_IRWXU);
	if(fd9 == -1){
		ErrExit("Errore apertura f9 nell'hackler");
	}

    // Lettura F8
    lseek(fd8, 14, SEEK_SET);

    do {
        numRead = read(fd8, &c, sizeof(char));
        if (c == ';' || c == '\n'||c == ',') {
            buffer_read[j] = '\0';
            j = 0;

            switch(field_counter) {

                case 0:
                    strcpy(idToPid[line_counter].id, buffer_read);
                    
                    break;

                case 1:
                    idToPid[line_counter].pid = atoi(buffer_read);

                    break;
					
                default: 
                    break;
            }
        
            if (c == ';' || c == ',')
                field_counter++;
            if (c == '\n') {
                field_counter = 0;
                line_counter++;
            }

        } else if (c != '\n' && (c != ';' || c != ','))
            buffer_read[j++] = c;
    
    } while (numRead != 0);
	buffer_read[--j] = '\0';
	idToPid[line_counter].pid = atoi(buffer_read);


    
    line_counter = 3;
    field_counter = 0;
    j = 0;

    // Lettura F9
    lseek(fd9, 15, SEEK_SET);

    do {
        numRead = read(fd9, &c, sizeof(char));
        if (c == ';' || c == '\n'||c == ',') {
            buffer_read[j] = '\0';
            j = 0;

            switch(field_counter) {

                case 0:
                    strcpy(idToPid[line_counter].id, buffer_read);
                    break;

                case 1:
                    idToPid[line_counter].pid = atoi(buffer_read);
                    break;
					
                default: 
                    break;
            }
        
            if (c == ';' || c == ',')
                field_counter++;
            if (c == '\n') {
                field_counter = 0;
                line_counter++;
            }

        } else if (c != '\n' && (c != ';' || c != ','))
            buffer_read[j++] = c;
    
    } while (numRead != 0);
    buffer_read[--j] = '\0';
	idToPid[line_counter].pid = atoi(buffer_read);

    // Hackler inizia l'invio dei segnali di disturbo
    j = 1;
    while (shutdown == 0 && j < nAction) {
        shutdown = handleAction(hack1[j], idToPid);
        j++;
    }
    
    // Attesa dei figli creati dall'hackler per la gestione dei segnali
    int status = 0;
    while ((wait(&status)) != -1){}
    

	close(fd8);
    close(fd9);
    
    semOp(semid, SEMCLS3, 2);
    return 0;
}
