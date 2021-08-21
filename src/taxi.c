#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <signal.h>

#include <time.h>
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
taxi *myself;

void bornATaxi(int myNumber)
{

    myTaxiNumber = myNumber; /*ti amo <3*/

    printf("Taxi n%d with pid %d\n", myNumber, getpid());

    myself = getTaxi(myNumber);
    myself->processid = getpid();
    myself->number = myNumber;
    myself->client = NULL;
    myself->distanceDone = 0;
    myself->ridesDone = 0;

    message msgPlaceholder;

    /*
        Waiting the simulation kickoff
    */

    if ((msgrcv(msgID, &msgPlaceholder, sizeof(msgPlaceholder), getpid(), 0)) == -1)
    {
        printf("Qualcosa non va bro");
        exit(EXIT_FAILURE);
    }
    else
    {
        if (msgPlaceholder.type == MSG_KICKOFF)
        {

            taxiKickoff();
        }
        else
        {
            printf("%d\n", msgPlaceholder.type);
        }
    }
}

void moveMyselfIn(int destX, int destY){
    while(getMapCellAt(destX,destY)->maxElements==getMapCellAt(destX, destY)->currentElements);
    reserveSem(getMap()->cellsSemID, (destX*getMap()->SO_HEIGHT)+destY);
    reserveSem(getMap()->cellsSemID, (myself->posX*getMap()->SO_HEIGHT)+myself->posY);
    getMapCellAt(myself->posX, myself->posY)->currentElements--;
    getMapCellAt(destX, destY)->currentElements++;
    releaseSem(getMap()->cellsSemID, (myself->posX*getMap()->SO_HEIGHT)+myself->posY);
    releaseSem(getMap()->cellsSemID, (destX*getMap()->SO_HEIGHT)+destY);
    myself->posX=destX; myself->posY=destY;
    alarm(getMap()->SO_TIMEOUT);
    struct timespec request = {0, getMapCellAt(destX, destY)->holdingTime};
    struct timespec remaining;
    nanosleep(&request, &remaining);
};

void driveTaxi(int destX, int destY)
{
    moveOnX(destX, destY);
    moveOnY(destX, destY);
}

void moveOnX(int destX, int destY)
{
    while (!(destX == myself->posX))
    {
        if ((destY == myself->posY))
        {
            int xToGo = myself->posX + (-1 + ((myself->posX - 1) == -1) * 2);
            moveMyselfIn(xToGo, myself->posY);
            moveOnY(destX, destY);
        }
        int xToGo = myself->posX + ((1) * (myself->posX < destX) + (-1) * (myself->posX > destX));
        if (getMapCellAt(xToGo, myself->posY)->maxElements == -1)
        {
            moveOnY(destX, destY);
        }
    }
}

void moveOnY(int destX, int destY)
{
    while (!(destY == myself->posY))
    {
        if ((destX == myself->posX))
        {
            int yToGo = myself->posY + (-1 + ((myself->posY - 1) == -1) * 2);
            moveMyselfIn(myself->posX, yToGo);
            moveOnX(destX, destY);
        }
        int yToGo = myself->posY + ((1) * (myself->posY < destY) + (-1) * (myself->posY > destY));
        if (getMapCellAt(myself->posX, yToGo)->maxElements == -1)
        {
            moveOnX(destX, destY);
        }
    }
}

static void alarmHandler(int signalNum)
{
    message killNotification;
    killNotification.driverID = myTaxiNumber;
    killNotification.type = MSG_TIMEOUT;
    killNotification.mtype = MSG_TIMEOUT;
    msgsnd(msgID, &killNotification, sizeof(message), 0);
    exit(EXIT_FAILURE);
}

void taxiKickoff()
{
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

    if (signal(SIGALRM, &alarmHandler) == SIG_ERR)
    {
        printf("Something's wrong on signal handler change");
    };
    alarm(getMap()->SO_TIMEOUT);
    while (1)
        ;
}