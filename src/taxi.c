#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <signal.h>
#include <sys/types.h>



#ifndef SHMUTILS_H
#define SHMUTILS_H
#include "shmUtils.h"
#endif
#ifndef TAXIELEMENTS_H
#include "TaxiElements.h"
#endif



int msgID;
int myTaxiNumber;



void bornATaxi(int myNumber){
    
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

void alarmHandler(int signalNum){
    message kickoffMessage;

    kickoffMessage.type=MSG_KICKOFF;
    kickoffMessage.driverID=myTaxiNumber;
        kickoffMessage.mtype=(long)(MSG_TIMEOUT);
        if((msgsnd(msgID, &kickoffMessage,sizeof(kickoffMessage), 0))==-1){
            printf("Can't send message to signal I'm killing myself, taxi n%d", myTaxiNumber);
        };
    
    exit(EXIT_FAILURE);
}

void taxiKickoff(){
    printf("Yay, taxi n%d andato\n", myTaxiNumber);
/*
    struct sigaction idleAlarm;
    sigset_t idleMask;

    idleAlarm.sa_handler = &alarmHandler;
    idleAlarm.sa_flags=0;
    sigemptyset(&idleMask);
    sigaddset(&idleMask, SIGALRM);
    idleAlarm.sa_mask = idleMask;
*/

    if(signal(SIGALRM, &alarmHandler)==SIG_ERR){
        printf("Something's wrong on signal handler change");
    };
    alarm(1);
    while(1);
}