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
int myNumber;
person *myself;

FILE *fp;
FILE *errorLog;

int goOn;
void kickoffClientHandler(int a)
{
    goOn = 0;
}

int main(int argc, char *argv[])
{
    srand(getpid() % time(NULL));
    errorLog = fopen("errorLog.txt", "ab+");


    signal(SIGUSR1, &kickoffClientHandler);

    myNumber = atoi(argv[1]);
    shmID = atoi(argv[2]);
    msgIDClientCall = atoi(argv[3]);

    void *addrstart = shmat(shmID, NULL, 0);
    if (addrstart == -1)
    {
        strerror(errno);
    }
    setAddrstart(addrstart);

    myself = getPerson(myNumber);
    myself->processid = getpid();
    myself->number = myNumber;
    fp = fopen("movement.txt", "ab+");

    myNumber = myNumber;

    message msgPlaceholder;

    int found, x, y;
    found = 0;
    for (x = rand() % getMap()->SO_WIDTH; x < getMap()->SO_WIDTH && !found; x++)
    {
        for (y = rand() % getMap()->SO_HEIGHT; y < getMap()->SO_HEIGHT && !found; y++)
        {
            if (getMapCellAt(x, y)->maxElements > -1 && getMapCellAt(x, y)->clientID == -1)
            {
                getMapCellAt(x, y)->clientID = myself->number;
                myself->posX = x;
                myself->posY = y;
                found = 1;
            }
        }
        if ((x == getMap()->SO_WIDTH - 1) && (y == getMap()->SO_HEIGHT))
        {
            x = y = found = 0;
        }
    }

    struct timespec request = {0, 1000000};
    struct timespec remaining;
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
    message imHere;

    FILE *fp;
    fp = fopen("try.txt", "ab+");
    struct timespec request = {0, 50000000};
    struct timespec remaining;

    while (1)
    {

        int x,y,found;
        found=0;

        for (x = rand() % getMap()->SO_WIDTH; x < getMap()->SO_WIDTH && !found; x++)
        {
            for (y = rand() % getMap()->SO_HEIGHT; y < getMap()->SO_HEIGHT && !found; y++)
            {
                if (getMapCellAt(x, y)->maxElements > -1)
                {
                    myself->posX = x;
                    myself->posY = y;
                    found = 1;
                }
            }
            if ((x == getMap()->SO_WIDTH - 1) && (y == getMap()->SO_HEIGHT))
            {
                x = y = found = 0;
            }
        }

        imHere.clientID=myNumber;
        imHere.x=x;
        imHere.y=y;
        nanosleep(&request, &remaining);
        if (msgsnd(msgIDClientCall, &imHere, sizeof(message), 0) == -1)
        {
            fprintf(errorLog, "source:%d\tx:%d\ty:%d msgsndclientcall %s\n", myNumber, myself->posX, myself->posY, strerror(errno));
            fflush(errorLog);
        }
    }
}