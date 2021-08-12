#include <sys/sem.h>

/*
semun:
	val -> value for SETVAL
	buf -> buffer for IPC_STAT, IPC_SET
	array -> array for GETALL, SETALL
	__buf -> buffer fot IPC_INFO
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

int initSemAvailable(int semId, int semNum)
{
	union semun arg;
	arg.val = 1;
	return semctl(semId, semNum, SETVAL, arg);
}

int initSemInUse(int semId, int semNum)
{
	union semun arg;
	arg.val = 0;
	return semctl(semId, semNum, SETVAL, arg);
}

int reserveSem(int semId, int semNum)
{
	struct sembuf sops;
	sops.sem_num = semNum;
	sops.sem_op = -1;
	sops.sem_flg = 0;
	return semop(semId, &sops, 1);
}

int relaseSem(int semId, int semNum)
{
	struct sembuf sops;
	sops.sem_num = semNum;
	sops.sem_op = 1;
	sops.sem_flg = 0;
	return semop(semId, &sops, 1);
}
