# OS Project Data Structures

Mappa

```c
struct masterMap{
	int SO_WIDTH;
	int SO_HEIGHT;
	int SO_HOLES;
	int SO_TOP_CELLS;
	int SO_SOURCES;
	int SO_CAP_MIN;
	int SO_CAP_MAX;
	int SO_TAXI;
	int SO_TIMENSEC_MIN;
	int SO_TIMENSEC_MAX;
	int SO_TIMEOUT;
	int SO_DURATION;
	taxi[SO_TAXI] allDrivers;
	person[SO_SOURCES] allClients;
//coda movement
  requests requestList;
	mapCell[][] map;
}
```

Request

```c
struct requestMessage{
	int sourceX, sourceY;
	int destX, destY;
	person client;
	taxi driver;
	int status;
}
```

Movement Notification

```c
struct Notification{
	int fromX, fromY;
	int toX, toY;
	int flag;
	taxi driver;
	person cient;
}
```

Cell

```c
struct mapCell{
	int maxElements; //0 if inaccessible
	int currentElements;
	int holdingTime;
	taxi[] drivers;
	person[] clients;
//puntatore semaforo
}
```

Taxi

```c
struct taxi{
	pid processID;
	int distanceDone;
	int ridesDone;
	person client; 
}
```

Person

```c
struct person{
	pid processID;
	int isBeingRidden;
	taxi driver;
}

```

Requests

```c
struct requests{
	//coda requests
	//semaforo requests
	requestMessage[] done;
	requestMessage[] aborted;
	requestMessage[] processing;
}
```

Altro

- Semafori per le celle
- Semafori per esaudimento richieste
- Coda richieste
- Coda movementNotification
- Condivisione memoria per strutture