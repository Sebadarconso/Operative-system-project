/// @file shared_memory.h
/// @brief Contiene la definizioni di variabili e funzioni
///         specifiche per la gestione della MEMORIA CONDIVISA.

#pragma once

#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>

// the Request structure defines a request sent by a client
struct Request {
    char pathname[250];
    key_t shmKey;
};

int alloc_shared_memory(key_t shmKey, size_t size);

void *get_shared_memory(int shmid, int shmflg);

void free_shared_memory(void *ptr_sh);

void remove_shared_memory(int shmid);