#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ncurses.h>

#include <semaphore.h>

#include <sys/shm.h>
#include <errno.h>

#include "TaxiCab.h"

#ifndef CONFIGFULLPATH
#define CONFIGFULLPATH "./config"
#endif

/*
Variables used to parse config file
*/

FILE *config;
char AttributeName[20];
char EqualsPlaceHolder;
int ParsedValue;
int updateMap;

key_t ipcKey;
int projID;

masterMap *map;
int shmID, msgID, activeTaxi, updatedmap;
updatedmap = 0;
void *addrstart; /*the addres of the shared memory portion (first element is map)*/

WINDOW *win;
int h, w, a;

void alarmMaster(int sig)
{
    endwin();
    int a;
    for (a = 0; a < getMap()->SO_SOURCES; a++)
    {
        kill(SIGKILL, getPerson(a)->processid);
    }
    for (a = 0; a < getMap()->SO_TAXI; a++)
    {
        kill(SIGKILL, getTaxi(a)->processid);
    }
    msgctl(msgID, IPC_RMID, NULL);
    shmctl(shmID, IPC_RMID, NULL);
    /*remove sme array*/
    exit(EXIT_SUCCESS);
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
            cells->isAvailable = 1;
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
                getMapCellAt(x, y)->isAvailable = 0;

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

    shmID = allocateShm(ipcKey, map);         /*create the shared memory*/
    msgID = msgget(ipcKey, IPC_CREAT | 0666); /*create the queue of messages*/
    addrstart = shmat(shmID, NULL, 0);        /*return the addres of the shared memory portion*/

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

    shouldIBeATaxi = shouldIBeAClient = 0;
    for (a = 0; a < map->SO_TAXI; a++)
    {
        if (fork() == 0)
        {
            shouldIBeATaxi = 1;
            bornATaxi(a);
            a = map->SO_TAXI; /*break*/
        }
    }
    if (!shouldIBeATaxi)
    {
        for (a = 0; a < map->SO_SOURCES; a++)
        {
            if (fork() == 0)
            {
                shouldIBeAClient = 1;
                bornAClient(a);
                a = map->SO_SOURCES;
            }
        }
    }

    if (!shouldIBeAClient && !shouldIBeATaxi)
    {
        bornAMaster();
    }
}

void bornAMaster()
{
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
        if ((msgrcv(msgID, &placeHolder, sizeof(message), MSG_TAXI_CELL, 0)) == -1)
        {

            /* printw("Can't receive message to kickoff taxi n%d", getTaxi(a)->processid);
            refresh();*/
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
        if (msgrcv(msgID, &placeHolder, sizeof(message), MSG_KICKOFF, IPC_NOWAIT) != -1)
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
    kickoffMessage.mtype = MSG_KICKOFF;

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

    move(h - 1, 0);
    clrtoeol();
    mvprintw(h - 1, 0, "Status: Simulation's going.");
    alarm(getMap()->SO_DURATION);
    refresh();

    move(2, 0);
    clrtoeol();
    mvprintw(2, 2, "Active taxis: %d/%d   ", activeTaxi, map->SO_TAXI);
    refresh();

    int requestTaken, requestBegin, requestDone, requestAborted;
    requestBegin = requestDone = requestTaken = requestAborted = 0;
    while (1)
    {

        message placeHolder;
        if ((msgrcv(msgID, &placeHolder, sizeof(message), MSG_TAXI_CELL, IPC_NOWAIT)) != -1)
        {
            kill(getTaxi(placeHolder.driverID)->processid, SIGUSR1);

        } /* checking if someone's killed itself*/
        if (msgrcv(msgID, &placeHolder, sizeof(message), MSG_TIMEOUT, IPC_NOWAIT) != -1)
        {
            activeTaxi--;
            move(2, 0);
            clrtoeol();
            reserveSem(getMap()->cellsSemID, (placeHolder.sourceX * getMap()->SO_HEIGHT) + placeHolder.sourceY);
            getMapCellAt(placeHolder.sourceX, placeHolder.sourceY)->currentElements--;
            releaseSem(getMap()->cellsSemID, (placeHolder.sourceX * getMap()->SO_HEIGHT) + placeHolder.sourceY);
            if((placeHolder.destX!=-1)){
                reserveSem(getMap()->cellsSemID, (placeHolder.destX * getMap()->SO_HEIGHT) + placeHolder.destY);
            getMapCellAt(placeHolder.destX, placeHolder.destY)->isAvailable=1;
            releaseSem(getMap()->cellsSemID, (placeHolder.destX * getMap()->SO_HEIGHT) + placeHolder.destY);
            
            }
            requestAborted++;
            mvprintw(2, 2, "Active taxis: %d/%d", activeTaxi, map->SO_TAXI);
            if (fork() == 0)
            {
                bornATaxi(placeHolder.driverID);
            }
            refresh();
            /*printf("Taxi n%d suicidato\n", placeHolder.driverID);*/
        }
        if (msgrcv(msgID, &placeHolder, sizeof(message), MSG_KICKOFF, IPC_NOWAIT) != -1)
        {
            activeTaxi++;
            move(2, 0);
            clrtoeol();
            mvprintw(2, 2, "Active taxis: %d/%d", activeTaxi, map->SO_TAXI);
            refresh();
            /*printf("Taxi n%d posizionato in x:%d, y:%d, cella a %d/%d\n", placeHolder.driverID, placeHolder.sourceX, placeHolder.sourceY, getMapCellAt(placeHolder.sourceX, placeHolder.sourceY)->currentElements, getMapCellAt(placeHolder.sourceX, placeHolder.sourceY)->maxElements);*/
        }
        if (msgrcv(msgID, &placeHolder, sizeof(message), MSG_CLIENT_TAKEN, IPC_NOWAIT) != -1)
        {
            requestTaken++;
            updateMap = 1;
            move(3, 0);
            clrtoeol();
            mvprintw(3, 2, "Requests\tTaken:%d\tStarted:%d\tEnded:%d\tAborted:%d", requestTaken, requestBegin, requestDone, requestAborted);
            refresh();
        }

        if (msgrcv(msgID, &placeHolder, sizeof(message), MSG_REQUEST_BEGIN, IPC_NOWAIT) != -1)
        {
            requestBegin++;
            updateMap = 1;
            move(3, 0);
            clrtoeol();
            mvprintw(3, 2, "Requests\tTaken:%d\tStarted:%d\tEnded:%d\tAborted:%d", requestTaken, requestBegin, requestDone, requestAborted);
            refresh();
        }

        if (msgrcv(msgID, &placeHolder, sizeof(message), MSG_REQUEST_DONE, IPC_NOWAIT) != -1)
        {
            requestDone++;
            updateMap = 1;
            move(3, 0);
            clrtoeol();
            mvprintw(3, 2, "Requests\tTaken:%d\tStarted:%d\tEnded:%d\tAborted:%d", requestTaken, requestBegin, requestDone, requestAborted);
            refresh();
        }

        /* Checking if map is to update */
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

    map = mapFromConfig(CONFIGFULLPATH);

    beFruitful();

    return EXIT_SUCCESS;
}