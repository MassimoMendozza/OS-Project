#include <sys/sem.h>

/*
Utils funciton to release and reserve bin semaphores
*/

union semun
{
	int val;
	struct semid_ds *buf;
	unsigned short *array;
	#if defined(__linux__)
	struct seminfo *__buf;
	#endif
};

int reserveSem(int semId, int semNum)
{
	struct sembuf sops;
	sops.sem_num = semNum;
	sops.sem_op = -1;
	sops.sem_flg = 0;
	return semop(semId, &sops, 1);
}

int releaseSem(int semId, int semNum)
{
	struct sembuf sops;
	sops.sem_num = semNum;
	sops.sem_op = 1;
	sops.sem_flg = 0;
	return semop(semId, &sops, 1);
}
