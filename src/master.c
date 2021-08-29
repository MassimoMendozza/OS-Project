#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ncurses.h>
#include <unistd.h>
#include <sys/msg.h>

#include <semaphore.h>

#include <sys/shm.h>
#include <errno.h>
#include <signal.h>
#include <wait.h>

#include "TaxiCab.h"

#ifndef CONFIGFULLPATH
#define CONFIGFULLPATH "./config"
#endif

#define ctrl(x)           ((x) & 0x1f)

/*
Variables used to parse config file
*/

FILE *config;
FILE *errorLog;
char AttributeName[20];
char EqualsPlaceHolder;
int ParsedValue;
int updateMap;
int keepOn;

key_t ipcKey;
key_t ipcKeyKickoff;
key_t ipcKeyTimeout;
key_t ipcKeyTaxiCell;
key_t ipcKeyClientCall;
key_t ipcKeyClientTaken;
key_t ipcKeyRequestBegin;
key_t ipcKeyRequestDone;

int projID;

masterMap *map;
taxiNode *deadTaxis;
int shmID, activeTaxi, updatedmap,
    msgIDKickoff,
    msgIDTimeout,
    msgIDTaxiCell,
    msgIDClientCall,
    msgIDClientTaken,
    msgIDRequestBegin,
    msgIDRequestDone;
updatedmap = 0;
void *addrstart; /*the addres of the shared memory portion (first element is map)*/

WINDOW *win;
int h, w, a;

void alarmMaster(int sig)
{
    keepOn = 0;
}

/*
Simulation parameter
*/

int randFromRange(int min, int max)
{
    return rand() % (max + 1 - min) + min;
}

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
    newMap->SO_HEIGHT = -1;
    newMap->SO_WIDTH = -1;

    /*printf("\033[H\033[J");
    printf("Reading config file from %s...\n\n", configPath);*/
    if ((config = (FILE *)fopen(configPath, "r")) == NULL)
    {
        printf("Config file not found.\nMaster process will now exit.\n");
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
            else if (strcmp(AttributeName, "SO_HEIGHT") == 0)
            {
                newMap->SO_HEIGHT = ParsedValue;
            }
            else if (strcmp(AttributeName, "SO_WIDTH") == 0)
            {
                newMap->SO_WIDTH = ParsedValue;
            }
            else
            {
                printf("Unexpected attribute in config file on \" %s %c %d \".\nMaster process will now exit.\n", AttributeName, EqualsPlaceHolder, ParsedValue);
                exit(1);
            }
        }
        else
        {
            printf("Failed to parse config file with \" %s %c %d \".\nMaster process will now exit.\n", AttributeName, EqualsPlaceHolder, ParsedValue);
            exit(1);
        }
    }

    if ((newMap->SO_HEIGHT == -1) || (newMap->SO_WIDTH == -1) || (newMap->SO_HOLES == -1) || (newMap->SO_TOP_CELLS == -1) || (newMap->SO_CAP_MIN == -1) || (newMap->SO_CAP_MAX == -1) || (newMap->SO_TAXI == -1) || (newMap->SO_TIMENSEC_MIN == -1) || (newMap->SO_TIMENSEC_MAX == -1) || (newMap->SO_TIMEOUT == -1) || (newMap->SO_DURATION == -1) || (newMap->SO_SOURCES == -1))
    {
        printf("Not all parameters have been initialized, check your config file and try again.\nMaster process will now exit.\n");
        exit(EXIT_FAILURE);
    }

    /*
    The parameters are correctly loaded
    */

    return newMap;
}

void initializeMapCells()
{
    int x, y;
    masterMap *map = addrstart;
    mapCell *cells;
    map->cellsSemID = semget(ipcKey, map->SO_HEIGHT * map->SO_WIDTH, IPC_CREAT | 0666); /*initialization of the semaphores array*/
    initSemAvailable(map->cellsSemID, map->SO_HEIGHT * map->SO_WIDTH);
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
    createHoles
    Takes random x and y and checks if the cell [x][y] is eligible for a hole.
    If not we scan the matrix linearly in search of the first eligible cell.
    Then we get back and make the whole 3x3 area around the hole ineligible to get a hole.

    The mechanism for linear scan is:
        first we put as limit the end of the matrix
        then we scan:
            if we find a cell then we put the current indexes as the limits
            to get out of the three cycles and then we make x and y the top left corner
            of the new [a][b] cell's area
            if we do not find the cell from [x][y] to the end of the matrix we'll end up likely
            with a == SO_WIDTH, a sort of checkpoint that indicates that we reached the end of 
            the two cycles, at this point we need to check the part of the matrix before [x][y].
            We put a and b to 0 and put the limit as the original x and y, now we must find the 
            eligible cell, if not the configuration cannot go ahead and we should retry from the beginning;
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

masterMap *mapFromConfig(char *configPath)
{
    masterMap *map = readConfig(configPath);

    shmID = allocateShm(ipcKey, map);

    msgIDKickoff = msgget(ipcKeyKickoff, IPC_CREAT | 0666);
    msgIDTimeout = msgget(ipcKeyTimeout, IPC_CREAT | 0666);
    msgIDTaxiCell = msgget(ipcKeyTaxiCell, IPC_CREAT | 0666);
    msgIDClientCall = msgget(ipcKeyClientCall, IPC_CREAT | 0666);
    msgIDClientTaken = msgget(ipcKeyClientTaken, IPC_CREAT | 0666);
    msgIDRequestBegin = msgget(ipcKeyRequestBegin, IPC_CREAT | 0666);
    msgIDRequestDone = msgget(ipcKeyRequestDone, IPC_CREAT | 0666);

    addrstart = shmat(shmID, NULL, 0); /*return the addres of the shared memory portion*/

    setAddrstart(addrstart); /*setting the parameter for the address of sh. memo.*/
    putMapInShm(map);        /*copy in the sh. memo. the parameters of the map*/

    initializeMapCells();
    createHoles();

    return map;
}

void beFruitful() /*creation of processes like taxi and client*/
{
    int a, shouldIBeATaxi, shouldIBeAClient;
    masterMap *map;

    map = getMap();

    char numberString[10];
    char shmString[10];
    char msgKickString[10];
    char msgTimeString[10];
    char msgTaCeString[10];
    char msgClCaString[10];
    char msgClTaString[10];
    char msgReBeString[10];
    char msgReDoString[10];
    sprintf(shmString, "%d", shmID);
    sprintf(msgKickString, "%d", msgIDKickoff);
    sprintf(msgTimeString, "%d", msgIDTimeout);
    sprintf(msgTaCeString, "%d", msgIDTaxiCell);
    sprintf(msgClCaString, "%d", msgIDClientCall);
    sprintf(msgClTaString, "%d", msgIDClientTaken);
    sprintf(msgReBeString, "%d", msgIDRequestBegin);
    sprintf(msgReDoString, "%d", msgIDRequestDone);

    shouldIBeATaxi = shouldIBeAClient = 0;
    for (a = 0; a < map->SO_TAXI; a++)
    {
        if (fork() == 0)
        {

            sprintf(numberString, "%d", a);

            char *paramList[] = {"./bin/taxi",
                                 numberString,
                                 shmString,
                                 msgKickString,
                                 msgTimeString,
                                 msgTaCeString,
                                 msgClCaString,
                                 msgClTaString,
                                 msgReBeString,
                                 msgReDoString,
                                 NULL};
            char *environ[] = {NULL};
            if (execve("./bin/taxi", paramList, environ) == -1)
            {
                printf("%s", strerror(errno));
            };
            break;
        }
    }

    for (a = 0; a < map->SO_SOURCES; a++)
    {
        if (fork() == 0)
        {
            sprintf(numberString, "%d", a);

            char *const paramList[] = {"./bin/source", numberString, shmString, msgClCaString, NULL};
            char *environ[] = {NULL};
            execve("./bin/source", paramList, environ);
            break;
        }
    }
    bornAMaster();
}

void bornAMaster()
{
    deadTaxis = creator();
    keepOn = 1;
    wmove(win, 0, 0);
    getMap()->masterProcessID = getpid();

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
    signal(SIGALRM, &alarmMaster);
    signal(SIGINT, &alarmMaster);

    activeTaxi = 0;

    move(h - 1, 0);
    clrtoeol();
    mvprintw(h - 1, 0, "Status: Kicking off the taxis...");
    refresh();

    move(2, 0);
    clrtoeol();
    for (a = 0; a < map->SO_TAXI; a++)
    {
        kill(getTaxi(a)->processid, SIGUSR1);

        if (msgrcv(msgIDKickoff, &placeHolder, sizeof(message), 0, IPC_NOWAIT) != -1)
        {
            activeTaxi++;
            /*printf("Taxi n%d posizionato in x:%d, y:%d, cella a %d/%d\n", placeHolder.driverID, placeHolder.sourceX, placeHolder.sourceY, getMapCellAt(placeHolder.sourceX, placeHolder.sourceY)->currentElements, getMapCellAt(placeHolder.sourceX, placeHolder.sourceY)->maxElements);*/
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

    while (activeTaxi < map->SO_TAXI)
    {
        if (msgrcv(msgIDKickoff, &placeHolder, sizeof(message), 0, IPC_NOWAIT) != -1)
        {
            activeTaxi++;
            mvprintw(2, 2, "Taxi alive: %d/%d   ", activeTaxi, map->SO_TAXI);
            refresh();
            /*printf("Taxi n%d posizionato in x:%d, y:%d, cella a %d/%d\n", placeHolder.driverID, placeHolder.sourceX, placeHolder.sourceY, getMapCellAt(placeHolder.sourceX, placeHolder.sourceY)->currentElements, getMapCellAt(placeHolder.sourceX, placeHolder.sourceY)->maxElements);*/
        }
    }

    move(h - 1, 0);
    clrtoeol();
    mvprintw(h - 1, 0, "Status: Kicking off the clients...");
    refresh();

    message kickoffMessage;

    move(2, 0);
    clrtoeol();
    mvprintw(2, 2, "Kicked clients: %d/%d   ", a + 1, map->SO_SOURCES);
    refresh();
    for (a = 0; a < map->SO_SOURCES; a++)
    {

        kill(getPerson(a)->processid, SIGUSR1);
        mvprintw(2, 2, "Kicked clients: %d/%d   ", a + 1, map->SO_SOURCES);
        refresh();
    }
    alarm(getMap()->SO_DURATION);

    int requestTaken, requestBegin, requestDone, requestAborted, resurrectedTaxi, spammedRequests;
    requestBegin = requestDone = requestTaken = requestAborted = resurrectedTaxi = 0;

    move(h - 1, 0);
    clrtoeol();
    mvprintw(h - 1, 0, "Status: Simulation's going. CTRL+B to spam SO_SOURCES requests, successfuly spammed requests: %d.", spammedRequests);
    refresh();

    move(2, 0);
    clrtoeol();
    mvprintw(2, 2, "Active taxis: %d/%d   ", activeTaxi, map->SO_TAXI);
    refresh();

    char numberString[10];
    char shmString[10];
    char msgKickString[10];
    char msgTimeString[10];
    char msgTaCeString[10];
    char msgClCaString[10];
    char msgClTaString[10];
    char msgReBeString[10];
    char msgReDoString[10];
    sprintf(shmString, "%d", shmID);
    sprintf(msgKickString, "%d", msgIDKickoff);
    sprintf(msgTimeString, "%d", msgIDTimeout);
    sprintf(msgTaCeString, "%d", msgIDTaxiCell);
    sprintf(msgClCaString, "%d", msgIDClientCall);
    sprintf(msgClTaString, "%d", msgIDClientTaken);
    sprintf(msgReBeString, "%d", msgIDRequestBegin);
    sprintf(msgReDoString, "%d", msgIDRequestDone);

    while (keepOn)
    {
        message placeHolder;
        if (getch() == ctrl('b'))
        {
            for (a = 0; a < getMap()->SO_SOURCES; a++)
            {
                if (kill(getPerson(a)->processid, SIGUSR2) != -1)
                {
                    spammedRequests++;
                };
            }

            move(h - 1, 0);
            clrtoeol();
            mvprintw(h - 1, 0, "Status: Simulation's going. CTRL+B to spam SO_SOURCES requests, successfuly spammed requests: %d.", spammedRequests);
            refresh();
        }
        if ((msgrcv(msgIDTaxiCell, &placeHolder, sizeof(message), 0, IPC_NOWAIT)) != -1)
        {
            kill(getTaxi(placeHolder.driverID)->processid, SIGUSR1);
            resurrectedTaxi++;
        } /* checking if someone's killed itself*/
        if (msgrcv(msgIDTimeout, &placeHolder, sizeof(message), 0, IPC_NOWAIT) != -1)
        {

            kill(getTaxi(placeHolder.driverID)->processid, SIGKILL);
            taxi *dead = malloc(sizeof(taxi));
            dead->distanceDone = getTaxi(placeHolder.driverID)->distanceDone;
            dead->number = getTaxi(placeHolder.driverID)->number;
            dead->posX = getTaxi(placeHolder.driverID)->posX;
            dead->posY = getTaxi(placeHolder.driverID)->posY;
            dead->processid = getTaxi(placeHolder.driverID)->processid;
            dead->requestsTaken = getTaxi(placeHolder.driverID)->requestsTaken;
            addTaxi(deadTaxis, dead);

            /* activeTaxi--; */
            move(2, 0);
            clrtoeol();

            requestAborted++;
            mvprintw(2, 2, "Active taxis: %d/%d\t Resurrected taxis:%d", activeTaxi, map->SO_TAXI, resurrectedTaxi);

            int pid = fork();
            if (pid == 0)
            {

                sprintf(numberString, "%d", placeHolder.driverID);

                char *paramList[] = {"./bin/taxi",
                                     numberString,
                                     shmString,
                                     msgKickString,
                                     msgTimeString,
                                     msgTaCeString,
                                     msgClCaString,
                                     msgClTaString,
                                     msgReBeString,
                                     msgReDoString,
                                     NULL};
                char *environ[] = {NULL};
                if (execve("./bin/taxi", paramList, environ) == -1)
                {
                    printf("%s", strerror(errno));
                };
                refresh();
                /*printf("Taxi n%d suicidato\n", placeHolder.driverID);*/
            }
        }
        if (msgrcv(msgIDKickoff, &placeHolder, sizeof(message), 0, IPC_NOWAIT) != -1)
        {
            /*activeTaxi++;  */
            move(2, 0);
            clrtoeol();
            mvprintw(2, 2, "Active taxis: %d/%d\t Resurrected taxis:%d", activeTaxi, map->SO_TAXI, resurrectedTaxi);
            refresh();
            /*printf("Taxi n%d posizionato in x:%d, y:%d, cella a %d/%d\n", placeHolder.driverID, placeHolder.sourceX, placeHolder.sourceY, getMapCellAt(placeHolder.sourceX, placeHolder.sourceY)->currentElements, getMapCellAt(placeHolder.sourceX, placeHolder.sourceY)->maxElements);*/
        }
        if (msgrcv(msgIDClientTaken, &placeHolder, sizeof(message), 0, IPC_NOWAIT) != -1)
        {
            requestTaken++;
            updateMap = 1;
            move(3, 0);
            clrtoeol();
            mvprintw(3, 2, "Requests\tTaken:%d\tStarted:%d\tEnded:%d\tAborted:%d", requestTaken, requestBegin, requestDone, requestAborted);
        }

        if (msgrcv(msgIDRequestBegin, &placeHolder, sizeof(message), 0, IPC_NOWAIT) != -1)
        {
            requestBegin++;
            updateMap = 1;
            move(3, 0);
            clrtoeol();
            mvprintw(3, 2, "Requests\tTaken:%d\tStarted:%d\tEnded:%d\tAborted:%d", requestTaken, requestBegin, requestDone, requestAborted);
        }

        if (msgrcv(msgIDRequestDone, &placeHolder, sizeof(message), 0, IPC_NOWAIT) != -1)
        {
            requestDone++;
            updateMap = 1;
            move(3, 0);
            clrtoeol();
            mvprintw(3, 2, "Requests\tTaken:%d\tStarted:%d\tEnded:%d\tAborted:%d", requestTaken, requestBegin, requestDone, requestAborted);
        }

        /* Checking if map is to update */
        if (updateMap)
        {
            int temptaxis;
            temptaxis = 0;
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
                            temptaxis = temptaxis + getMapCellAt(a, b)->currentElements;
                        }
                    }
                }
            }
            activeTaxi = temptaxis;
            updateMap = 0;
            refresh();
        }
    };
    mvprintw(h - 1, 0, "Status: Simulation's done, removing ipc resources...");

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
    semctl(getMap()->cellsSemID, 1200, IPC_RMID);
    msgctl(msgIDKickoff, IPC_RMID, NULL);
    msgctl(msgIDTimeout, IPC_RMID, NULL);
    msgctl(msgIDTaxiCell, IPC_RMID, NULL);
    msgctl(msgIDClientCall, IPC_RMID, NULL);
    msgctl(msgIDClientTaken, IPC_RMID, NULL);
    msgctl(msgIDRequestBegin, IPC_RMID, NULL);
    msgctl(msgIDRequestDone, IPC_RMID, NULL);

    int temptaxis;
    temptaxis = 0;
    start_color();
    init_pair(0, COLOR_GREEN, COLOR_BLACK);
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
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
                    temptaxis = temptaxis + getMapCellAt(a, b)->currentElements;
                }
                else
                {
                    addch(' ');
                    temptaxis = temptaxis + getMapCellAt(a, b)->currentElements;
                }
            }
        }
    }
    activeTaxi = temptaxis;
    move(2, 0);
    clrtoeol();
    nodelay(stdscr, FALSE);
    mvprintw(2, 2, "Active taxis: %d/%d\t Resurrected taxis:%d", activeTaxi, map->SO_TAXI, resurrectedTaxi);

    mvprintw(h - 1, 0, "Status: Simulation's done, making up the statistics...");

    move(h - 1, 0);
    clrtoeol();
    refresh();
    /* making so_top_cell array */
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

    taxi *cellsMax, *timeMax, *reqMax;
    Node *tempTaxi;
    tempTaxi = deadTaxis->first;
    cellsMax = timeMax = reqMax = getTaxi(0);
    /* Seeking taxi who has done the longest distance*/
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

    /* Seeking taxi who has done the longest time*/
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

    /* Seeking taxi who took the highest request*/
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

    shmctl(shmID, IPC_RMID, NULL);
    endwin();
}

int main(int argc, char *argv[])
{
    srand(time(NULL));

    initscr();
    curs_set(0);
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    getmaxyx(stdscr, h, w);
    win = newwin(h, w, 0, 0);

    char *configPath;

    if (argc == 1)
    {
        configPath = argv[0];
    }
    else if (argc == 0)
    {
        configPath = CONFIGFULLPATH;
    }
    else
    {
        printf("Wrong number of parameters!\nusage: ./master pathToConfigFile\n");
        exit(EXIT_FAILURE);
    }

    projID = rand();
    ipcKey = ftok(configPath, projID);
    ipcKeyKickoff = ftok(configPath, projID + 1);
    ipcKeyTimeout = ftok(configPath, projID + 2);
    ipcKeyTaxiCell = ftok(configPath, projID + 3);
    ipcKeyClientCall = ftok(configPath, projID + 4);
    ipcKeyClientTaken = ftok(configPath, projID + 5);
    ipcKeyRequestBegin = ftok(configPath, projID + 6);
    ipcKeyRequestDone = ftok(configPath, projID + 7);

    map = mapFromConfig(CONFIGFULLPATH);

    beFruitful();

    return EXIT_SUCCESS;
}