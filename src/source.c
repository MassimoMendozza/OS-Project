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

int myNumber, shmID, msgIDClientCall, addRequest, counter;
person *myself;

FILE *errorLog;

int goOn;

/*
The signal handler
*/
void signalHandler(int sigNum)
{
    switch (sigNum)
    {
    case SIGTERM:
    case SIGUSR1:
        goOn = 0;
        break;
    case SIGUSR2:
        addRequest++;
        break;
    }
}

int main(int argc, char *argv[])
{

    /*
    Setting up the signal handler
    */
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = signalHandler;
    sigaction(SIGUSR1, &act, 0);
    sigaction(SIGUSR2, &act, 0);
    sigaction(SIGTERM, &act, 0);

    addRequest = 1;
    srand(getpid() % time(NULL));
    errorLog = fopen("errorLog.txt", "ab+");
    myNumber = atoi(argv[1]);
    shmID = atoi(argv[2]);

    /*
    Getting and setting base address for shmUtils
    */
    void *addrstart = shmat(shmID, NULL, 0);
    if (addrstart == -1)
    {
        strerror(errno);
    }
    setAddrstart(addrstart);

    /*
    Putting its information in shm and getting msg queue id
    */
    msgIDClientCall = getMap()->msgIDClientCall;
    myself = getPerson(myNumber);
    while (getpid() == 0)
        ;
    myself->processid = getpid();
    myself->number = myNumber;

    message msgPlaceholder;

    /*
    Searching a place to put myself in
    */
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

    /*
    Waiting for SIGUSR1 to kickoff simulation
    */
    struct timespec request = {0, 1000000};
    struct timespec remaining;
    goOn = 1;
    while (goOn)
    {
        nanosleep(&request, &remaining);
    }

    /*
        Simulation goes on
    */
    clientKickoff();
}

void clientKickoff()
{
    message imHere;
    goOn = 1;

    myself->processid = getpid();

    struct timespec request = {0, 50000000};
    struct timespec remaining;

    while (goOn)
    {
        nanosleep(&request, &remaining);
        for (counter = 0; counter < addRequest; counter++)
        {

            int x, y, found;
            found = 0;

            /*
            Searching destination
            */
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

            imHere.clientID = myNumber;
            imHere.x = x;
            imHere.y = y;
            if (msgsnd(msgIDClientCall, &imHere, sizeof(message), 0) == -1)
            {
                fprintf(errorLog, "source:%d\tx:%d\ty:%d msgsndclientcall %s\n", myNumber, myself->posX, myself->posY, strerror(errno));
                fflush(errorLog);
            }
        }
        /*
        Resetting manual request (from SIGUSR2) counter
        */
        addRequest = 1;
    }
    exit(EXIT_SUCCESS);
}