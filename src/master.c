#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

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

key_t ipcKey;
int projID;

masterMap *map;
int shmID,msgID;
void *addrstart;

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

    printf("\033[H\033[J");
    printf("Reading config file from %s...\n\n", configPath);
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
    mapCell *cells = map->map = getMapCellAt(0, 0);
    map->cellsSemID = semget(ipcKey, map->SO_HEIGHT * map->SO_WIDTH, IPC_CREAT | 0666);
    initSemAvailable(map->cellsSemID, map->SO_HEIGHT * map->SO_WIDTH);
    for (x = 0; x < map->SO_WIDTH; x++)
    {
        for (y = 0; y < map->SO_HEIGHT; y++)
        {
            cells = getMapCellAt(x, y);
            cells->maxElements = randFromRange(map->SO_CAP_MIN, map->SO_CAP_MAX);
            cells->holdingTime = randFromRange(map->SO_TIMENSEC_MIN, map->SO_TIMENSEC_MAX);
            cells->currentElements = 0;
            cells->cantPutAnHole = 0;
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
    printf("The following parameters have been loaded:\n\tSO_WIDTH = %d\n\tSO_HEIGHT = %d\n\tSO_HOLES = %d\n\tSO_TOP_CELLS = %d\n\tSO_SOURCES = %d\n\tSO_CAP_MIN = %d\n\tSO_CAP_MAX = %d\n\tSO_TAXI = %d\n\tSO_TIMENSEC_MIN = %d\n\tSO_TIMENSEC_MAX = %d\n\tSO_TIMEOUT = %d\n\tSO_DURATION = %d\n\n", map->SO_WIDTH, map->SO_HEIGHT, map->SO_HOLES, map->SO_TOP_CELLS, map->SO_SOURCES, map->SO_CAP_MIN, map->SO_CAP_MAX, map->SO_TAXI, map->SO_TIMENSEC_MIN, map->SO_TIMENSEC_MAX, map->SO_TIMEOUT, map->SO_DURATION);

    printf("Creating %d holes...\n", map->SO_HOLES);
    for (c = 0; c < map->SO_HOLES; c++)
    {
        x = randFromRange(0, map->SO_WIDTH - 1) - 1;
        y = randFromRange(0, map->SO_HEIGHT - 1) - 1;
        cells = getMapCellAt(x + 1, y + 1);
        if (cells->cantPutAnHole)
        {
            a = x + 1;
            b = y + 1;
            int limitx, limity;
            limitx = map->SO_WIDTH;
            limity = map->SO_HEIGHT;
            while (a < limitx)
            {
                for (; a < limitx; a++)
                {
                    for (; b < limity; b++)
                    {
                        cells = getMapCellAt(a, b);
                        if (cells->cantPutAnHole)
                        {
                            limitx = a;
                            limity = b;
                            x = a - 1;
                            y = b - 1;
                        }
                    }
                }
                if (a == map->SO_WIDTH)
                {
                    limitx = x + 1;
                    limity = y + 1;
                    a = b = 0;
                }
            }
            if (a == limitx)
            {
                printf("Cannot find a holes configuration with the given parameters, check your config parameters or try again.\nMaster program will now exit.");
                exit(EXIT_FAILURE);
            }
        }

        for (a = 0; a < (x + 3) && a < map->SO_WIDTH; a++)
        {
            for (b = 0; b < (y + 3) && b < map->SO_HEIGHT; b++)
            {
                cells = getMapCellAt(a, b);
                cells->cantPutAnHole = 1;
            }
        }
        cells = getMapCellAt(x + 1, y + 1);
        cells->maxElements = -1;
    }
    printf("\n");
}

masterMap *mapFromConfig(char *configPath)
{
    masterMap *map = readConfig(configPath);

    shmID = allocateShm(ipcKey, map);
    msgID = msgget(ipcKey, IPC_CREAT | 0666);
    addrstart = shmat(shmID, NULL, 0);

    setAddrstart(addrstart);
    putMapInShm(map);

    initializeMapCells();
    createHoles();

    return map;
}

void beFruitful()
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
            a = map->SO_TAXI;
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
    printf("\n\nMaster's speaking\n\n");

    int a;
    message kickoffMessage;

    kickoffMessage.type=MSG_KICKOFF;
    for(a=0; a<map->SO_TAXI; a++){
        kickoffMessage.mtype=(long)(getTaxi(a)->processid);
        if((msgsnd(msgID, &kickoffMessage,sizeof(kickoffMessage),  0))==-1){
            printf("Can't send message to kickoff taxi n%d", getTaxi(a)->processid);
        };
    }
    
    for(a=0; a<map->SO_SOURCES; a++){
        kickoffMessage.mtype=(long)(getPerson(a)->processid);
        if((msgsnd(msgID, &kickoffMessage,sizeof(kickoffMessage), 0))==-1){
            printf("Can't send message to kickoff taxi n%d", getPerson(a)->processid);
        };
    }

    
    while(1){
        /* checking if someone's killed itself*/
        message placeHolder;
        if(msgrcv(msgID, &placeHolder, sizeof(placeHolder), MSG_TIMEOUT, IPC_NOWAIT)!=-1){
            printf("Taxi n%d suicidato\n", placeHolder.driverID);
        }
    };
}

int main(int argc, char *argv[])
{
    srand(time(0));

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
