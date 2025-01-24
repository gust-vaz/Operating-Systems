#include "ep2.h"

// Variables that will be constant during the execution
int k, d, INTERVAL = 60000;

// Variables shared between all the threads
Cyclist_info* dataC;
int **pista;

// Variables that will be updated during the execution
_Atomic int cycs_at_CurrLap = 0;
_Atomic int waiting = 0;
_Atomic int remaining;
int currTime = 0;
int currLap = 1;
int n = 1;

// Booleans to control executions
int farAhead = 0;   // signals if there is a cyclist far ahead
int canRide = 1;    // signals if there are 2 or more cyclists in the track

// Cyclists that arrive at the finish line in 2*n lap
_Atomic int nPossWinn = 0;
int possibleWinners[10];

// List of the rank in order and its current index 
int indR = 0;
int *Rank;

// Semaphores
pthread_mutex_t pistaMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t barrier;

void newLap(int id){
    // Update the lap of the cyclist
    dataC[id].lastCrossTime = currTime;
    dataC[id].lap++;

    // A broken guy
    if(dataC[id].lap % 6 == 0 && ((double) rand()/RAND_MAX) < 0.15){
        pista[FINISH_LINE][dataC[id].j] = -1;
        dataC[id].status = BROKEN;
        remaining--;
    }

    // A possible winner
    if(dataC[id].lap == 2*n)
        possibleWinners[nPossWinn++] = id;
    // The guy in front is far ahead
    else if(dataC[id].lap == (currLap+2) && dataC[id].status == RUNNING)
        farAhead = 1;
    
    if(dataC[id].lap == currLap)
        cycs_at_CurrLap++;

    // Update the speed of the cyclist
    if(((double) rand()/RAND_MAX) < dataC[id].probability){
        if(dataC[id].speed == 60){
            dataC[id].speed = 30;
            dataC[id].probability = 0.7;
        }
        else{
            dataC[id].speed = 60;
            dataC[id].probability = 0.5;
        }
    }
        
}
void movingInTrack(int id){
    int x_new = (dataC[id].i+1) % d;
    int y_new = 0;
    int x_old = dataC[id].i;
    int y_old = dataC[id].j;

    // Check the first available position
    while(y_new < 10 && pista[x_new][y_new] != -1)
        y_new++;
    
    // It's impossible to ultrapass
    if(y_new == 10)
        return;

    // Moving
    pista[x_new][y_new] = id;
    pista[x_old][y_old] = -1;
    dataC[id].i = x_new;
    dataC[id].j = y_new;

    // Shift the others "down"
    for(y_old++; y_old < 10 && pista[x_old][y_old] != -1; y_old++){
        pista[x_old][y_old-1] = pista[x_old][y_old];
        pista[x_old][y_old] = -1;
        dataC[pista[x_old][y_old-1]].j = y_old-1;
    }
}
void *Riding(void *arg){
    int id = *(int*) arg;
    int halfMeter = 0;

    while(canRide){
        if(dataC[id].status == RUNNING){
            // Move half step
            if(dataC[id].speed == 30 && !halfMeter)
                halfMeter = 1;
            // Complete his move
            else{
                if(halfMeter) halfMeter = 0;

                pthread_mutex_lock(&pistaMutex);
                movingInTrack(id);
                if(dataC[id].i == FINISH_LINE)
                    newLap(id);  
                pthread_mutex_unlock(&pistaMutex);
            }
        }
        waiting++;
        pthread_barrier_wait(&barrier);
        if(dataC[id].status == BROKEN){
            printf("\n[QUEBROU] Ciclista %d na volta %d\n", id+1, dataC[id].lap);
            dataC[id].status = FINISHED;
        }
    }
    waiting++;
    pthread_exit(NULL);
}

void initCyclists(){
    dataC = (Cyclist_info*)malloc(sizeof(Cyclist_info)*k);
    for(int i = 0; i < k; i++){
        dataC[i].speed = 30;
        dataC[i].probability = 0.7;
        dataC[i].lap = 0;
        dataC[i].id = i;
        dataC[i].status = RUNNING;
    }
}
void initTrack(){
    int index = 0, line = 1;

    // Allocate the track
    pista = (int**) malloc(sizeof(int*)*d);
    for(int i = 0; i < d; i++){
        pista[i] = (int*) malloc(sizeof(int)*10);
        for(int j = 0; j < 10; j++)
            pista[i][j] = -1;
    }

    // Shuffle the cyclists
    int *ids = (int*) malloc(sizeof(int)*k);
    for(int i = 0; i < k; i++)
        ids[i] = i;
    for(int i = k-1; i > 0; i--)
        swap(&ids[i], &ids[rand() % (i+1)]);

    // Distribute the last cyclists in the track
    for(int j = 0; index < k % 5; index++){
        dataC[ids[index]].i = line;
        dataC[ids[index]].j = j;
        pista[line][j++] = ids[index];
    }
    if(index > 0)
        line++;

    // Distribute the rest of the cyclists
    for(int i = 0; i < k / 5; i++){
        int j = 0;
        for(int cont = 0; cont < 5; cont++){
            dataC[ids[index]].i = line;
            dataC[ids[index]].j = j;
            pista[line][j++] = ids[index++];
        }
        line++;
    }
    free(ids);
}

void tieBreaker(){
    // Skip while nobody finishes the 2*n lap
    if(nPossWinn == 0)
        return;

    // Choose the winner
    int winner = rand() % nPossWinn;
    Rank[indR++] = possibleWinners[winner];
    if(dataC[possibleWinners[winner]].status == RUNNING)
        remaining--; 

    // Update the status of the winner
    dataC[possibleWinners[winner]].status = indR;

    // Remove the winner from the track
    pista[FINISH_LINE][dataC[possibleWinners[winner]].j] = -1;

    // Shift the others "down"
    for(int y = dataC[possibleWinners[winner]].j+1; y < 10 && pista[FINISH_LINE][y] != -1; y++){
        pista[FINISH_LINE][y-1] = pista[FINISH_LINE][y];
        pista[FINISH_LINE][y] = -1;
        dataC[pista[FINISH_LINE][y-1]].j = y-1;
    }

    nPossWinn = 0;
    n++;
}

void swap(int *a, int *b){
    int aux = *a;
    *a = *b;
    *b = aux;
}
int partition(int arr[], int start, int end){
    int pivot = arr[end];
    int i = start-1;
    for(int j = start; j < end; j++){
        if(dataC[arr[j]].lap > dataC[pivot].lap || 
          (dataC[arr[j]].lap == dataC[pivot].lap && dataC[arr[j]].i > dataC[pivot].i) ||
          (dataC[arr[j]].lap == dataC[pivot].lap && dataC[arr[j]].i == dataC[pivot].i && dataC[arr[j]].j < dataC[pivot].j)){
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i+1], &arr[end]);
    return i+1;
}
void quickSort(int arr[], int start, int end){
    if(start < end){
        int pivot = partition(arr, start, end);
        quickSort(arr, start, pivot-1);
        quickSort(arr, pivot+1, end);
    }
}

void Report(int debug){
    // Skip while not all cyclists finished the lap
    if(cycs_at_CurrLap < remaining && !farAhead)
        return;

    cycs_at_CurrLap = 0;
    // Others, except the far ahead and some others, will be latecomers
    if(farAhead){
        farAhead = 0;
        currLap += 2;
    }
    currLap++;

    if(debug)
        return;

    int endR = 0, endL = 0;
    int *currRank = (int*) malloc(sizeof(int)*remaining);
    int *latecomers = (int*)malloc(sizeof(int)*remaining);
    for(int i = 0; i < remaining; i++)
        currRank[i] = latecomers[i] = -1;

    // Sort the cyclists in the track
    for(int i = 0; i < k; i++){
        if(dataC[i].status == RUNNING && dataC[i].lap >= currLap-1)
            currRank[endR++] = i;
        else if(dataC[i].status == RUNNING)
            latecomers[endL++] = i;
    }
    quickSort(currRank, 0, endR-1);
    
    printf("\nVolta atual: %d\n", currLap);
    for(int i = 0; i < endR; i++)
        printf("[%4d°] Ciclista %d na volta %d\n", i+1, currRank[i]+1, dataC[currRank[i]].lap); 
    for(int i = 0; i < endL; i++)
        printf("[ RET ] Ciclista %d na volta %d\n", latecomers[i]+1, dataC[latecomers[i]].lap);
    
    free(latecomers);
    free(currRank);
}
void printTrack(){
    printf("\n");
    for(int j = 9; j >= 0; j--){
        for(int i = d-1; i >= 0; i--){
            if(pista[i][j] != -1)
                printf("%-4d", pista[i][j]+1); // print not optimized for k > 999
            else
                printf(".   ");
        }
        printf("\n");
    }
}
void FinalReport(){
    printf("\n============= Relatório Final =============\n");
    printf("Ranking:\n");
    for(int i = 0; i < indR; i++)
        printf("[ %d° ] Ciclista %-4d (%dms)\n", i+1, Rank[i]+1, dataC[Rank[i]].lastCrossTime);
    printf("Quebraram:\n");
    for(int i = 0; i < k; i++)
        if(dataC[i].status < -1)
            printf("Ciclista %-4d na volta [%-4d]\n", i+1, dataC[i].lap);
}

int main(int argc, char *argv[]){
    d = atoi(argv[1]);
    k = atoi(argv[2]);
    remaining = k;
    int debug = argc > 3;

    // Initialize the cyclists and the track
    pthread_barrier_init(&barrier, NULL, k+1);
    Rank = (int*) malloc(sizeof(int)*k);
    pthread_t ciclista[k];
    initCyclists();
    initTrack();
    if(debug) printTrack();
    for(int i = 0; i < k; i++)
        pthread_create(&ciclista[i], NULL, Riding, &dataC[i].id);

    while(canRide){
        // Proceed one step in a competitive way and advancing the clock
        usleep(INTERVAL); // comment this line to execute long tests
        currTime += 60;
        while(waiting < k);
        waiting = 0;

        // Print the information according to the mode
        tieBreaker();
        if(debug) printTrack();
        Report(debug);

        // Allow the cyclists to proceed
        if(remaining == 1) canRide = 0;
        pthread_barrier_wait(&barrier);
    }

    // Put the last cyclist in the rank
    for(int i = 0; i < k; i++)
        if(dataC[i].status == RUNNING)
            Rank[indR++] = i;
    // Obs.: his lastCrossTime don't correspond to the real time he could officially finish

    while(waiting < k);
    FinalReport();

    // Free the memory and finish the program
    for(int i = 0; i < k; i++)
        pthread_join(ciclista[i], NULL);
    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&pistaMutex);
    for(int i = 0; i < d; i++)
        free(pista[i]);
    free(pista);
    free(dataC);
    free(Rank);

    return 0;
}