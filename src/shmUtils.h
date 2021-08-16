
#ifndef SHMUTILS_H
#define SHMUTILS_H
#include "shmUtils.h"
#include "shmUtils.c"
#endif

#ifndef TAXIELEMENTS_H
#include "TaxiElements.h"
#endif


void putMapInShm(masterMap *map);
int allocateShm(int shmKey, masterMap *map);
masterMap *getMap();
mapCell *getMapCellAt(int x, int y);
taxi *getTaxi(int TaxiNumber);
person *getPerson(int personNumber);