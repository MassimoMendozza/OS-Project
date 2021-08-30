#ifndef TAXIELEMENTS_H
#define TAXIELEMENTS_H
#include <sys/types.h>

/*
Struct for client process
*/
typedef struct _person
{
	pid_t processid;
	int number;
	int requestDone;
	int posX;
	int posY;
} person;

/*
Struct for taxi process
*/
typedef struct _taxi
{
	int posX;
	int posY;
	pid_t processid;
	int number;
	long maxTime;
	int requestsTaken, distanceDone;
} taxi;

/*
Struct for map cell
*/
typedef struct _mapCell
{
	int maxElements, topCell, currentElements, holdingTime, cantPutAnHole, clientID, passedBy;
} mapCell;

/*
Struct for map's information
*/
typedef struct _masterMap
{
	int SO_WIDTH, SO_HEIGHT, SO_HOLES, SO_TOP_CELLS, SO_SOURCES, SO_CAP_MIN, SO_CAP_MAX, SO_TAXI, SO_TIMENSEC_MIN, SO_TIMENSEC_MAX, SO_TIMEOUT, SO_DURATION;
	int cellsSemID;
	int msgIDKickoff,
		msgIDTimeout,
		msgIDTaxiCell,
		msgIDClientCall,
		msgIDClientTaken,
		msgIDRequestBegin,
		msgIDRequestDone;
	pid_t masterProcessID;
	mapCell ***map;
} masterMap;

/*
Message struct
*/
typedef struct _message
{
	int x, y, driverID, clientID;
	long mtype;
} message;

#endif