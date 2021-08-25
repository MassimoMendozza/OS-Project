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
person *myselfClient;

FILE *fp;

int goOn;
void kickoffClientHandler(int a)
{
    goOn = 0;
}

void bornAClient(int myNumber)
{
    /*printf("Client n%d with pid %d\n", myNumber, getpid());*/
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
    srand(getpid());
    message imHere;

    struct timespec request = {0, 500000000};
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
                if (getMapCellAt(sourceX, sourceY)->maxElements > -1 && getMapCellAt(sourceX, sourceY)->isAvailable)
                {

                    if (reserveSem(getMap()->cellsSemID, (sourceX * getMap()->SO_HEIGHT) + sourceY) == -1)
                    {
                        fprintf(stdout, "%s", strerror(errno));
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
                                if (getMapCellAt(destX, destY)->maxElements > -1 && getMapCellAt(destX, destY)->isAvailable)
                                {

                                    if (reserveSem(getMap()->cellsSemID, (destX * getMap()->SO_HEIGHT) + destY) == -1)
                                    {
                                        fprintf(stdout, "%s", strerror(errno));
                                    }
                                    else
                                    {
                                        getMapCellAt(destX, destY)->isAvailable = 0;
                                        destFound = 1;
                                        imHere.destX = destX;
                                        imHere.destY = destY;

                                        releaseSem(getMap()->cellsSemID, (destX * getMap()->SO_HEIGHT) + destY);
                                    };
                                }
                            }
                            if ((destX == getMap()->SO_WIDTH - 1) && (destY == getMap()->SO_HEIGHT - 1))
                            {
                                destX = destY = 0;
                            }
                        }
                        imHere.sourceX = sourceX;
                        imHere.sourceY = sourceY;
                        releaseSem(getMap()->cellsSemID, (sourceX * getMap()->SO_HEIGHT) + sourceY);
                    };
                }
            }
            if ((sourceX == getMap()->SO_WIDTH - 1) && (sourceY == getMap()->SO_HEIGHT - 1))
            {
                sourceX = sourceY = 0;
            }
        }

        imHere.clientID = myClientNumber;
        imHere.mtype = MSG_CLIENT_CALL;

        nanosleep(&request, &remaining);
        if (msgsnd(msgID, &imHere, sizeof(message), 0) == -1)
        {
            printf("%s", strerror(errno));
        }
    }
}