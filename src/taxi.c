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

int myNumber, shmID, duringRequest,
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

/*
Signal handler
*/
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

    alarmCame = duringRequest = 0;

    /*
    Installing signal handlers
    */
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = signalHandler;
    sigaction(SIGUSR1, &act, 0);
    sigaction(SIGALRM, &act, 0);
    sigaction(SIGTERM, &act, 0);

    errorLog = fopen("errorLog.txt", "ab+");

    /*
    Getting all necessary information for shm and setting it up
    */
    myNumber = atoi(argv[1]);
    shmID = atoi(argv[2]);
    void *addrstart = shmat(shmID, NULL, 0);
    setAddrstart(addrstart);
    msgIDKickoff = getMap()->msgIDKickoff;
    msgIDTimeout = getMap()->msgIDTimeout;
    msgIDTaxiCell = getMap()->msgIDTaxiCell;
    msgIDClientCall = getMap()->msgIDClientCall;
    msgIDClientTaken = getMap()->msgIDClientTaken;
    msgIDRequestBegin = getMap()->msgIDRequestBegin;
    msgIDRequestDone = getMap()->msgIDRequestDone;
    if (addrstart == -1)
    {
        fprintf(errorLog,"%s", strerror(errno));
    }
    srand(getpid() % time(NULL));

    /*
    Putting myself in shm
    */
    myTaxiNumber = myNumber; 
    myself = getTaxi(myNumber);
    while (getpid() == 0)
        ;
    myself->processid = getpid();
    myself->number = myNumber;
    myself->distanceDone = 0;
    myself->requestsTaken = 0;
    myself->posX = -1;
    myself->posY = -1;

    /*
    Searching a position for myself
    */
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

    /*
    Sending message for taken position
    */
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
    
    /*
    Waiting for SIGUSR1 to kickoff the simulation
    */
    while (goOn)
    {
        nanosleep(&request, &remaining);
    }

    /*
    Simulation begin
    */
    taxiKickoff();
}

/*
Function to communicate quitting
*/
void quitItAll()
{
    keepOn = 0;
    message killNotification;
    killNotification.driverID = myTaxiNumber;
    killNotification.x =
        killNotification.y =
            killNotification.mtype = 1;
    if (duringRequest)
    {
        killNotification.x = -1;
    }
    msgsnd(msgIDTimeout, &killNotification, sizeof(message), 0);
    reserveSem(getMap()->cellsSemID, (myself->posX * getMap()->SO_HEIGHT) + myself->posY);
    getMapCellAt(myself->posX, myself->posY)->currentElements--;
    releaseSem(getMap()->cellsSemID, (myself->posX * getMap()->SO_HEIGHT) + myself->posY);
    exit(EXIT_SUCCESS);
}

/*
Function for moving the taxi in a cell by reserving the old and new cells' semaphores
and transitioning the current elements count
*/
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

/*
Driving taxi alternating a movement on x and a movement on Y
*/
void driveTaxi(int destX, int destY)
{
    while (!((myself->posX == destX) && (myself->posY == destY)))
    {
        moveOnX(destX, destY);
        moveOnY(destX, destY);
    }
}

/*
Function to move myself on x nearer to destination
*/
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

/*
Function to move myself on y nearer to destination
*/
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
    
    /*
    Sending message to notifying kickoff
    */
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

    
    while (keepOn && !alarmCame)
    {
        if (alarmCame)
            quitItAll();
        tempTime = 0;
        sourceX = sourceY = destX = destY = -1;

        /*
        Waiting for requests
        */
        message requestPlaceholder;
        if (msgrcv(msgIDClientCall, &requestPlaceholder, sizeof(message), 0, 0) == -1)
        {

            fprintf(errorLog, "taxi:%d\tx:%d\ty:%d rcvclicall %s\n", myTaxiNumber, myself->posX, myself->posY, strerror(errno));
            fflush(errorLog);
        };
        duringRequest = 1;
        if (alarmCame)
            quitItAll();
        sourceX = getPerson(requestPlaceholder.clientID)->posX;
        sourceY = getPerson(requestPlaceholder.clientID)->posY;
        destX = requestPlaceholder.x;
        destY = requestPlaceholder.y;

        requestPlaceholder.driverID = myTaxiNumber;

        /*
        Notifying that I've taken a client
        */
        requestPlaceholder.mtype = 1;
        if (msgsnd(msgIDClientTaken, &requestPlaceholder, sizeof(message), 0) == -1)
        {
            fprintf(errorLog, "taxi:%d\tx:%d\ty:%d sndclitak %s\n", myTaxiNumber, sourceX, sourceY, strerror(errno));
            fflush(errorLog);
        };
        myself->requestsTaken++;

        if (alarmCame)
            quitItAll();
        /*
        Driving the taxi to client position
        */
        driveTaxi(sourceX, sourceY);
        if (alarmCame)
            quitItAll();

        /*
        Checking if it has been done in a bgger time than before
        */
        if (myself->maxTime < tempTime)
        {
            myself->maxTime = tempTime;
        }
        tempTime = 0;
        requestPlaceholder.mtype = 1;

        /*
        Notifying that I'm at client position
        */
        if (msgsnd(msgIDRequestBegin, &requestPlaceholder, sizeof(message), 0) == -1)
        {

            fprintf(errorLog, "taxi:%d\tx:%d\ty:%d sndreqbeg %s\n", myTaxiNumber, sourceX, sourceY, strerror(errno));
            fflush(errorLog);
        };

        if (alarmCame)
            quitItAll();
        /*
        Driving to destination
        */
        driveTaxi(destX, destY);
        if (alarmCame)
            quitItAll();

        alarm(getMap()->SO_TIMEOUT);

        
        /*
        Notifying that I'm at destination
        */
        requestPlaceholder.mtype = 1;
        if (msgsnd(msgIDRequestDone, &requestPlaceholder, sizeof(message), 0) == -1)
        {

            fprintf(errorLog, "taxi:%d\tx:%d\ty:%d sndreqdon %s\n", myTaxiNumber, sourceX, sourceY, strerror(errno));
            fflush(errorLog);
        };
        duringRequest = 0;

        /*
        Checking if it has been done in a bgger time than before
        */
        if (myself->maxTime < tempTime)
        {
            myself->maxTime = tempTime;
        }
    }

    quitItAll();
}