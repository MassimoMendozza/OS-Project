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
#include <sys/msg.h>

#include "shmUtils.h"
#include "TaxiElements.h"
#include "BinSemaphores.h"

int myNumber, shmID,
    msgIDKickoff,
    msgIDTimeout,
    msgIDTaxiCell,
    msgIDClientCall,
    msgIDClientTaken,
    msgIDRequestBegin,
    msgIDRequestDone;
long tempTime;
int myTaxiNumber;
int goOn;
int keepOn;
int alarmCame;
taxi *myself;
FILE *errorLog;

int sourceX, sourceY, destX, destY;
alarmInsideSem = 0;

void signalHandler(int sigNumber)
{
    switch (sigNumber)
    {
    case SIGUSR1:
        goOn = 0;
        break;
    case SIGTERM:
    case SIGALRM:
        alarmCame = 1;
        break;
    }
}
int main(int argc, char *argv[])
{

    alarmCame = 0;

    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = signalHandler;
    sigaction(SIGUSR1, &act, 0);
    sigaction(SIGALRM, &act, 0);
    sigaction(SIGTERM, &act, 0);

    errorLog = fopen("errorLog.txt", "ab+");

    myNumber = atoi(argv[1]);
    shmID = atoi(argv[2]);
    msgIDKickoff = atoi(argv[3]);
    msgIDTimeout = atoi(argv[4]);
    msgIDTaxiCell = atoi(argv[5]);
    msgIDClientCall = atoi(argv[6]);
    msgIDClientTaken = atoi(argv[7]);
    msgIDRequestBegin = atoi(argv[8]);
    msgIDRequestDone = atoi(argv[9]);

    void *addrstart = shmat(shmID, NULL, 0);
    if (addrstart == -1)
    {
        strerror(errno);
    }
    setAddrstart(addrstart);
    srand(getpid() % time(NULL));

    myTaxiNumber = myNumber; /*ti amo <3*/

    /*printf("Taxi n%d with pid %d\n", myNumber, getpid());*/

    myself = getTaxi(myNumber);
    while(getpid()==0);
    myself->processid = getpid();
    myself->number = myNumber;
    myself->distanceDone = 0;
    myself->requestsTaken = 0;
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

                    fprintf(errorLog, "taxi:%d\tx:%d\ty:%d semerrorcellrandsearch %s\n", myTaxiNumber, x, y, strerror(errno));
                    fflush(errorLog);
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
        if ((x == getMap()->SO_WIDTH - 1) && (y == getMap()->SO_HEIGHT))
        {
            x = y = found = 0;
        }
    }
    if ((myself->posX == -1) || (myself->posY == -1))
    {

        fprintf(errorLog, "taxi:%d\tx:%d\ty:%d noplace4me %s\n", myTaxiNumber, sourceX, sourceY, strerror(errno));
        fflush(errorLog);
        exit(EXIT_FAILURE);
    }
    message positionMessage;
    positionMessage.driverID = myTaxiNumber;
    positionMessage.x =
        positionMessage.y =
            positionMessage.mtype = 1;
    if (msgsnd(msgIDTaxiCell, &positionMessage, sizeof(message), 0) == -1)
    {
        fprintf(errorLog, "taxi:%d\tx:%d\ty:%d sndtxicell %s\n", myTaxiNumber, myself->posX, myself->posY, strerror(errno));
        fflush(errorLog);
    };

    message msgPlaceholder;

    struct timespec request = {0, 1000000};
    struct timespec remaining;
    while (goOn)
    {
        nanosleep(&request, &remaining);
    }

    taxiKickoff();
}

void quitItAll()
{
    keepOn=0;
    message killNotification;
    killNotification.driverID = myTaxiNumber;
    killNotification.x =
        killNotification.y =
            killNotification.mtype = 1;
    msgsnd(msgIDTimeout, &killNotification, sizeof(message), 0);
    reserveSem(getMap()->cellsSemID, (myself->posX * getMap()->SO_HEIGHT) + myself->posY);
    getMapCellAt(myself->posX, myself->posY)->currentElements--;
    releaseSem(getMap()->cellsSemID, (myself->posX * getMap()->SO_HEIGHT) + myself->posY);
    exit(EXIT_SUCCESS);
}

void moveMyselfIn(int destX, int destY)
{
    int originX, originY;
    originX = myself->posX;
    originY = myself->posY;
    if (alarmCame)
        quitItAll();
    reserveSem(getMap()->cellsSemID, (destX * getMap()->SO_HEIGHT) + destY);
    reserveSem(getMap()->cellsSemID, (myself->posX * getMap()->SO_HEIGHT) + myself->posY);
    if (getMapCellAt(destX, destY)->maxElements > getMapCellAt(destX, destY)->currentElements)
    {

        getMapCellAt(originX, originY)->currentElements--;
        getMapCellAt(destX, destY)->currentElements++;
        getMapCellAt(destX, destY)->passedBy++;
        myself->posX = destX;
        myself->posY = destY;
    }

    releaseSem(getMap()->cellsSemID, (originX * getMap()->SO_HEIGHT) + originY);
    releaseSem(getMap()->cellsSemID, (destX * getMap()->SO_HEIGHT) + destY);

    if (alarmCame)
        quitItAll();
    if ((myself->posX == destX) && (myself->posY == destY))
    {
        alarm(getMap()->SO_TIMEOUT);
    }

    struct timespec request = {0, getMapCellAt(destX, destY)->holdingTime};
    struct timespec remaining;
    tempTime = tempTime + getMapCellAt(destX, destY)->holdingTime;
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
        if (xToGo != myself->posX)
        {

            if (getMapCellAt(xToGo, myself->posY)->maxElements != -1)
            {
                moveMyselfIn(xToGo, myself->posY);
                if (myself->posX == xToGo)
                {
                    myself->distanceDone++;
                }
            }
        }
    }
}

void moveOnY(int destX, int destY)
{
    if (!((myself->posX == destX) && (myself->posY == destY)))
    {
        int yToGo = myself->posY + (1 * (destY > myself->posY)) + ((-1) * (destY < myself->posY));
        if (yToGo != myself->posY)
        {
            if (getMapCellAt(myself->posX, yToGo)->maxElements != -1)
            {
                moveMyselfIn(myself->posX, yToGo);
                if (myself->posY == yToGo)
                {
                    myself->distanceDone++;
                }
            }
        }
    }
}

void taxiKickoff()
{
    keepOn = 1;

    message imHere;
    imHere.driverID = myself->number;
    imHere.x =
        imHere.y =
            imHere.mtype = 1;
    if (msgsnd(msgIDKickoff, &imHere, sizeof(message), 0) == -1)
    {

        fprintf(errorLog, "taxi:%d\tx:%d\ty:%d sndtxikick %s\n", myTaxiNumber, sourceX, sourceY, strerror(errno));
        fflush(errorLog);
    };

    /*Taxi si mette in attesa di richieste*/
    while (keepOn&&!alarmCame)
    {
        if (alarmCame)
            quitItAll();
        tempTime = 0;
        sourceX = sourceY = destX = destY = -1;
        message requestPlaceholder;
        if (msgrcv(msgIDClientCall, &requestPlaceholder, sizeof(message), 0, 0) == -1)
        {

            fprintf(errorLog, "taxi:%d\tx:%d\ty:%d rcvclicall %s\n", myTaxiNumber, myself->posX, myself->posY, strerror(errno));
            fflush(errorLog);
        };
        if (alarmCame)
            quitItAll();
        sourceX = getPerson(requestPlaceholder.clientID)->posX;
        sourceY = getPerson(requestPlaceholder.clientID)->posY;
        destX = requestPlaceholder.x;
        destY = requestPlaceholder.y;

        requestPlaceholder.driverID = myTaxiNumber;

        requestPlaceholder.mtype = 1;
        if (msgsnd(msgIDClientTaken, &requestPlaceholder, sizeof(message), 0) == -1)
        {
            fprintf(errorLog, "taxi:%d\tx:%d\ty:%d sndclitak %s\n", myTaxiNumber, sourceX, sourceY, strerror(errno));
            fflush(errorLog);
        };
        myself->requestsTaken++;

        if (alarmCame)
            quitItAll();
        driveTaxi(sourceX, sourceY);
        if (alarmCame)
            quitItAll();
        if (myself->maxTime < tempTime)
        {
            myself->maxTime = tempTime;
        }
        tempTime = 0;
        requestPlaceholder.mtype = 1;
        if (msgsnd(msgIDRequestBegin, &requestPlaceholder, sizeof(message), 0) == -1)
        {

            fprintf(errorLog, "taxi:%d\tx:%d\ty:%d sndreqbeg %s\n", myTaxiNumber, sourceX, sourceY, strerror(errno));
            fflush(errorLog);
        };

        if (alarmCame)
            quitItAll();
        driveTaxi(destX, destY);
        if (alarmCame)
            quitItAll();
        alarm(getMap()->SO_TIMEOUT);

        requestPlaceholder.mtype = 1;
        if (msgsnd(msgIDRequestDone, &requestPlaceholder, sizeof(message), 0) == -1)
        {

            fprintf(errorLog, "taxi:%d\tx:%d\ty:%d sndreqdon %s\n", myTaxiNumber, sourceX, sourceY, strerror(errno));
            fflush(errorLog);
        };
        if (myself->maxTime < tempTime)
        {
            myself->maxTime = tempTime;
        }
    }

            quitItAll();
}