#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <signal.h>
#include <sys/types.h>



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


void guida_taxi(taxi bro, mapCell destination, masterMap mappa){ //muove il taxi

    //cambiare parametri, rivedere il sistema delle matrici e controllo buchi/muri

    while (!(bro->xcoord==destination->xcoord&&bro->xcoord==destination->xcoord)){

        if (bro->xcoord< destination->xcoord){ //caso 1

            if (bro->ycoord< destination->ycoord){  //caso 1.1
                if (mappa[bro->xcoord+1][bro->ycoord+1]!= hole){        /**/
                    bro->xcoord++;  //ti sposti a dx
                    bro->ycoord++;  //ti sposti in alto
                }
                else if (mappa[bro->xcoord+1][bro->ycoord]!=muro){       /**/
                    bro->xcoord--;
                    bro->ycoord++;
                }
                else{  
                    bro->xcoord=bro->xcoord +2; //ti sposti a dx
                    bro->ycoord++;
                }
            }

            else if(bro->ycoord> destination->ycoord){  //caso 1.2
                if (mappa[bro->xcoord+1][bro->ycoord-1]!=hole){        /**/
                    bro->xcoord++; //ti sposti a dx
                    bro->ycoord--;  //ti sposti in basso
                }
                else if(mappa[bro->xcoord+1][bro->ycoord]!=muro){       /**/
                    bro->xcoord--;
                    bro->ycoord--;
                }
                else{
                    bro->xcoord=bro->xcoord+2;  //ti sposti a dx
                    bro->ycoord--; //ti sposti in basso
                }
            }

            else{  //caso 1.3 orizzontale
                if (mappa[bro->xcoord+1][bro->ycoord]!=hole){      /**/
                    bro->xcoord++; //vai a destra
                }
                else{ 
                    bro->xcoord++; //vai a destra
                    bro->ycoord++; //vai in alto
                }
            }
        }

        else if(bro->xcoord> destination->xcoord){ //caso 2
            if (bro->ycoord< destination->ycoord){ //caso 2.1
                if (mappa[bro->xcoord-1][bro->ycoord+1]!=hole){         /**/
                    bro->xcoord--; //ti sposti a sx
                    bro->ycoord++; //ti sposti in alto
                }
                else if(mappa[bro->xcoord-1][bro->ycoord]!=muro){       /**/
                    bro->xcoord++;
                    bro->ycoord--;
                }
                else{
                    bro->xcoord= bro->xcoord-2; //ti sposti a sx
                    bro->ycoord++; //ti sposti in alto
                }
            }
            
            else if(bro->ycoord>destination->ycoord){ //caso 2.2
                if(mappa[bro->xcoord-1][bro->ycoord-1]!=hole){            /**/
                    bro->xcoord--; //ti sposti a sx
                    bro->ycoord--; //ti sposti in basso
                }
                else if (mappa[bro->xcoord-1][bro->ycoord]!=muro){      /**/
                    bro->xcoord++;
                    bro->ycoord--;
                }
                else{
                    bro->xcoord=bro->xcoord-2; //ti sposti in basso
                    bro->ycoord--;
                }
            }
            
            else{ //caso 2.3 orizzontale
                if(mappa[bro->xcoord][bro->ycoord-1]!=hole){       /**/
                    bro->ycoord--; //ti sposti in basso
                }
                else{
                    bro->xcoord--; //ti sposti a sx
                    bro->ycoord--; //ti sposti in basso
                }
            }
        }

        else{ //caso 3
            if (bro->ycoord< destination->ycoord){ //caso 3.1
                if(mappa[bro->xcoord][bro->ycoord+1]!=hole){               /**/
                    bro->ycoord++; //ti muovi in alto
                }
                else if(mappa[bro->xcoord+1][bro->ycoord]!=muro){          /**/
                    bro->xcoord++; //ti muovi a dx
                    bro->ycoord++; //ti muovi in alto
                }
                else{
                    bro->xcoord--;
                    bro->ycoord++;
                }
            }
            
            else if(bro->ycoord>destination->ycoord){ //caso 3.2
                if(mappa[bro->xcoord][bro->ycoord-1]!=hole){           /**/
                    bro->ycoord--; //ti muovi in basso
                }
                else if(mappa[bro->xcoord-1][bro->ycoord]!=muro){      /**/
                    bro->xcoord--;
                    bro->ycoord--;
                }
                else{
                    bro->xcoord++; //ti muovi a sx
                    bro->ycoord--; //ti muovi in basso
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
/*
    struct sigaction idleAlarm;
    sigset_t idleMask;

    idleAlarm.sa_handler = &alarmHandler;
    idleAlarm.sa_flags=0;
    sigemptyset(&idleMask);
    sigaddset(&idleMask, SIGALRM);
    idleAlarm.sa_mask = idleMask;
*/

    if(signal(SIGALRM, &alarmHandler)==SIG_ERR){
        printf("Something's wrong on signal handler change");
    };
    alarm(1);
    while(1);
}