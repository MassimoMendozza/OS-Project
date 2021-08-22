#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <semaphore.h>
#include <sys/shm.h>



#ifndef SHMUTILS_H
#define SHMUTILS_H
#include "shmUtils.h"
#endif
#ifndef TAXIELEMENTS_H
#include "TaxiElements.h"
#endif





int msgID;
int myClientNumber;

void bornAClient(int myNumber)
{
    /*printf("Client n%d with pid %d\n", myNumber, getpid());*/
    person *myself = getPerson(myNumber);
    myself->processid = getpid();
    myself->number = myNumber;
    myself->isOnTaxi = 0;

    myClientNumber=myNumber;

    
    message msgPlaceholder;

    clientKickoff();

    /*
        Waiting the simulation kickoff
    */

}

void clientKickoff(){
    /*printf("ALLAAAAAAAAAh, cliente n%d andato\n", myClientNumber);*/
    int goOn=1;
    
    message imHere;
    imHere.mtype=getpid();
    msgsnd(msgID, &imHere, sizeof(message), 0);
    struct timespec request = {0, 1000000};
    struct timespec remaining;
    while (goOn)
    {
        nanosleep(&request, &remaining);
    }
}