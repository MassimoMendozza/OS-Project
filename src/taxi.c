#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <signal.h>



#ifndef SHMUTILS_H
#define SHMUTILS_H
#include "shmUtils.h"
#endif
#ifndef TAXIELEMENTS_H
#include "TaxiElements.h"
#endif



int msgID;
int myTaxiNumber;



void bornATaxi(int myNumber){
    
    myTaxiNumber = myNumber;/*ti amo <3*/

    printf("Taxi n%d with pid %d\n", myNumber, getpid());

    taxi *myself = getTaxi(myNumber);
    myself->processid = getpid();
    myself->number = myNumber;
    myself->client = NULL;
    myself->distanceDone = 0;
    myself->ridesDone = 0;

    message msgPlaceholder;

    /*
        Waiting the simulation kickoff
    */

    if((msgrcv(msgID, &msgPlaceholder, sizeof(msgPlaceholder), getpid(),0))==-1){
        printf("Qualcosa non va bro");
        exit(EXIT_FAILURE);
    }else{
        if(msgPlaceholder.type==MSG_KICKOFF){
            
        taxiKickoff();
        }else{
            printf("%d\n", msgPlaceholder.type);
        }
    }


}


void guida_taxi(taxi map, mapCell destination){ //muove il taxi

    //cambiare parametri, rivedere il sistema delle matrici e controllo buchi/muri

    while (!(map->xcoord==destination->xcoord&&map->xcoord==destination->xcoord)){

        if (map->xcoord< destination->xcoord){ //caso 1

            if (map->ycoord< destination->ycoord){  //caso 1.1
                if (map[x taxi +1][y taxi +1]!= hole){        /**/
                    map->xcoord++;  //ti sposti a dx
                    map->ycoord++;  //ti sposti in alto
                }
                else if (map[x taxi +1][y taxi]!=muro){       /**/
                    map->xcoord--;
                    map->ycoord++;
                }
                else{  
                    map->xcoord=map->xcoord +2; //ti sposti a dx
                    map->ycoord++;
                }
            }

            else if(map->ycoord> destination->ycoord){  //caso 1.2
                if (map[x taxi +1][y taxi-1]!=hole){        /**/
                    map->xcoord++; //ti sposti a dx
                    map->ycoord--;  //ti sposti in basso
                }
                else if(map[x taxi+1][y taxi]!=muro){       /**/
                    map->xcoord--;
                    map->ycoord--;
                }
                else{
                    map->xcoord=map->xcoord+2;  //ti sposti a dx
                    map->ycoord--; //ti sposti in basso
                }
            }

            else{  //caso 1.3 orizzontale
                if (map[x taxi+1][y taxi]!=hole){      /**/
                    map->xcoord++; //vai a destra
                }
                else{ 
                    map->xcoord++; //vai a destra
                    map->ycoord++; //vai in alto
                }
            }
        }

        else if(map->xcoord> destination->xcoord){ //caso 2
            if (map->ycoord< destination->ycoord){ //caso 2.1
                if (map[x taxi-1][y taxi +1]!=hole){         /**/
                    map->xcoord--; //ti sposti a sx
                    map->ycoord++; //ti sposti in alto
                }
                else if(map[x taxi -1][y taxi]!=muro){       /**/
                    map->xcoord++;
                    map->ycoord--;
                }
                else{
                    map->xcoord= map->xcoord-2; //ti sposti a sx
                    map->ycoord++; //ti sposti in alto
                }
            }
            
            else if(map->ycoord>destination->ycoord){ //caso 2.2
                if(map[x taxi-1][y taxi-1]){            /**/
                    map->xcoord--; //ti sposti a sx
                    map->ycoord--; //ti sposti in basso
                }
                else if (map[taxi x-1][taxi y]!=muro){      /**/
                    map->xcoord++;
                    map->ycoord--;
                }
                else{
                    map->xcoord=map->xcoord-2; //ti sposti in basso
                    map->ycoord--;
                }
            }
            
            else{ //caso 2.3 orizzontale
                if(map[x taxi][y taxi-1]!=hole){       /**/
                    map->ycoord--; //ti sposti in basso
                }
                else{
                    map->xcoord--; //ti sposti a sx
                    map->ycoord--; //ti sposti in basso
                }
            }
        }

        else{ //caso 3
            if (map->ycoord< destination->ycoord){ //caso 3.1
                if(map[x taxi][y taxi+1]!=hole){               /**/
                    map->ycoord++; //ti muovi in alto
                }
                else if(map[x taxi+1][y taxi]!=muro){          /**/
                    map->xcoord++; //ti muovi a dx
                    map->ycoord++; //ti muovi in alto
                }
                else{
                    map->xcoord--;
                    map->ycoord++;
                }
            }
            
            else if(map->ycoord>destination->ycoord){ //caso 3.2
                if(map[x taxi][y taxi-1]!=hole){           /**/
                    map->ycoord--; //ti muovi in basso
                }
                else if(map[x taxi-1][y taxi]!=muro){      /**/
                    map->xcoord--;
                    map->ycoord--;
                }
                else{
                    map->xcoord++; //ti muovi a sx
                    map->ycoord--; //ti muovi in basso
                }
            }
        }
    }
}

static void alarmHandler(int signalNum){
    printf("Taxi n%d si è suicidato\n", getpid());
    exit(EXIT_FAILURE);
}

void taxiKickoff(){
    printf("Yay, taxi n%d andato\n", myTaxiNumber);
    if(signal(SIGALRM, alarmHandler)==SIG_ERR){
        printf("Something's wrong on signal handler change");
    };
    alarm(1);
    while(1);
}