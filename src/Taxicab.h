typedef struct _masterMap{
	int SO_WIDTH, SO_HEIGHT, SO_HOLES, SO_TOP_CELLS, SO_SOURCES, SO_CAP_MIN, SO_CAP_MAX, SO_TAXI, SO_TIMENSEC_MIN, SO_TIMENSEC_MAX, SO_TIMEOUT, SO_DURATION;
	taxi[SO_TAXI] allDrivers;
	person[SO_SOURCES] allClients;
	**mapCell map;
} masterMap;

typedef struct _mapCell{
	int maxElements,currentElements, holdingTime;
	**taxi drivers;
	**person clients;
	//semaphore of the cell
} mapCell;

typedef struct _taxi{
	//process id
	int distanceDone, ridesDone;
	*person client;
} taxi;

typedef struct _person{
	//process id
	int isOnTaxi;
	*taxi driver;
} person;

typedef struct _requests{
	//coda richieste
	//semaforo richieste
	int maxDone, maxAborted, maxProcessing, numDone, numAborted, numProcessing;
	**requestMessage done, aborted, processing;
} requests;

typedef struct _requestMessage{
	int sourceX, sourceY, destX, destY;
	*person client;
	*taxi driver;
	int status;
} requestMessage;

typedef struct _notification{
	int fromX, fromY, toX, toY, flag;
	*taxi driver;
	*person client;
} notification;
