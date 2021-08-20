
int distanza = distance(x taxi, x arrivo, y taxi, y arrivo);



void guida_taxi(){

    if (x taxi< x arrivo){ //caso 1

        if (y taxi< y arrivo){  //caso 1.1
            if (map[x taxi +1][y taxi +1]!= hole){
                x taxi++;  //ti sposti a dx
                y taxi++;  //ti sposti in alto
            }
            else {  
                x taxi++; //ti sposti a dx
            }
        }
        else if(y taxi> y arrivo){  //caso 1.2
            if (map[x taxi +1][y taxi-1]!=hole){
                x taxi ++; //ti sposti a dx
                y taxi--;  //ti sposti in basso
            }
            else {
                x taxi++;  //ti sposti a dx
            }
        }
        else{  //caso 1.3 orizzontale
            if (map[x taxi+1][y taxi]!=hole){
                x++; //vai a destra
            }
            else{ 
                x++; //vai a destra
                y++; //vai in alto
            }
        }
    }
    else if(x taxi> x arrivo){ //caso 2
        if (y taxi< y arrivo){ //caso 2.1
            if (map[x taxi-1][y taxi +1]!=hole){
                y++; //ti sposti in alto
                x--; //ti sposti a sx
            }
            else{
                x--; //ti sposti a sx
            }
        }
        else if(y taxi> y arrivo){ //caso 2.2
            if(map[x taxi-1][y taxi-1]){
                x--; //ti sposti a sx
                y--; //ti sposti in basso
            }
            else{
                y--; //ti sposti in basso
            }
        }
        else{ //caso 2.3 orizzontale
            if(map[x taxi][y taxi-1]!=hole){
                y--; //ti sposti in basso
            }
            else{
                y--; //ti sposti in basso
                x--; //ti sposti a sx
            }
        }
    }
    else{ //caso 3
        if (y taxi< y arrivo){ //caso 3.1
            if(map[x taxi][y taxi+1]){
                y++; //ti muovi in alto
            }
            else{
                x++; //ti muovi a dx
                y++; //ti muovi in alto
            }
        }
        else if(y taxi> y arrivo){ //caso 3.2
            if(map[x taxi][y taxi-1]){
                y--; //ti muovi in basso
            }
            else{
                x--; //ti muovi a sx
                y--; //ti muovi in basso
            }
        }
    }

}

int distance(int x1, int x2, int y1, int y2){
    int a= (x1-x2)+(y1-y2);
    if (a<0){return a;}
    else {return -a;}
}