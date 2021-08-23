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

    int x, y;
    int found = 0;

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
                    if (getMapCellAt(x, y)->client == NULL)
                    {
                        getMapCellAt(x, y)->client = myself;
                        found = 1;
                        myselfClient->posX = x;
                        myselfClient->posY = y;
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
    }

    clientKickoff();

    /*
        Waiting the simulation kickoff
    */
}

void clientKickoff()
{

    message imHere;
    imHere.mtype = getpid();

    struct timespec request = {0, 1000000};
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
        nanosleep(&request, &remaining);
        imHere.clientID = myClientNumber;
        imHere.mtype = MSG_CLIENT_CALL;
        imHere.sourceX = myself->posX;
        imHere.sourceY = myself->posY;
        imHere.type = MSG_CLIENT_CALL;

        message placeHolder;

        msgsnd(msgID, &imHere, sizeof(message), 0);
        msgrcv(msgID, &placeHolder, sizeof(message), getpid(), 0);
        msgrcv(msgID, &placeHolder, sizeof(message), getpid(), 0);

        /*

        mettere dato in cella che dice che è già presa per destinazione
        if(ok){
            aggiorna posizione
            int x, y;
    int found = 0;

    for (x = rand() % getMap()->SO_WIDTH; x < getMap()->SO_WIDTH && !found; x++)
    {
        for (y = rand() % getMap()->SO_HEIGHT; y < getMap()->SO_HEIGHT && !found; y++)
        {
            if (getMapCellAt(x, y)->maxElements > -1)
            {
                if (myself->posX != -1 && myself->posY != -1)
                {

                    if (reserveSem(getMap()->cellsSemID, (myself->posX * getMap()->SO_HEIGHT) + myself->posY) == -1)
                    {
                        fprintf(stdout, "%s", strerror(errno));
                    }
                }

                if (reserveSem(getMap()->cellsSemID, (x * getMap()->SO_HEIGHT) + y) == -1)
                {
                    fprintf(stdout, "%s", strerror(errno));
                }
                else
                {
                    if (getMapCellAt(x, y)->client == NULL)
                    {
                        getMapCellAt(x, y)->client = myself;
                        found = 1;
                    }
                    releaseSem(getMap()->cellsSemID, (x * getMap()->SO_HEIGHT) + y);
                    if (myself->posX != -1 && myself->posY != -1)
                    {

                        getMapCellAt(myself->posX, myself->posY)->client = NULL;
                        if (releaseSem(getMap()->cellsSemID, (myself->posX * getMap()->SO_HEIGHT) + myself->posY) == -1)
                        {
                            fprintf(stdout, "%s", strerror(errno));
                        }
                    }
                    myself->posX = x;
                    myself->posY = y;
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
    }

        }else{
            lascia perdere
        }

        come genero e tengo d'occhio la destinazione?
        */
    }
}