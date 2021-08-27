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
	int isOnTaxi;
	int posX;
	int posY;
} person;


typedef struct _taxi  /*oggetto taxi*/
{
	int posX;
	int posY;
	int processid;
	int number;
	int distanceDone, ridesDone;
	person *client;
} taxi;


typedef struct _mapCell  /*oggetto mappa della città*/
{
	int maxElements, currentElements, holdingTime, cantPutAnHole, isAvailable;
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
	int sourceX, sourceY, destX, destY;
	int driverID;
	long mtype;
} message;


typedef struct _requests /*oggetto richiesta*/
{
	/*coda richieste*/
	int semID;
	int maxDone, maxAborted, maxProcessing, numDone, numAborted, numProcessing;
	message **done, **aborted, **processing;
} requests;



#endif