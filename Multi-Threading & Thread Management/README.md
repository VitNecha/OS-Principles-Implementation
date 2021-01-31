# Car-Washing Station: Multi-Threading & Thread Management Implementation

About: 

Simulation of car-washing station('s) made for purposes of implementing Multi-Threading, Thread Management, Shared Memory and Signals 
in C.

User gives the number of washing stations and the programms generates randomly the arriving of cars to the stations and their washing 
times within the station.

Generally, after user enters the number of wished washing stations the system begin to generate cars (processes)
with fork() call. Each car is entering the washing function where it enters the queue and then checks if there is an
available washing station: 

- If there is, the car enters the washing action with prints and sleep (washing simulation). By the end of the
		action (car exiting the washing station) the car sends a signal to the next car that waiting in queue (FIFO).
		
- If there isn't, the car waits withing the queue to signal (of continue) to arrive.

Code explanation:

	- Two signal handlers.
	
		- First for internal use of a process: to "wake up" process waiting in the queue.
		
		- Second catches and handles user's input of CTRL+Z (according to the task).

	- Number of shared memory segments including:
	
		- System data information: Number of cars in the queue, number of cars in washing stations, etc.

	- Timer that activated when system starts to run.

	- NextTime function to give random times for different purposes of the system (according to the task).

	- Number generator that gives rates for NextTime function.

	- Semaphore implementation (with functions UP, DOWN and init).

	- "Washing" function implements washing process including queue implementation.
	
	Run: gcc CarWash.c -lm
