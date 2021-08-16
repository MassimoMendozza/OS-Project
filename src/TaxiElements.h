#ifndef TAXIELEMENTS_H
#define TAXIELEMENTS_H
#include <sys/types.h>
#define MSG_KICKOFF 0

typedef struct _person
{
	int processid;
	int number;
	int isOnTaxi;
} person;

typedef struct _taxi
{
	int processid;
	int number;
	int distanceDone, ridesDone;
	person *client;
} taxi;

typedef struct _mapCell
{
	int maxElements, currentElements, holdingTime, cantPutAnHole;
	taxi **drivers;
	person **clients;
} mapCell;

typedef struct _masterMap
{
	int SO_WIDTH, SO_HEIGHT, SO_HOLES, SO_TOP_CELLS, SO_SOURCES, SO_CAP_MIN, SO_CAP_MAX, SO_TAXI, SO_TIMENSEC_MIN, SO_TIMENSEC_MAX, SO_TIMEOUT, SO_DURATION;
	int cellsSemID;
	taxi **allDrivers;
	person **allClients;
	mapCell ***map;
} masterMap;

typedef struct _message
{
	long mtype;
	int sourceX, sourceY, destX, destY;
	int clientID;
	int driverID;
	int type;
} message;
typedef struct _requests
{
	/*coda richieste*/
	int semID;
	int maxDone, maxAborted, maxProcessing, numDone, numAborted, numProcessing;
	message **done, **aborted, **processing;
} requests;
#endif