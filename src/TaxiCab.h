#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <semaphore.h>
#include <sys/shm.h>

#include "TaxiElements.h"
#include "shmUtils.h"
#include "BinSemaphores.h"
#include "taxi.h"
#include "client.c"