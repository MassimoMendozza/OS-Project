#include "BinSemaphores.c"

int initSemAvailable(int semId, int semNum);
int initSemInUse(int semId, int semNum);
int reserveSem(int semId, int semNum);
int relaseSem(int semId, int semNum);
