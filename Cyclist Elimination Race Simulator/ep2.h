#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define FINISH_LINE 0

// status of the cyclists
#define RUNNING -1
#define BROKEN -2
#define FINISHED -3

typedef struct Cyclist_info{
    int id;             // cyclist id
    int speed;          // 30 km/h or 60 km/h
    double probability; // probability to change the speed
    int i, j;           // position in the track
    int lap;            // current lap of this cyclist
    int status;         // indicates if he's running, broken or the rank
    int lastCrossTime;  // time of the last cross in the finish line
} Cyclist_info;

// Fuctions to move the cyclists and update all his status
void newLap(int);
void movingInTrack(int);
void *Riding(void*);

// QuickSort functions to sort the cyclists
void swap(int*, int*);
int partition(int[], int, int);
void quickSort(int[], int, int);

// Functions to print the output of the program
void Report(int);
void printTrack();
void FinalReport();

// Functions to initialize the cyclists and the track
void initCyclists();
void initTrack();

// Function to decide who is the winner of the lap and eventually break ties
void tieBreaker();