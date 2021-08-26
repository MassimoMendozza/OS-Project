#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <signal.h>

#include <time.h>
#include <sys/types.h>
#include <errno.h>

#include "shmUtils.h"
#include "TaxiElements.h"
#include "BinSemaphores.h"

int myNumber, shmID, msgID;
int myTaxiNumber;
int goOn;
int alarmInsideSem;
taxi *myself;
FILE *fp;

int sourceX, sourceY, destX, destY;
alarmInsideSem = 0;

void cautiousHandler(int a)
{
    alarmInsideSem = 1;
    signal(SIGALRM, &cautiousHandler);
}

void kickoffHandler(int a)
{
    goOn = 0;
}

int main(int argc, char *argv[])
{
    myNumber = atoi(argv[1]);
    shmID = atoi(argv[2]);
    msgID = atoi(argv[3]);

    void *addrstart = shmat(shmID, NULL, 0);
    if (addrstart == -1)
    {
        strerror(errno);
    }
    setAddrstart(addrstart);
    srand(getpid());
    fp = fopen("movement.txt", "ab+");

    myTaxiNumber = myNumber; /*ti amo <3*/

    /*printf("Taxi n%d with pid %d\n", myNumber, getpid());*/

    myself = getTaxi(myNumber);
    myself->processid = getpid();
    myself->number = myNumber;
    myself->client = NULL;
    myself->distanceDone = 0;
    myself->ridesDone = 0;
    myself->posX = -1;
    myself->posY = -1;

    signal(SIGUSR1, &kickoffHandler);

    int found, x, y;
    found = 0;
    goOn = 1;
    for (x = rand() % getMap()->SO_WIDTH; x < getMap()->SO_WIDTH && !found; x++)
    {
        for (y = rand() % getMap()->SO_HEIGHT; y < getMap()->SO_HEIGHT && !found; y++)
        {
            if (getMapCellAt(x, y)->maxElements > -1)
            {

                if (reserveSem(getMap()->cellsSemID, (x * getMap()->SO_HEIGHT) + y) == -1)
                {
                    /*fprintf(stdout, "%s", strerror(errno));*/
                }
                else
                {
                    if (getMapCellAt(x, y)->maxElements > getMapCellAt(x, y)->currentElements)
                    {
                        getMapCellAt(x, y)->currentElements++;
                        myself->posX = x;
                        myself->posY = y;
                        found = 1;
                    }
                    releaseSem(getMap()->cellsSemID, (x * getMap()->SO_HEIGHT) + y);
                };
            }
        }
        if ((x == getMap()->SO_WIDTH - 1) && (y == getMap()->SO_HEIGHT - 1))
        {
            x = y = 0;
        }
    }
    /*if ((myself->posX == -1) && (myself->posY == -1))
    {

        exit(EXIT_FAILURE);
    } */
    message positionMessage;
    positionMessage.mtype = MSG_TAXI_CELL;
    positionMessage.driverID = myTaxiNumber;
    positionMessage.sourceX = myself->posX;
    positionMessage.sourceY = myself->posY;
    msgsnd(msgID, &positionMessage, sizeof(message), 0);

    message msgPlaceholder;

    struct timespec request = {0, 1000000};
    struct timespec remaining;
    while (goOn)
    {
        nanosleep(&request, &remaining);
    }

    taxiKickoff();
}

static void alarmHandler(int signalNum)
{
    message killNotification;
    killNotification.driverID = myTaxiNumber;
    killNotification.sourceX = myself->posX;
    killNotification.sourceY = myself->posY;
    killNotification.destX = destX;
    killNotification.destY = destY;
    killNotification.type = MSG_TIMEOUT;
    killNotification.mtype = MSG_TIMEOUT;
    msgsnd(msgID, &killNotification, sizeof(message), 0);
    reserveSem(getMap()->cellsSemID, ( myself->posX * getMap()->SO_HEIGHT) +  myself->posY);
    getMapCellAt( myself->posX,  myself->posY)->currentElements--;
    releaseSem(getMap()->cellsSemID, ( myself->posX * getMap()->SO_HEIGHT) +  myself->posY);
    if ((destX != -1))
    {
        reserveSem(getMap()->cellsSemID, (destX * getMap()->SO_HEIGHT) +destY);
        getMapCellAt(destX, destY)->isAvailable = 1;
        releaseSem(getMap()->cellsSemID, (destX * getMap()->SO_HEIGHT) + destY);
    }
    fclose(fp);
    raise(SIGKILL);
}
void moveMyselfIn(int destX, int destY)
{
    int originX, originY;
    originX = myself->posX;
    originY = myself->posY;

    signal(SIGALRM, &cautiousHandler);
    reserveSem(getMap()->cellsSemID, (destX * getMap()->SO_HEIGHT) + destY);
    reserveSem(getMap()->cellsSemID, (myself->posX * getMap()->SO_HEIGHT) + myself->posY);
    if (getMapCellAt(destX, destY)->maxElements > getMapCellAt(destX, destY)->currentElements)
    {

        getMapCellAt(originX, originY)->currentElements--;
        getMapCellAt(destX, destY)->currentElements++;
        myself->posX = destX;
        myself->posY = destY;
    }
    else
    {

        /* */fprintf(fp, "%d\tWAITING to enter x:%d y:%d\n", getpid(), destX, destY); 
    }

    releaseSem(getMap()->cellsSemID, (originX * getMap()->SO_HEIGHT) + originY);
    releaseSem(getMap()->cellsSemID, (destX * getMap()->SO_HEIGHT) + destY);
    signal(SIGALRM, &alarmHandler);
    if (alarmInsideSem)
    {
        raise(SIGALRM);
    }
    alarm(getMap()->SO_TIMEOUT);

    struct timespec request = {0, getMapCellAt(destX, destY)->holdingTime};
    struct timespec remaining;
    /* */fprintf(fp, "%d\tx:%d y:%d\n", getpid(), myself->posX, myself->posY); 
    /*  kill(getMap()->masterProcessID, SIGUSR1);*/
    nanosleep(&request, &remaining);
};

void driveTaxi(int destX, int destY)
{
    while (!((myself->posX == destX) && (myself->posY == destY)))
    {
        moveOnX(destX, destY);
        moveOnY(destX, destY);
    }
}

void moveOnX(int destX, int destY)
{
    if (!((myself->posX == destX) && (myself->posY == destY)))
    {
        int xToGo = myself->posX + (1 * (destX > myself->posX)) + ((-1) * (destX < myself->posX));
        if (xToGo == myself->posX)
        {
            xToGo = myself->posX - 1 + (2 * (myself->posX == 0));
        }
        else
        {

            if (getMapCellAt(xToGo, myself->posY)->maxElements != -1)
            {

                moveMyselfIn(xToGo, myself->posY);
            }
        }
    }
}

void moveOnY(int destX, int destY)
{
    if (!((myself->posX == destX) && (myself->posY == destY)))
    {
        int yToGo = myself->posY + (1 * (destY > myself->posY)) + ((-1) * (destY < myself->posY));
        if (yToGo == myself->posY)
        {
            yToGo = myself->posY - 1 + (2 * (myself->posY == 0));
        }
        else
        {

            if (getMapCellAt(myself->posX, yToGo)->maxElements != -1)
            {
                moveMyselfIn(myself->posX, yToGo);
            }
        }
    }
}

void taxiKickoff()
{

    if (signal(SIGALRM, &alarmHandler) == SIG_ERR)
    {
        printf("Something's wrong on signal handler change");
    };
    message imHere;
    imHere.mtype = MSG_KICKOFF;
    msgsnd(msgID, &imHere, sizeof(message), 0);
    alarm(getMap()->SO_TIMEOUT);

    /*Taxi si mette in attesa di richieste*/
    while (1)
    {
        sourceX = sourceY = destX = destY = -1;
        message requestPlaceholder;
        msgrcv(msgID, &requestPlaceholder, sizeof(message), MSG_CLIENT_CALL, 0);

        sourceX = requestPlaceholder.sourceX;
        sourceY = requestPlaceholder.sourceY;
        destX = requestPlaceholder.destX;
        destY = requestPlaceholder.destY;

        requestPlaceholder.driverID = myTaxiNumber;
        requestPlaceholder.mtype = MSG_CLIENT_TAKEN;

        /* */fprintf(fp, "%d\tx:%d y:%d\tRICHIESTA\n", getpid(), sourceX, sourceY); 
        msgsnd(msgID, &requestPlaceholder, sizeof(message), 0);

        driveTaxi(sourceX, sourceY);
        /* making source cell available again */
        signal(SIGALRM, &cautiousHandler);
        reserveSem(getMap()->cellsSemID, (myself->posX * getMap()->SO_HEIGHT) + myself->posY);

        getMapCellAt(myself->posX, myself->posY)->isAvailable = 1;

        releaseSem(getMap()->cellsSemID, (myself->posX * getMap()->SO_HEIGHT) + myself->posY);
        signal(SIGALRM, &alarmHandler);
        if (alarmInsideSem)
        {
            raise(SIGALRM);
        }
        alarm(getMap()->SO_TIMEOUT);
/*  */fprintf(fp, "%d\tx:%d y:%d\PRELEVATO\n", getpid(), sourceX, sourceY);
        
        requestPlaceholder.mtype = MSG_REQUEST_BEGIN;
        msgsnd(msgID, &requestPlaceholder, sizeof(message), 0);

        driveTaxi(destX, destY);
        /* making end cell available again */
        signal(SIGALRM, &cautiousHandler);
        reserveSem(getMap()->cellsSemID, (myself->posX * getMap()->SO_HEIGHT) + myself->posY);

        getMapCellAt(myself->posX, myself->posY)->isAvailable = 1;

        releaseSem(getMap()->cellsSemID, (myself->posX * getMap()->SO_HEIGHT) + myself->posY);
        signal(SIGALRM, &alarmHandler);
        if (alarmInsideSem)
        {
            raise(SIGALRM);
        }
        alarm(getMap()->SO_TIMEOUT);
        /* */ fprintf(fp, "%d\tx:%d y:%d\ARRIVATO\n", getpid(), sourceX, sourceY);
        
        requestPlaceholder.mtype = MSG_REQUEST_DONE;
        msgsnd(msgID, &requestPlaceholder, sizeof(message), 0);
    }
}