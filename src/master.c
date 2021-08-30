#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ncurses.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/sem.h>

#include <sys/shm.h>
#include <errno.h>
#include <signal.h>
#include <wait.h>

#include "TaxiCab.h"

#ifndef CONFIGFULLPATH
#define CONFIGFULLPATH "./config"
#endif

#ifndef _GRID_WIDTH
#define _GRID_WIDTH 60
#endif

#ifndef _GRID_HEIGHT
#define _GRID_HEIGHT 20
#endif

#define ctrl(x) ((x)&0x1f)
#define MSG_STAT 11

/*
Variables used to parse config file
*/

FILE *config;
FILE *errorLog;
char AttributeName[20];
char EqualsPlaceHolder;
int ParsedValue;

/* 
Flags for the simulation
*/
int updateMap;
int keepOn;

/* 
Keys and ids for the ipc elements
*/
key_t ipcKey;
key_t ipcKeyKickoff;
key_t ipcKeyTimeout;
key_t ipcKeyTaxiCell;
key_t ipcKeyClientCall;
key_t ipcKeyClientTaken;
key_t ipcKeyRequestBegin;
key_t ipcKeyRequestDone;
int shmID, activeTaxi,
    msgIDKickoff,
    msgIDTimeout,
    msgIDTaxiCell,
    msgIDClientCall,
    msgIDClientTaken,
    msgIDRequestBegin,
    msgIDRequestDone;
int projID;

/* 
Simulation utils
*/
masterMap *map;
taxiNode *deadTaxis;

/* 
ncurses utils
*/
WINDOW *win;
int h, w, a;

/* 
Alarm handler
*/
void alarmMaster(int sig)
{
    keepOn = 0;
}

/*
Util to use range with rand
*/
int randFromRange(int min, int max)
{
    return rand() % (max + 1 - min) + min;
}

/*
Reads map parameters from file
*/
masterMap *readConfig(char *configPath)
{
    /*
    Parsing config file
    */

    masterMap *newMap = (masterMap *)malloc(sizeof(masterMap));
    newMap->SO_HOLES = -1;
    newMap->SO_TOP_CELLS = -1;
    newMap->SO_SOURCES = -1;
    newMap->SO_CAP_MIN = -1;
    newMap->SO_CAP_MAX = -1;
    newMap->SO_TAXI = -1;
    newMap->SO_TIMENSEC_MIN = -1;
    newMap->SO_TIMENSEC_MAX = -1;
    newMap->SO_TIMEOUT = -1;
    newMap->SO_DURATION = -1;
    newMap->SO_HEIGHT = _GRID_HEIGHT;
    newMap->SO_WIDTH = _GRID_WIDTH;

    if ((config = (FILE *)fopen(configPath, "r")) == NULL)
    {
        endwin();
        fprintf(stdout, "Config file not found.\nMaster process will now exit.\n");
        exit(1);
    }

    while (fscanf(config, "%s %c %d", AttributeName, &EqualsPlaceHolder, &ParsedValue) != EOF)
    {
        if ((EqualsPlaceHolder == '=') && (ParsedValue >= 0))
        {
            if (strcmp((char *)AttributeName, "SO_HOLES") == 0)
            {
                newMap->SO_HOLES = ParsedValue;
            }
            else if (strcmp(AttributeName, "SO_TOP_CELLS") == 0)
            {
                newMap->SO_TOP_CELLS = ParsedValue;
            }
            else if (strcmp(AttributeName, "SO_SOURCES") == 0)
            {
                newMap->SO_SOURCES = ParsedValue;
            }
            else if (strcmp(AttributeName, "SO_CAP_MIN") == 0)
            {
                newMap->SO_CAP_MIN = ParsedValue;
            }
            else if (strcmp(AttributeName, "SO_CAP_MAX") == 0)
            {
                newMap->SO_CAP_MAX = ParsedValue;
            }
            else if (strcmp(AttributeName, "SO_TAXI") == 0)
            {
                newMap->SO_TAXI = ParsedValue;
            }
            else if (strcmp(AttributeName, "SO_TIMENSEC_MIN") == 0)
            {
                newMap->SO_TIMENSEC_MIN = ParsedValue;
            }
            else if (strcmp(AttributeName, "SO_TIMENSEC_MAX") == 0)
            {
                newMap->SO_TIMENSEC_MAX = ParsedValue;
            }
            else if (strcmp(AttributeName, "SO_TIMEOUT") == 0)
            {
                newMap->SO_TIMEOUT = ParsedValue;
            }
            else if (strcmp(AttributeName, "SO_DURATION") == 0)
            {
                newMap->SO_DURATION = ParsedValue;
            }
            else
            {
                endwin();
                fprintf(stdout, "Unexpected attribute in config file on \" %s %c %d \".\nMaster process will now exit.\n", AttributeName, EqualsPlaceHolder, ParsedValue);
                exit(1);
            }
        }
        else
        {
            endwin();
            fprintf(stdout, "Failed to parse config file with \" %s %c %d \".\nMaster process will now exit.\n", AttributeName, EqualsPlaceHolder, ParsedValue);
            exit(1);
        }
    }

    if ((newMap->SO_HEIGHT == -1) || (newMap->SO_WIDTH == -1) || (newMap->SO_HOLES == -1) || (newMap->SO_TOP_CELLS == -1) || (newMap->SO_CAP_MIN == -1) || (newMap->SO_CAP_MAX == -1) || (newMap->SO_TAXI == -1) || (newMap->SO_TIMENSEC_MIN == -1) || (newMap->SO_TIMENSEC_MAX == -1) || (newMap->SO_TIMEOUT == -1) || (newMap->SO_DURATION == -1) || (newMap->SO_SOURCES == -1))
    {
        endwin();
        fprintf(stdout, "Not all parameters have been initialized, check your config file and try again.\nMaster process will now exit.\n");
        exit(EXIT_FAILURE);
    }

    if (
        ((newMap->SO_HOLES > (newMap->SO_HEIGHT * newMap->SO_WIDTH))) ||
        (((newMap->SO_TAXI) > ((newMap->SO_HEIGHT * newMap->SO_WIDTH) - newMap->SO_HOLES))) ||
        (((newMap->SO_SOURCES) > ((newMap->SO_HEIGHT * newMap->SO_WIDTH) - newMap->SO_HOLES))) ||
        (newMap->SO_TOP_CELLS > ((newMap->SO_HEIGHT * newMap->SO_WIDTH) - newMap->SO_HOLES)))
    {
        endwin();
        fprintf(stdout, "Something's wrong with simulation's number, check them.\nMaster process will now exit.\n");
        exit(EXIT_FAILURE);
    }

    /*
    The parameters are correctly loaded
    */

    return newMap;
}

/* 
Initializes map's cell
*/

void initializeMapCells()
{
    int x, y;
    masterMap *map = getMap();
    mapCell *cells;
    map->cellsSemID = semget(ipcKey, map->SO_HEIGHT * map->SO_WIDTH, IPC_CREAT | 0666); /*initialization of the semaphores array*/
    union semun arg;
    arg.val = 1;
    for (a = 0; a < (map->SO_HEIGHT * map->SO_WIDTH); a++) /*setting the array*/
    {
        semctl(map->cellsSemID, a, SETVAL, arg);
    }
    for (x = 0; x < map->SO_WIDTH; x++)
    {
        for (y = 0; y < map->SO_HEIGHT; y++) /*creation of cells with capacity and time*/
        {
            cells = getMapCellAt(x, y);
            cells->maxElements = randFromRange(map->SO_CAP_MIN, map->SO_CAP_MAX);
            cells->holdingTime = randFromRange(map->SO_TIMENSEC_MIN, map->SO_TIMENSEC_MAX);
            cells->currentElements = 0;
            cells->cantPutAnHole = 0;
            getMapCellAt(x, y)->clientID = -1;
            getMapCellAt(x, y)->topCell = 0;
        }
    }
}

/*
Creates random holes in the map
*/

void createHoles()
{
    int a, b, c, x, y;
    masterMap *map = getMap();
    mapCell *cells;

    for (c = 0; c < map->SO_HOLES; c++)
    {
        x = rand() % map->SO_WIDTH;
        y = rand() % map->SO_HEIGHT;
        int found;
        found = 0;
        while (!found)
        {
            if (getMapCellAt(x, y)->cantPutAnHole)
            {
                y++;
                if (y == map->SO_HEIGHT)
                {
                    y = 0;
                    x++;
                }
                if (x == map->SO_WIDTH)
                    x = 0;
            }
            else
            {
                found = 1;
                getMapCellAt(x, y)->maxElements = -1;

                for (a = x - 1; a < x + 2; a++)
                {
                    for (b = y - 1; b < y + 2; b++)
                    {
                        if (!(a < 0 || a >= map->SO_WIDTH || b < 0 || b >= map->SO_HEIGHT))
                        {
                            getMapCellAt(a, b)->cantPutAnHole = 1;
                        }
                    }
                }
            }
        }
    }
}

/*
Puts the map in shared memory with the config passed by argument and initializes
it and it's message queues
*/
void mapFromConfig(char *configPath)
{
    masterMap *map = readConfig(configPath);

    shmID = allocateShm(ipcKey, map);

    setAddrstart(shmat(shmID, NULL, 0));
    putMapInShm(map);

    initializeMapCells();
    createHoles();

    getMap()->msgIDKickoff = msgIDKickoff = msgget(ipcKeyKickoff, IPC_CREAT | 0666);
    getMap()->msgIDTimeout = msgIDTimeout = msgget(ipcKeyTimeout, IPC_CREAT | 0666);
    getMap()->msgIDTaxiCell = msgIDTaxiCell = msgget(ipcKeyTaxiCell, IPC_CREAT | 0666);
    getMap()->msgIDClientCall = msgIDClientCall = msgget(ipcKeyClientCall, IPC_CREAT | 0666);
    getMap()->msgIDClientTaken = msgIDClientTaken = msgget(ipcKeyClientTaken, IPC_CREAT | 0666);
    getMap()->msgIDRequestBegin = msgIDRequestBegin = msgget(ipcKeyRequestBegin, IPC_CREAT | 0666);
    getMap()->msgIDRequestDone = msgIDRequestDone = msgget(ipcKeyRequestDone, IPC_CREAT | 0666);
}

/* 
Executes all the taxi and client processes
*/
void beFruitful()
{
    masterMap *map;

    map = getMap();

    char numberString[10];
    char shmString[10];
    sprintf(shmString, "%d", shmID);

    for (a = 0; a < map->SO_TAXI; a++)
    {
        if (fork() == 0)
        {

            sprintf(numberString, "%d", a);

            char *paramList[] = {"./bin/taxi",
                                 numberString,
                                 shmString,
                                 NULL};

            char *environ[] = {NULL};
            while (execve("./bin/taxi", paramList, environ) != -1)
                ;
            fprintf(errorLog, "forking taxi n%d %s", a, strerror(errno));
            fflush(errorLog);
            exit(EXIT_FAILURE);
        }
    }

    for (a = 0; a < map->SO_SOURCES; a++)
    {
        if (fork() == 0)
        {
            sprintf(numberString, "%d", a);

            char *const paramList[] = {"./bin/source", numberString, shmString, NULL};
            char *environ[] = {NULL};
            while (execve("./bin/source", paramList, environ) != -1)
                ;
            fprintf(errorLog, "forking client n%d %s", a, strerror(errno));
            fflush(errorLog);

            exit(EXIT_FAILURE);
        }
    }
    bornAMaster();
}

/*
Master function to monitor the simulation
*/
void bornAMaster()
{
    /*
    Initializes flag and dead taxis list
    */
    masterMap *map = getMap();
    deadTaxis = creator();
    keepOn = 1;
    getMap()->masterProcessID = getpid();
    /*
    Prints titlebar and map's border
    */
    for (a = 0; a < w; a++)
        addch(ACS_BULLET);
    move(0, (w / 2) - 5);
    addch(ACS_CKBOARD);
    printw(" Taxicab ");
    addch(ACS_CKBOARD);

    move(4, 2);
    addch(ACS_ULCORNER);
    move(4, 2 + getMap()->SO_WIDTH + 1);
    addch(ACS_URCORNER);

    move(4 + getMap()->SO_HEIGHT, 2);
    addch(ACS_LLCORNER);

    move(5 + getMap()->SO_HEIGHT, 2 + getMap()->SO_WIDTH);
    addch(ACS_LRCORNER);

    move(4, 3);
    for (a = 0; a < getMap()->SO_WIDTH; a++)
        addch(ACS_S7);

    move(5 + getMap()->SO_HEIGHT, 2);
    for (a = 0; a < getMap()->SO_WIDTH + 1; a++)
        addch(ACS_S7);

    for (a = 0; a < getMap()->SO_HEIGHT; a++)
    {
        move(a + 5, 2);
        addch(ACS_VLINE);
        move(a + 5, 2 + getMap()->SO_WIDTH + 1);
        addch(ACS_VLINE);
    }

    move(5 + getMap()->SO_HEIGHT, 2);
    addch(ACS_LLCORNER);

    move(5 + getMap()->SO_HEIGHT, 2 + getMap()->SO_WIDTH + 1);
    addch(ACS_LRCORNER);

    refresh();

    mvprintw(2, 2, "Waiting for taxis to fill the map... 0/%d   ", map->SO_TAXI);

    refresh();
    int a, b;
    message placeHolder;
    /*
    Waits for taxis to fill the map
    */
    a = 0;
    while (a < getMap()->SO_TAXI)
    {
        if ((msgrcv(msgIDTaxiCell, &placeHolder, sizeof(message), 0, 0)) == -1)
        {

            mvprintw(3, 2, "Can't receive message to kickoff taxi, %s", strerror(errno));
            refresh();
        }
        else
        {

            mvprintw(2, 2, "Waiting for taxis to fill the map... %d/%d  ", ++a, map->SO_TAXI);
            refresh();
        }
    }
    /*
    Prints map's cell's state
    */
    for (a = 0; a < getMap()->SO_WIDTH; a++)
    {
        for (b = 0; b < getMap()->SO_HEIGHT; b++)
        {
            move(b + 5, a + 3);
            if (getMapCellAt(a, b)->maxElements == -1)
            {
                addch(ACS_CKBOARD);
            }
            else
            {
                if (getMapCellAt(a, b)->currentElements == 0)
                {
                    addch(ACS_BULLET);
                }
                else
                {
                    printw("%d", getMapCellAt(a, b)->currentElements);
                }
            }
        }
    }
    updateMap = 0;

    /*
    Sets the signal handler for stopping simulation if alarm or SIGINT come
    */
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = alarmMaster;
    sigaction(SIGALRM, &act, 0);
    sigaction(SIGINT, &act, 0);

    activeTaxi = 0;

    move(h - 1, 0);
    clrtoeol();
    mvprintw(h - 1, 0, "Status: Kicking off the taxis...");
    refresh();

    move(2, 0);
    clrtoeol();

    /*
    Kicks the taxi through SIGUSR1
    */
    for (a = 0; a < map->SO_TAXI; a++)
    {
        kill(getTaxi(a)->processid, SIGUSR1);

        if (msgrcv(msgIDKickoff, &placeHolder, sizeof(message), 0, IPC_NOWAIT) != -1)
        {
            activeTaxi++;
        }
        mvprintw(2, 2, "Kicking the taxi... %d/%d", a + 1, map->SO_TAXI);
        refresh();
    }

    move(h - 1, 0);
    clrtoeol();
    mvprintw(h - 1, 0, "Status: Receiving alive message from taxis...");
    refresh();

    move(2, 0);
    clrtoeol();
    /*
    Waiting for taxis to send alive messages 
    */
    while (activeTaxi < map->SO_TAXI)
    {
        if (msgrcv(msgIDKickoff, &placeHolder, sizeof(message), 0, IPC_NOWAIT) != -1)
        {
            activeTaxi++;
            mvprintw(2, 2, "Taxi alive: %d/%d   ", activeTaxi, map->SO_TAXI);
            refresh();
        }
    }

    move(h - 1, 0);
    clrtoeol();
    mvprintw(h - 1, 0, "Status: Kicking off the clients...");
    refresh();

    message kickoffMessage;

    move(2, 0);
    clrtoeol();
    mvprintw(2, 2, "Kicked clients: %d/%d   ", a, map->SO_SOURCES);
    refresh();

    /*
    Kicking users through SIGUSR1
    */
    for (a = 0; a < map->SO_SOURCES; a++)
    {
        while (getPerson(a)->processid == 0)
            ;
        kill(getPerson(a)->processid, SIGUSR1);
        mvprintw(2, 2, "Kicked clients: %d/%d   ", a + 1, map->SO_SOURCES);
        refresh();
    }

    int requestTaken, requestBegin, requestDone, requestAborted, resurrectedTaxi, spammedRequests;
    requestBegin = requestDone = requestTaken = requestAborted = resurrectedTaxi = 0;

    move(h - 1, 0);
    clrtoeol();
    mvprintw(h - 1, 0, "Status: Simulation's going. CTRL+B to spam SO_SOURCES requests, spammed requests: %d.", spammedRequests);
    refresh();

    move(2, 0);
    clrtoeol();
    mvprintw(2, 2, "Active taxis: %d/%d   ", activeTaxi, map->SO_TAXI);
    refresh();

    /*
    Preparing those for execv
    */
    char numberString[10];
    char shmString[10];
    sprintf(shmString, "%d", shmID);

    /*
    The simulation starts and so does the alarm
    */
    alarm(getMap()->SO_DURATION);
    while (keepOn)
    {
        /*
        Checking if CTRL+B is pressed
        */
        if (getch() == ctrl('b'))
        {
            for (a = 0; a < getMap()->SO_SOURCES; a++)
            {
                /*
                Spamming requests
                */
                if (kill(getPerson(a)->processid, SIGUSR2) != -1)
                {
                    spammedRequests++;
                };
            }

            move(h - 1, 0);
            clrtoeol();
            mvprintw(h - 1, 0, "Status: Simulation's going. CTRL+B to spam SO_SOURCES requests, spammed requests: %d.", spammedRequests);
            refresh();
        }
        /*
        Checking if new taxis took position
        */
        if ((msgrcv(msgIDTaxiCell, &placeHolder, sizeof(message), 0, IPC_NOWAIT)) != -1)
        {
            kill(getTaxi(placeHolder.driverID)->processid, SIGUSR1);
            resurrectedTaxi++;
        }
        /*
        Checking if some taxi has gone timeout
        */
        if (msgrcv(msgIDTimeout, &placeHolder, sizeof(message), 0, IPC_NOWAIT) != -1)
        {

            /*
            Putting the dead taxi inside the list
            */
            taxi *dead = malloc(sizeof(taxi));
            dead->distanceDone = getTaxi(placeHolder.driverID)->distanceDone;
            dead->number = getTaxi(placeHolder.driverID)->number;
            dead->posX = getTaxi(placeHolder.driverID)->posX;
            dead->posY = getTaxi(placeHolder.driverID)->posY;
            dead->processid = getTaxi(placeHolder.driverID)->processid;
            dead->requestsTaken = getTaxi(placeHolder.driverID)->requestsTaken;
            addTaxi(deadTaxis, dead);

            /*
            Sending SIGTERM to taxi to gently terminate them in case
            they aren't already terminated
            */
            kill(getTaxi(placeHolder.driverID)->processid, SIGTERM);
            activeTaxi--;
            move(2, 0);
            clrtoeol();
            /*
            if x==-1 taxi is dead during request
            */
            if (placeHolder.x == -1)
            {

                requestAborted++;
            }
            mvprintw(2, 2, "Active taxis: %d/%d\t Resurrected taxis:%d", activeTaxi, map->SO_TAXI, resurrectedTaxi);

            /*
            Executing new taxi with the dead number
            */
            int pid = fork();
            if (pid == 0)
            {

                sprintf(numberString, "%d", placeHolder.driverID);

                char *paramList[] = {"./bin/taxi",
                                     numberString,
                                     shmString,
                                     NULL};
                char *environ[] = {NULL};
                if (execve("./bin/taxi", paramList, environ) == -1)
                {
                    printf("%s", strerror(errno));
                };
                refresh();
            }
        }

        /*
        Checking if new taxis are active
        */
        if (msgrcv(msgIDKickoff, &placeHolder, sizeof(message), 0, IPC_NOWAIT) != -1)
        {
            activeTaxi++;
            move(2, 0);
            clrtoeol();
            mvprintw(2, 2, "Active taxis: %d/%d\t Resurrected taxis:%d", activeTaxi, map->SO_TAXI, resurrectedTaxi);
            refresh();
            /*printf("Taxi n%d posizionato in x:%d, y:%d, cella a %d/%d\n", placeHolder.driverID, placeHolder.sourceX, placeHolder.sourceY, getMapCellAt(placeHolder.sourceX, placeHolder.sourceY)->currentElements, getMapCellAt(placeHolder.sourceX, placeHolder.sourceY)->maxElements);*/
        }
        /*
        Checking if some taxi took some client's request
        */
        if (msgrcv(msgIDClientTaken, &placeHolder, sizeof(message), 0, IPC_NOWAIT) != -1)
        {
            requestTaken++;
            updateMap = 1;
            move(3, 0);
            clrtoeol();
            mvprintw(3, 2, "Requests\tTaken:%d\tStarted:%d\tEnded:%d\tAborted:%d", requestTaken, requestBegin, requestDone, requestAborted);
        }

        /*
        Checking if some taxi has come to the source position
        */
        if (msgrcv(msgIDRequestBegin, &placeHolder, sizeof(message), 0, IPC_NOWAIT) != -1)
        {
            requestBegin++;
            updateMap = 1;
            move(3, 0);
            clrtoeol();
            mvprintw(3, 2, "Requests\tTaken:%d\tStarted:%d\tEnded:%d\tAborted:%d", requestTaken, requestBegin, requestDone, requestAborted);
        }

        /*
        Checking if some taxi has come to the destination
        */
        if (msgrcv(msgIDRequestDone, &placeHolder, sizeof(message), 0, IPC_NOWAIT) != -1)
        {
            requestDone++;
            updateMap = 1;
            move(3, 0);
            clrtoeol();
            mvprintw(3, 2, "Requests\tTaken:%d\tStarted:%d\tEnded:%d\tAborted:%d", requestTaken, requestBegin, requestDone, requestAborted);
        }

        /*
        Checking if map is to update 
        */
        if (updateMap)
        {
            for (a = 0; a < getMap()->SO_WIDTH; a++)
            {
                for (b = 0; b < getMap()->SO_HEIGHT; b++)
                {
                    move(b + 5, a + 3);

                    if (getMapCellAt(a, b)->maxElements == -1)
                    {
                        addch(ACS_CKBOARD);
                    }
                    else
                    {
                        if (getMapCellAt(a, b)->currentElements == 0)
                        {
                            addch(ACS_BULLET);
                        }
                        else
                        {
                            printw("%d", getMapCellAt(a, b)->currentElements);
                        }
                    }
                }
            }
            updateMap = 0;
            refresh();
        }
    };

    /*
    Adding remaining messages on kickoff queue to the active taxis count
    */
    struct msqid_ds msgStat;
    msgctl(msgIDKickoff, MSG_STAT, &msgStat);
    activeTaxi = activeTaxi + msgStat.msg_qnum;

    /*
    Removing remaining messages on kickoff queue to the active taxis count
    */
    msgctl(msgIDTimeout, MSG_STAT, &msgStat);
    activeTaxi = activeTaxi - msgStat.msg_qnum;
    move(h - 1, 0);
    clrtoeol();
    mvprintw(h - 1, 0, "Status: Simulation's done, sending SIGTERM to taxis and sources...");
    refresh();

    /*
    Gently terminating taxis and clients through SIGTERM
    */
    for (a = 0; a < getMap()->SO_SOURCES; a++)
    {
        kill(getPerson(a)->processid, SIGTERM);
        waitpid(getPerson(a)->processid, NULL, WNOHANG);
    }
    for (a = 0; a < getMap()->SO_TAXI; a++)
    {
        kill(getTaxi(a)->processid, SIGTERM);
        waitpid(getTaxi(a)->processid, NULL, WNOHANG);
    }

    /*
    Checking if some message on timeout queue has to be added to aborted count
    */
    while (msgrcv(msgIDTimeout, &placeHolder, sizeof(message), 0, IPC_NOWAIT) != -1)
    {

        if (placeHolder.x == -1)
        {

            requestAborted++;
        }
    }

    /*
    Checking if some client call has been untaken
    */
    msgctl(msgIDClientCall, MSG_STAT, &msgStat);
    mvprintw(6 + getMap()->SO_HEIGHT, 2, "Requests not taken: %d", msgStat.msg_qnum);
    mvprintw(3, 2, "Requests\tTaken:%d\tStarted:%d\tEnded:%d\tAborted:%d", requestTaken, requestBegin, requestDone, requestAborted);

    refresh();
    move(h - 1, 0);
    clrtoeol();
    mvprintw(h - 1, 0, "Status: Simulation's done, removing ipc resources...");
    refresh();

    /*
    Removing semaphores and message queues
    */
    semctl(getMap()->cellsSemID, getMap()->SO_HEIGHT * getMap()->SO_WIDTH, IPC_RMID);
    msgctl(msgIDKickoff, IPC_RMID, NULL);
    msgctl(msgIDTimeout, IPC_RMID, NULL);
    msgctl(msgIDTaxiCell, IPC_RMID, NULL);
    msgctl(msgIDClientCall, IPC_RMID, NULL);
    msgctl(msgIDClientTaken, IPC_RMID, NULL);
    msgctl(msgIDRequestBegin, IPC_RMID, NULL);
    msgctl(msgIDRequestDone, IPC_RMID, NULL);

    /*
    Printing clients position
    */
    for (a = 0; a < getMap()->SO_WIDTH; a++)
    {
        for (b = 0; b < getMap()->SO_HEIGHT; b++)
        {
            move(b + 5, a + 3);
            if (getMapCellAt(a, b)->maxElements == -1)
            {
                addch(ACS_CKBOARD);
            }
            else
            {
                if (getMapCellAt(a, b)->clientID != -1)
                {
                    printw("S");
                }
                else
                {
                    addch(' ');
                }
            }
        }
    }

    move(2, 0);
    clrtoeol();
    nodelay(stdscr, FALSE);

    mvprintw(2, 2, "Active taxis: %d/%d\t Resurrected taxis:%d", activeTaxi, map->SO_TAXI, resurrectedTaxi);

    mvprintw(h - 1, 0, "Status: Simulation's done, making up the statistics...");

    move(h - 1, 0);
    clrtoeol();
    refresh();

    /*
    Getting the top cells
    */
    typedef struct _tempCell
    {
        int x, y;
    } tempCell;
    tempCell **top = malloc(sizeof(tempCell) * getMap()->SO_TOP_CELLS);
    int x, y, foundX, foundY;
    mapCell *temp;
    temp = getMapCellAt(0, 0);
    for (a = 0; a < getMap()->SO_TOP_CELLS; a++)
    {
        for (x = 0; x < map->SO_WIDTH; x++)
        {
            for (y = 0; y < map->SO_HEIGHT; y++) /*creation of cells with capacity and time*/
            {
                if (temp->passedBy < getMapCellAt(x, y)->passedBy)
                {
                    temp = getMapCellAt(x, y);
                    foundX = x;
                    foundY = y;
                }
            }
        }
        top[a] = malloc(sizeof(tempCell));
        top[a]->x = foundX;
        top[a]->y = foundY;
        temp->passedBy = 0;
    }

    /*
    Scanning the taxis to get the statistics
    */
    taxi *cellsMax, *timeMax, *reqMax;
    Node *tempTaxi;
    tempTaxi = deadTaxis->first;
    cellsMax = timeMax = reqMax = getTaxi(0);
    /*
    Seeking taxi who has done the longest distance
    */
    for (a = 0; a < getMap()->SO_TAXI; a++)
    {
        if (cellsMax->distanceDone < getTaxi(a)->distanceDone)
        {
            cellsMax = getTaxi(a);
        }
    }
    while (tempTaxi != deadTaxis->last)
    {
        if (cellsMax->distanceDone < tempTaxi->element->distanceDone)
        {
            cellsMax = tempTaxi->element;
        }
        tempTaxi = tempTaxi->next;
    }

    /*
    Seeking taxi who has done the longest time
    */
    tempTaxi = deadTaxis->first;
    for (a = 0; a < getMap()->SO_TAXI; a++)
    {
        if (timeMax->maxTime < getTaxi(a)->maxTime)
        {
            timeMax = getTaxi(a);
        }
    }
    while (tempTaxi != deadTaxis->last)
    {
        if (timeMax->maxTime < tempTaxi->element->maxTime)
        {
            timeMax = tempTaxi->element;
        }
        tempTaxi = tempTaxi->next;
    }

    /*
    Seeking taxi who took the highest request
    */
    tempTaxi = deadTaxis->first;
    for (a = 0; a < getMap()->SO_TAXI; a++)
    {
        if (reqMax->requestsTaken < getTaxi(a)->requestsTaken)
        {
            reqMax = getTaxi(a);
        }
    }
    while (tempTaxi != deadTaxis->last)
    {
        if (reqMax->requestsTaken < tempTaxi->element->requestsTaken)
        {
            reqMax = tempTaxi->element;
        }
        tempTaxi = tempTaxi->next;
    }
    nodelay(stdscr, FALSE);
    mvprintw(h - 1, 0, "Status: Source's location is shown. Press any key to show top cells.");
    refresh();
    getch();

    /*
    Showing the top cells' position
    */
    for (a = 0; a < getMap()->SO_TOP_CELLS; a++)
    {

        move(top[a]->y + 5, top[a]->x + 3);
        addch('T');
    }
    move(h - 1, 0);
    clrtoeol();
    mvprintw(h - 1, 0, "Status: Top cells are shown. Press any key to show best taxis.");
    refresh();
    getch();

    /*
    Showing the taxis' statistics
    */
    for (b = 0; b < getMap()->SO_HEIGHT + 2; b++)
    {
        move(b + 4, 0);
        clrtoeol();
    }
    move(h - 1, 0);
    clrtoeol();
    mvprintw(h - 1, 0, "Status: Taxis score are shown. Press any key to close Taxicab.");

    mvprintw(5, 2, "The taxi who has run the longest distance:");
    mvprintw(6, 2, "Taxi PID:%d, gone throught %d cells.", cellsMax->processid, cellsMax->distanceDone);
    mvprintw(8, 2, "The taxi who has done the longest travel:");
    mvprintw(9, 2, "Taxi PID:%d, has done a travel in %ld nanosec.", timeMax->processid, timeMax->maxTime);
    mvprintw(11, 2, "The taxi who took most requests:");
    mvprintw(12, 2, "Taxi PID:%d, took %d requests.", reqMax->processid, reqMax->requestsTaken);

    refresh();
    getch();

    move(h - 1, 0);
    clrtoeol();
    mvprintw(h - 1, 0, "Status: Quitting. Deallocating shared memory and SIGKILLING processes to be sure...");
    refresh();

    /*
    Killing any processes left
    */
    for (a = 0; a < getMap()->SO_SOURCES; a++)
    {
        kill(getPerson(a)->processid, SIGKILL);
        waitpid(getPerson(a)->processid, NULL, WNOHANG);
    }
    for (a = 0; a < getMap()->SO_TAXI; a++)
    {
        kill(getTaxi(a)->processid, SIGKILL);
        waitpid(getTaxi(a)->processid, NULL, WNOHANG);
    }

    /*
    Deallocating shared memory
    */
    shmctl(shmID, IPC_RMID, NULL);
    endwin();
}

int main(int argc, char *argv[])
{
    /*
    Rand seed
    */
    srand(time(NULL));

    /*
    Initializing ncurses
    */
    initscr();
    curs_set(0);
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    getmaxyx(stdscr, h, w);
    win = newwin(h, w, 0, 0);
    wmove(win, 0, 0);

    char *configPath;

    if (argc == 2)
    {
        configPath = argv[1];
    }
    else if (argc == 1)
    {
        configPath = CONFIGFULLPATH;
    }
    else
    {
        printw("Wrong number of parameters!\nusage: ./master pathToConfigFile\n");
        exit(EXIT_FAILURE);
    }

    /*
    Getting ipc keys
    */
    projID = rand();
    ipcKey = ftok(configPath, projID);
    ipcKeyKickoff = ftok(configPath, projID + 1);
    ipcKeyTimeout = ftok(configPath, projID + 2);
    ipcKeyTaxiCell = ftok(configPath, projID + 3);
    ipcKeyClientCall = ftok(configPath, projID + 4);
    ipcKeyClientTaken = ftok(configPath, projID + 5);
    ipcKeyRequestBegin = ftok(configPath, projID + 6);
    ipcKeyRequestDone = ftok(configPath, projID + 7);

    /*
    Initializing map
    */
    mapFromConfig(configPath);

    /*
    Executing processes and starting simulation
    */
    beFruitful();

    return EXIT_SUCCESS;
}