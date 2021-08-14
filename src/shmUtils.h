
#include "shmUtils.c"

void putMapInShm(masterMap *map);
int allocateShm(int shmKey, masterMap *map);
masterMap *getMap();
mapCell *getMapCellAt(int x, int y);