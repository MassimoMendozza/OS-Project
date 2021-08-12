# OS Project Functions

readConfig (mapCell map, string configPath)
initializeMap (mapCell map) //qui vengono create anche code e semafori
   createHoles (mapCell map) //creo buchi a caso
initializeDrivers (mapCell map)
initializeClients (mapCell map)
kickSimulation (mapCell map) //manda segnale di partenza e inizializza allarmi per refresh

requestMaker()
requestBinder()

requestSeeker()
requestCommunicator()
pathFinder()
moveTaxi()
timeoutRefresher()

int initSemAvailable(int semId, int semNum);
int initSemInUse(int semId, int semNum);
int reserveSem(int semId, int semNum);
int releaseSem(int semId, int semNum);