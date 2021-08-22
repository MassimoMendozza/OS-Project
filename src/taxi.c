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
int goOn;
taxi *myself;

void kickoffHandler(int a)
{
    goOn = 0;
}

void bornATaxi(int myNumber)
{

    srand(getpid());

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
                    fprintf(stdout, "%s", strerror(errno));
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
    if ((myself->posX == -1) && (myself->posY == -1))
    {

        exit(EXIT_FAILURE);
    } /**/
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

void moveMyselfIn(int destX, int destY)
{
    int originX, originY;
    while ((myself->posX != destX) && (myself->posY != destY))
    {

        while (getMapCellAt(destX, destY)->maxElements == getMapCellAt(destX, destY)->currentElements)
            ;

        reserveSem(getMap()->cellsSemID, (destX * getMap()->SO_HEIGHT) + destY);
        reserveSem(getMap()->cellsSemID, (myself->posX * getMap()->SO_HEIGHT) + myself->posY);
        if (getMapCellAt(destX, destY)->maxElements > getMapCellAt(destX, destY)->currentElements)
        {

            getMapCellAt(originX, originY)->currentElements--;
            getMapCellAt(destX, destY)->currentElements++;
            myself->posX = destX;
            myself->posY = destY;
        }

        releaseSem(getMap()->cellsSemID, (originX * getMap()->SO_HEIGHT) + originY);
        releaseSem(getMap()->cellsSemID, (destX * getMap()->SO_HEIGHT) + destY);
    }

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

    if (signal(SIGALRM, &alarmHandler) == SIG_ERR)
    {
        printf("Something's wrong on signal handler change");
    };
    message imHere;
    imHere.mtype = MSG_KICKOFF;
    msgsnd(msgID, &imHere, sizeof(message), 0);
    alarm(getMap()->SO_TIMEOUT);

    msgrcv(msgID, &imHere, sizeof(message), MSG_CLIENT_CALL,0);
}