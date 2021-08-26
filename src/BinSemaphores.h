#ifndef BIN_SEM_H
#define BIN_SEM_H
#include "BinSemaphores.c"

int initSemAvailable(int semId, int semNum);
int initSemInUse(int semId, int semNum);
int reserveSem(int semId, int semNum);
int releaseSem(int semId, int semNum);
#endif