#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
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

void initializeMapCells(masterMap *map)
{
    int x, y;
    map->map = (mapCell ***)malloc(sizeof(mapCell *) * map->SO_WIDTH * map->SO_HEIGHT);
    for (x = 0; x < map->SO_WIDTH; x++)
    {
        map->map[x] = (mapCell **)malloc(sizeof(mapCell **) * map->SO_HEIGHT);
        for (y = 0; y < map->SO_HEIGHT; y++)
        {
            map->map[x][y] = (mapCell *)malloc(sizeof(mapCell));
            map->map[x][y]->maxElements = randFromRange(map->SO_CAP_MIN, map->SO_CAP_MAX);
            map->map[x][y]->holdingTime = randFromRange(map->SO_TIMENSEC_MIN, map->SO_TIMENSEC_MAX);
            map->map[x][y]->currentElements = 0;
        }
    }
}

masterMap *mapFromConfig(char *configPath)
{
    masterMap *map = readConfig(configPath);
    initializeMapCells(map);
    return map;
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

    masterMap *map = mapFromConfig(CONFIGFULLPATH);

    printf("The following parameters have been loaded:\n\tSO_WIDTH = %d\n\tSO_HEIGHT = %d\n\tSO_HOLES = %d\n\tSO_TOP_CELLS = %d\n\tSO_SOURCES = %d\n\tSO_CAP_MIN = %d\n\tSO_CAP_MAX = %d\n\tSO_TAXI = %d\n\tSO_TIMENSEC_MIN = %d\n\tSO_TIMENSEC_MAX = %d\n\tSO_TIMEOUT = %d\n\tSO_DURATION = %d\n\n", map->SO_WIDTH, map->SO_HEIGHT, map->SO_HOLES, map->SO_TOP_CELLS, map->SO_SOURCES, map->SO_CAP_MIN, map->SO_CAP_MAX, map->SO_TAXI, map->SO_TIMENSEC_MIN, map->SO_TIMENSEC_MAX, map->SO_TIMEOUT, map->SO_DURATION);

    return EXIT_SUCCESS;
}
