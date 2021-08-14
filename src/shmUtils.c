#include "Taxicab.h"
void *addrstart = -1;

void setAddrstart(void *start){
    addrstart = start;
}

void putMapInShm(masterMap *map)
{
    masterMap *shmMap = addrstart;
    shmMap->SO_CAP_MAX = map->SO_CAP_MAX;
    shmMap->SO_CAP_MIN = map->SO_CAP_MIN;
    shmMap->SO_DURATION = map->SO_DURATION;
    shmMap->SO_HEIGHT = map->SO_HEIGHT;
    shmMap->SO_HOLES = map->SO_HOLES;
    shmMap->SO_SOURCES = map->SO_SOURCES;
    shmMap->SO_TAXI = map->SO_TAXI;
    shmMap->SO_TIMENSEC_MAX = map->SO_TIMENSEC_MAX;
    shmMap->SO_TIMENSEC_MIN = map->SO_TIMENSEC_MIN;
    shmMap->SO_TIMEOUT = map->SO_TIMEOUT;
    shmMap->SO_TOP_CELLS = map->SO_TOP_CELLS;
    shmMap->SO_WIDTH = map->SO_WIDTH;
}

/*
    Allocates in shared memory the necessary space to memorize the whole structure like this:
    || masterMap | taxi[map->SO_TAXI] | person[map->SO_SOURCES] | mapCell[map->SO_WIDTH][map->SO_HEIGHT] | taxi[map->SO_WIDTH][map->SO_HEIGHT][map->SO_CAP_MAX] ||
    Returns the semID
*/
int allocateShm(int shmKey, masterMap *map){
    return shmget(shmKey, sizeof(masterMap) + sizeof(taxi[map->SO_TAXI]) + sizeof(person[map->SO_SOURCES]) + sizeof(mapCell[map->SO_WIDTH][map->SO_HEIGHT]) + sizeof(taxi[map->SO_WIDTH][map->SO_HEIGHT][map->SO_CAP_MAX]), IPC_CREAT | 0666);
}

masterMap *getMap(){
    return addrstart;
}

mapCell *getMapCellAt(int x, int y){
    return ((addrstart + sizeof(masterMap) + sizeof(taxi[getMap()->SO_TAXI]) + sizeof(person[getMap()->SO_SOURCES])) + (x * getMap()->SO_HEIGHT + y)*sizeof(mapCell));
}


taxi *getTaxi(int taxiNumber){
    return (addrstart+ sizeof(masterMap)+taxiNumber*sizeof(taxi));
}

person *getPerson(int personNumber){
    return (addrstart + sizeof(masterMap) + sizeof(taxi[getMap()->SO_TAXI]) + personNumber*sizeof(person));
}