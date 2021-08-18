#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <signal.h>



#ifndef SHMUTILS_H
#define SHMUTILS_H
#include "shmUtils.h"
#endif
#ifndef TAXIELEMENTS_H
#include "TaxiElements.h"
#endif



int msgID;
int myTaxiNumber;



void bornATaxi(int myNumber)
{
    myTaxiNumber = myNumber;/*ti amo <3*/

    printf("Taxi n%d with pid %d\n", myNumber, getpid());

    taxi *myself = getTaxi(myNumber);
    myself->processid = getpid();
    myself->number = myNumber;
    myself->client = NULL;
    myself->distanceDone = 0;
    myself->ridesDone = 0;

    message msgPlaceholder;

    /*
        Waiting the simulation kickoff
    */

    if((msgrcv(msgID, &msgPlaceholder, sizeof(msgPlaceholder), getpid(),0))==-1){
        printf("Qualcosa non va bro");
        exit(EXIT_FAILURE);
    }else{
        if(msgPlaceholder.type==MSG_KICKOFF){
            
        taxiKickoff();
        }else{
            printf("%d\n", msgPlaceholder.type);
        }
    }


}

static void alarmHandler(int signalNum){
    printf("Taxi n%d si è suicidato\n", getpid());
    exit(EXIT_FAILURE);
}

void taxiKickoff(){
    printf("Yay, taxi n%d andato\n", myTaxiNumber);
    if(signal(SIGALRM, alarmHandler)==SIG_ERR){
        printf("Something's wrong on signal handler change");
    };
    alarm(1);
    while(1);
}