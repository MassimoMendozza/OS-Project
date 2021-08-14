#include <sys/types.h>

typedef struct _person
{
	pid_t processid;
	int number;
	int isOnTaxi;
} person;

typedef struct _taxi
{
	pid_t processid;
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

typedef struct _requestMessage
{
	int sourceX, sourceY, destX, destY;
	person *client;
	taxi *driver;
	int status;
} requestMessage;
typedef struct _requests
{
	/*coda richieste*/
	int semID;
	int maxDone, maxAborted, maxProcessing, numDone, numAborted, numProcessing;
	requestMessage **done, **aborted, **processing;
} requests;

typedef struct _notification
{
	int fromX, fromY, toX, toY, flag;
	taxi *driver;
	person *client;
} notification;
