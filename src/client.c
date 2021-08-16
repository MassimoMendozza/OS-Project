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
    printf("Client n%d with pid %d\n", myNumber, getpid());
    person *myself = getPerson(myNumber);
    myself->processid = getpid();
    myself->number = myNumber;
    myself->isOnTaxi = 0;

    myClientNumber=myNumber;

    
    message msgPlaceholder;

    /*
        Waiting the simulation kickoff
    */

    if((msgrcv(msgID, &msgPlaceholder, sizeof(msgPlaceholder), (long)(myself->processid),0))==-1){
        printf("Qualcosa non va bro");
        exit(EXIT_FAILURE);
    }else{
        if(msgPlaceholder.type==MSG_KICKOFF){
            
        clientKickoff();
        }else{
            printf("%d\n", msgPlaceholder.type);
        }
    }
}

void clientKickoff(){
    printf("ALLAAAAAAAAAh, cliente n%d andato\n", myClientNumber);
}