#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "TaxiCab.h"

#ifndef SO_WIDTH
#define SO_WIDTH 100
#endif

#ifndef SO_HEIGHT
#define SO_HEIGHT 100
#endif

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

int SO_HOLES = -1;
int SO_TOP_CELLS = -1;
int SO_SOURCES = -1;
int SO_CAP_MIN = -1;
int SO_CAP_MAX = -1;
int SO_TAXI = -1;
int SO_TIMENSEC_MIN = -1;
int SO_TIMENSEC_MAX = -1;
int SO_TIMEOUT = -1;
int SO_DURATION = -1;

*masterMap readConfig(char* configPath){
    /*
    Parsing config file
    */

    printf("\033[H\033[J");
    printf("Reading config file from " CONFIGFULLPATH "...\n\n");
    if ((config = (FILE *)fopen(CONFIGFULLPATH, "r")) == NULL)
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
                SO_HOLES = ParsedValue;
            }
            else if (strcmp(AttributeName, "SO_TOP_CELLS") == 0)
            {
                SO_TOP_CELLS = ParsedValue;
            }
            else if (strcmp(AttributeName, "SO_SOURCES") == 0)
            {
                SO_SOURCES = ParsedValue;
            }
            else if (strcmp(AttributeName, "SO_CAP_MIN") == 0)
            {
                SO_CAP_MIN = ParsedValue;
            }
            else if (strcmp(AttributeName, "SO_CAP_MAX") == 0)
            {
                SO_CAP_MAX = ParsedValue;
            }
            else if (strcmp(AttributeName, "SO_TAXI") == 0)
            {
                SO_TAXI = ParsedValue;
            }
            else if (strcmp(AttributeName, "SO_TIMENSEC_MIN") == 0)
            {
                SO_TIMENSEC_MIN = ParsedValue;
            }
            else if (strcmp(AttributeName, "SO_TIMENSEC_MAX") == 0)
            {
                SO_TIMENSEC_MAX = ParsedValue;
            }
            else if (strcmp(AttributeName, "SO_TIMEOUT") == 0)
            {
                SO_TIMEOUT = ParsedValue;
            }
            else if (strcmp(AttributeName, "SO_DURATION") == 0)
            {
                SO_DURATION = ParsedValue;
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

    if((SO_HOLES == -1) || (SO_TOP_CELLS == -1) || (SO_CAP_MIN == -1) || (SO_CAP_MAX == -1) || (SO_TAXI == -1) || (SO_TIMENSEC_MIN== -1) || (SO_TIMENSEC_MAX == -1) || (SO_TIMEOUT == -1) || (SO_DURATION == -1) || (SO_SOURCES == -1) ){
        printf("Not all parameters have been initialized, check your config file and try again.\nMaster process will now exit.\n");
        exit(1);
    }

    /*
    The parameters are correctly loaded
    */

    printf("The following parameters have been loaded:\n\tSO_HOLES = %d\n\tSO_TOP_CELLS = %d\n\tSO_SOURCES = %d\n\tSO_CAP_MIN = %d\n\tSO_CAP_MAX = %d\n\tSO_TAXI = %d\n\tSO_TIMENSEC_MIN = %d\n\tSO_TIMENSEC_MAX = %d\n\tSO_TIMEOUT = %d\n\tSO_DURATION = %d\n\n",SO_HOLES,SO_TOP_CELLS,SO_SOURCES,SO_CAP_MIN,SO_CAP_MAX,SO_TAXI,SO_TIMENSEC_MIN,SO_TIMENSEC_MAX,SO_TIMEOUT,SO_DURATION);
    
    
    return *newMap;
}
int main(int argc, char *argv[])
{

}
