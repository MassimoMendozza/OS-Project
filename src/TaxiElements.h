#ifndef TAXIELEMENTS_H
#define TAXIELEMENTS_H
#include <sys/types.h>
#define MSG_KICKOFF 1
#define MSG_TIMEOUT 2
#define MSG_TAXI_CELL 3
#define MSG_CLIENT_CALL 5
#define MSG_CLIENT_TAKEN 6
#define MSG_REQUEST_BEGIN 7
#define MSG_REQUEST_DONE 8

typedef struct _person /*oggetto persona*/
{
	int processid;
	int number;
	int requestDone;
	int posX;
	int posY;
} person;

typedef struct _taxi  /*oggetto taxi*/
{
	int posX;
	int posY;
	int processid;
	int number;
	long maxTime;
	int distanceDone, ridesDone;
} taxi;

typedef struct _mapCell  /*oggetto mappa della città*/
{
	int maxElements, topCell,currentElements, holdingTime, cantPutAnHole, clientID, passedBy;
} mapCell;

typedef struct _masterMap /*oggetto mappa città*/
{
	int SO_WIDTH, SO_HEIGHT, SO_HOLES, SO_TOP_CELLS, SO_SOURCES, SO_CAP_MIN, SO_CAP_MAX, SO_TAXI, SO_TIMENSEC_MIN, SO_TIMENSEC_MAX, SO_TIMEOUT, SO_DURATION;
	int cellsSemID;
	pid_t masterProcessID;
	taxi **allDrivers;
	person **allClients;
	mapCell ***map;
} masterMap;

typedef struct _message /*oggetto messaggio (?)*/
{
	int x,y,driverID,clientID;
	long mtype;
} message;

#endif