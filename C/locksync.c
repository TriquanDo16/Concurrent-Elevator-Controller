#include"elevator.h"
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<pthread.h>

pthread_mutex_t passenger_lock;
pthread_mutex_t elevator_lock;
int elevator_floor=0;
int elevator_direction=1;

void scheduler_init() {
    pthread_mutex_init(&passenger_lock,0);
    pthread_mutex_init(&elevator_lock,0);
    pthread_mutex_lock(&elevator_lock);
}

void passenger_request(int passenger, int from_floor, int to_floor, void (*enter)(int, int), void(*exit)(int, int)) {
    pthread_mutex_lock(&passenger_lock);
    while(1) {
        pthread_mutex_lock(&elevator_lock);
        if(elevator_floor == from_floor) {
            enter(passenger,0); 
            pthread_mutex_unlock(&elevator_lock);
            break;
        } 
        pthread_mutex_unlock(&elevator_lock);
    }
    while(1) {
        pthread_mutex_lock(&elevator_lock);
        if(elevator_floor == to_floor) { 
            exit(passenger,0);
            pthread_mutex_unlock(&elevator_lock);
            break;
        }
        pthread_mutex_unlock(&elevator_lock);
    }

    pthread_mutex_unlock(&passenger_lock);
}

void elevator_ready(int elevator, int at_floor, void(*move_direction)(int, int), void(*door_open)(int), void(*door_close)(int)) {
    if(elevator == 0) {
        if(at_floor == FLOORS-1)
            elevator_direction = -1;
        if(at_floor == 0)  
            elevator_direction = 1;
            
        door_open(elevator);
        pthread_mutex_unlock(&elevator_lock);

        usleep(1); // give another thread a chance to grab the lock

        pthread_mutex_lock(&elevator_lock);
        door_close(elevator);          
          
        move_direction(elevator,elevator_direction);
        elevator_floor = at_floor+elevator_direction;
    }
}
