/*
 * Author: Tri Quan Do _ tdo22@uic.edu
 * University of Illinois at Chicago, Spring 2022
 * CS 361     : Professor Jacob Eriksson
 * Homework 7 : A concurrent elevator controller
 */

#include "elevator.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

/*******************************************
 * BELOW IS FOR 25 SECOND BOUNDARIES
 ******************************************/

// /* Threading simulates elevator movement */
// pthread_mutex_t passenger_lock;
// pthread_mutex_t elevator_lock;

// pthread_cond_t elevator_signal = PTHREAD_COND_INITIALIZER;
// pthread_cond_t passenger_signal = PTHREAD_COND_INITIALIZER;

// int elevator_floor = 0;
// int elevator_direction = 1;

// /* Global scope for checking valid readiablitly */
// int is_elevator_ready = 0;
// int is_passenger_ready = 0;

// int wait_at = -1; /* Waiting time for passenger, elevator */


/*******************************************
 * CONSTRUCTING < 25 SECOND BOUNDARIES
 ******************************************/
static struct Elevator {
	// int floor;	
	// int open;
	int num_passengers;  // time efficiency
	// int trips;

	/* Threading simulates elevator movement */
	pthread_mutex_t passenger_lock;
	pthread_mutex_t elevator_lock;

	pthread_cond_t elevator_signal;
	pthread_cond_t passenger_signal;

	int elevator_floor;
	int elevator_direction;

	/* Global scope for checking valid readiablitly */
	int is_elevator_ready;
	int is_passenger_ready;
	int wait_at;
	
} elevators[ELEVATORS];


/*******************************************
 * CONSTRUCTING < 9 SECOND BOUNDARIES
 ******************************************/
static struct Passenger {
	int standon_what_elevator;
    int from_floor;
    int to_floor;

    int check_inside_elevator;  // To check if passenger is inside or not!
    int check_finish_trip;

} Passengers[PASSENGERS];


void scheduler_init() {
    /* These lines were running for 25s
        pthread_mutex_init(&passenger_lock,0);
        pthread_mutex_init(&elevator_lock,0);
        pthread_mutex_lock(&elevator_lock);
    */

    for (int i = 0; i < ELEVATORS; i++) {
        // elevators[i].floor = 0;
        // elevators[i].open = 0;
        // elevators[i].passenger = 0;
        // elevators[i].trips = 0;

        pthread_mutex_init(&elevators[i].passenger_lock, 0);
        pthread_mutex_init(&elevators[i].elevator_lock, 0);
        pthread_mutex_lock(&elevators[i].elevator_lock);

        elevators[i].elevator_floor = 0;
        elevators[i].elevator_direction = 1;
        elevators[i].is_elevator_ready = 0;
        elevators[i].is_passenger_ready = 0;
        elevators[i].wait_at= -1;
        elevators[i].num_passengers = 0;
        

        // elevators[i].elevator_signal = PTHREAD_COND_INITIALIZER;
	    // elevators[i].passenger_signal = PTHREAD_COND_INITIALIZER;

        pthread_cond_init(&elevators[i].elevator_signal, NULL);
        pthread_cond_init(&elevators[i].passenger_signal, NULL);
    }

    for (int i = 0; i < PASSENGERS; i++) {
        // Passengers[i].standon_what_elevator = i % ELEVATORS;
        Passengers[i].standon_what_elevator = 0;
        Passengers[i].from_floor = 0;
        Passengers[i].to_floor = 0;
        Passengers[i].check_inside_elevator = 0;
        Passengers[i].check_finish_trip = 0;
    }
}

void passenger_request(int passenger, int from_floor, int to_floor, void (*enter)(int, int), void(*exit)(int, int)) {

    int passengerClassified = passenger % ELEVATORS;

    Passengers[passenger].standon_what_elevator = passenger % ELEVATORS;
    Passengers[passenger].from_floor = from_floor;
    Passengers[passenger].to_floor = to_floor;

    // pthread_mutex_lock(&passenger_lock);
    // pthread_mutex_lock(&elevator_lock);

    pthread_mutex_lock(&elevators[passengerClassified].passenger_lock);
    pthread_mutex_lock(&elevators[passengerClassified].elevator_lock);

    // wait_at = from_floor;
    elevators[passengerClassified].wait_at = from_floor;

    /* TODO: Adding a check ele is ready or from_floor is not equal to elevator floor and is_elevator_full
            !(is_elevator_ready && from_floor = elevator_floor && num_passengers < CAPACITY)
            otherwise, passengers go to sleep !!
     */
    // while(!elevators[passengerClassified].is_elevator_ready) {
    while(!((elevators[passengerClassified].is_elevator_ready) && (from_floor == elevators[passengerClassified].elevator_floor) && (elevators[passengerClassified].num_passengers < MAX_CAPACITY)  )) {
        pthread_cond_wait(&elevators[passengerClassified].elevator_signal, &elevators[passengerClassified].elevator_lock);
    }
    /* If it is not full, adding the passenger to the elevator --> not need to add new code but will consider*/
    elevators[passengerClassified].is_elevator_ready = 0;
    
    // enter(passenger, 0);  -> purposely for single elevators 
    enter(passenger, passengerClassified);
    Passengers[passenger].check_inside_elevator = 1;
    elevators[passengerClassified].num_passengers += 1;
    elevators[passengerClassified].is_passenger_ready = 1;

    pthread_cond_signal(&elevators[passengerClassified].passenger_signal);
    elevators[passengerClassified].wait_at = to_floor;
    /* !(is_elevator_ready && elevator_floor == to_floor) */
    // while(!elevators[passengerClassified].is_elevator_ready) {
    while (!(elevators[passengerClassified].is_elevator_ready && elevators[passengerClassified].elevator_floor == to_floor)) {
        pthread_cond_wait(&elevators[passengerClassified].elevator_signal, &elevators[passengerClassified].elevator_lock);
    }
    elevators[passengerClassified].is_elevator_ready = 0;

    // exit(passenger, 0);  -> purposely for single elevators 
    exit(passenger, passengerClassified);
    elevators[passengerClassified].num_passengers -= 1;
    elevators[passengerClassified].is_passenger_ready = 1;
    Passengers[passenger].check_inside_elevator = 0;
    Passengers[passenger].check_finish_trip = 1;

    // The first lock should be the last one to be unlock
    pthread_cond_signal(&elevators[passengerClassified].passenger_signal);
    pthread_mutex_unlock(&elevators[passengerClassified].elevator_lock);
    pthread_mutex_unlock(&elevators[passengerClassified].passenger_lock);
}



void elevator_ready(int elevator, int at_floor, void(*move_direction)(int, int), void(*door_open)(int), void(*door_close)(int)) {
    /*
        Loop over the whole passenger to check if 2 things:
        1. can anyone get off the floor (check_in_elevator == 1 for entering) and (= 0 when exit)
        2. prere: check_in_elvator == 1 if the passenger can get off
        accessing index for passenger -> loop from 0 to Passenger_Capa
    */

    if(at_floor == FLOORS-1)
        elevators[elevator].elevator_direction = -1;
    else if(at_floor == 0)  
        elevators[elevator].elevator_direction = 1;
    else if (at_floor > elevators[elevator].wait_at)
        elevators[elevator].elevator_direction = -1;
    else 
        elevators[elevator].elevator_direction = 1;

    if (elevators[elevator].wait_at == at_floor && (elevators[elevator].num_passengers < MAX_CAPACITY)) {
        /* The loop for passengers struct will go there !! */
        door_open(elevator);
        elevators[elevator].wait_at = -1;
        elevators[elevator].is_elevator_ready = 1; // consider this line !!

        // May loop on this scoop !!
        for (int i = 0; i < PASSENGERS; i++) {
            int can_Off = (elevators[elevator].is_elevator_ready) && (Passengers[i].check_inside_elevator) && !(Passengers[i].check_finish_trip);
            int can_On  = (elevators[elevator].is_elevator_ready) && !(Passengers[i].check_inside_elevator) && (elevators[elevator].num_passengers < MAX_CAPACITY);

            if (!Passengers[i].check_finish_trip && (at_floor == Passengers[i].from_floor))
                log(9, "Check passenger %d on floor %d-th : can-on: %d - can off: %d\n", i, at_floor, can_On, can_Off);
            
            if (can_Off || can_On) {
                // elevators[elevator].is_elevator_ready = 1;
                pthread_cond_signal(&elevators[elevator].elevator_signal);
                
                log(9, "Check capacity:  %d -> %d\n", elevator, elevators[elevator].num_passengers);
                
                while (!elevators[elevator].is_passenger_ready) {
                    pthread_cond_wait(&elevators[elevator].passenger_signal, &elevators[elevator].elevator_lock);
                    // elevators[elevator].is_elevator_ready = 1; // should change the position if necessary
                }

                elevators[elevator].is_passenger_ready = 0;
            }

            // elevators[elevator].is_passenger_ready = 0;
            // while (!elevators[elevator].is_passenger_ready) {
            //     pthread_cond_wait(&elevators[elevator].passenger_signal, &elevators[elevator].elevator_lock);
            // }
        
            
            // elevators[elevator].is_passenger_ready = 0;
        }
        door_close(elevator);

    } else {
        pthread_mutex_unlock(&elevators[elevator].elevator_lock);
        usleep(1); // give another thread a chance to grab the lock
        pthread_mutex_lock(&elevators[elevator].elevator_lock);
    }
                
    move_direction(elevator, elevators[elevator].elevator_direction);
    elevators[elevator].elevator_floor = at_floor + elevators[elevator].elevator_direction;
}
