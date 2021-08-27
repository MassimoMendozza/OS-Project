#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <signal.h>
#include <errno.h>

#include "shmUtils.h"
#include "TaxiElements.h"
#include "BinSemaphores.h"

int myNumber, shmID, msgIDClientCall;
int myClientNumber;
person *myselfClient;

FILE *fp;
FILE *errorLog;

int goOn;
void kickoffClientHandler(int a)
{
    goOn = 0;
}

int main(int argc, char *argv[])
{
    /*printf("Client n%d with pid %d\n", myNumber, getpid());*/
    errorLog = fopen("errorLog.txt", "ab+");

    myNumber = atoi(argv[1]);
    shmID = atoi(argv[2]);
    msgIDClientCall = atoi(argv[3]);

    void *addrstart = shmat(shmID, NULL, 0);
    if (addrstart == -1)
    {
        strerror(errno);
    }
    setAddrstart(addrstart);

    myselfClient = getPerson(myNumber);
    myselfClient->processid = getpid();
    myselfClient->number = myNumber;
    myselfClient->isOnTaxi = 0;
    fp = fopen("movement.txt", "ab+");

    myClientNumber = myNumber;

    message msgPlaceholder;

    struct timespec request = {0, 1000000};
    struct timespec remaining;
    signal(SIGUSR1, &kickoffClientHandler);
    goOn = 1;
    while (goOn)
    {
        nanosleep(&request, &remaining);
    }

    clientKickoff();

    /*
        Waiting the simulation kickoff
    */
}

void clientKickoff()
{
    srand(getpid() % time(NULL));
    message imHere;

    FILE *fp;
    fp = fopen("try.txt", "ab+");
    struct timespec request = {0, 100000000};
    struct timespec remaining;

    /*
        sleep
        mando messaggio
        ricevo ok di presa in carico
        aspetto di ricevere messaggio esito 
        ricomincio
    */
    while (1)
    {

        message placeHolder;

        /* Cerco posizione in cui infilarmi */
        int sourceX, sourceY, destX, destY;
        int sourceFound, destFound;
        sourceFound = destFound = 0;

        for (sourceX = rand() % getMap()->SO_WIDTH; sourceX < getMap()->SO_WIDTH && !sourceFound; sourceX++)
        {
            for (sourceY = rand() % getMap()->SO_HEIGHT; sourceY < getMap()->SO_HEIGHT && !sourceFound; sourceY++)
            {
                if ((getMapCellAt(sourceX, sourceY)->maxElements != -1) && getMapCellAt(sourceX, sourceY)->isAvailable)
                {

                    if (reserveSem(getMap()->cellsSemID, (sourceX * getMap()->SO_HEIGHT) + sourceY) == -1)
                    {

                        fprintf(errorLog, "source:%d\tx:%d\ty:%d semerrorsource %s\n", myClientNumber, sourceX, sourceY, strerror(errno));
                        fflush(errorLog);
                    }
                    else
                    {
                        getMapCellAt(sourceX, sourceY)->isAvailable = 0;
                        sourceFound = 1;
                        /* for annidato per cella dest */
                        for (destX = rand() % getMap()->SO_WIDTH; destX < getMap()->SO_WIDTH && !destFound; destX++)
                        {
                            for (destY = rand() % getMap()->SO_HEIGHT; destY < getMap()->SO_HEIGHT && !destFound; destY++)
                            {
                                if ((getMapCellAt(destX, destY)->maxElements != -1) && getMapCellAt(destX, destY)->isAvailable)
                                {

                                    if (reserveSem(getMap()->cellsSemID, (destX * getMap()->SO_HEIGHT) + destY) == -1)
                                    {

                                        fprintf(errorLog, "source:%d\tx:%d\ty:%d semerrordest %s\n", myClientNumber, destX, destY, strerror(errno));
                                        fflush(errorLog);
                                    }
                                    else
                                    {
                                        getMapCellAt(destX, destY)->isAvailable = 0;
                                        destFound = 1;
                                        imHere.destX = destX;
                                        imHere.destY = destY;
                                        fprintf(fp, "%d sX:%d sY:%d dD:%d dY:%d\n", getpid(), sourceX, sourceY, destX, destY);
                                        fflush(fp);
                                        releaseSem(getMap()->cellsSemID, (destX * getMap()->SO_HEIGHT) + destY);
                                    };
                                }
                            }
                            if ((destX == getMap()->SO_WIDTH - 1) && (destY == getMap()->SO_HEIGHT ))
                            {
                                destX = destY = destFound=0;
                            }
                        }
                        imHere.sourceX = sourceX;
                        imHere.sourceY = sourceY;
                        releaseSem(getMap()->cellsSemID, (sourceX * getMap()->SO_HEIGHT) + sourceY);
                    };
                }
            }
            if ((sourceX == getMap()->SO_WIDTH - 1) && (sourceY == getMap()->SO_HEIGHT ))
            {
                sourceX = sourceY = sourceFound = 0;
            }
        }

        imHere.driverID=imHere.mtype=1;
        nanosleep(&request, &remaining);
        if (msgsnd(msgIDClientCall, &imHere, sizeof(message), 0) == -1)
        {
                        fprintf(errorLog, "source:%d\tx:%d\ty:%d msgsndclientcall %s\n", myClientNumber,sourceX, sourceY, strerror(errno));
                        fflush(errorLog);
        }
    }
}