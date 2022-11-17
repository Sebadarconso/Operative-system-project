/*
/// @file semaphore.h
/// @brief Contiene le definizioni di variabili e funzioni
///         specifiche per la gestione dei SEMAFORI.
*/

#ifndef _SEMAPHORE_HH
#define _SEMAPHORE_HH

// definition of the union semun
union semun {
    int val;
    struct semid_ds * buf;
    unsigned short * array;
};


void semOp (int semid, unsigned short sem_num, short sem_op);

#endif
