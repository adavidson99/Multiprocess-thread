#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>

#define NUM_THREADS 16

typedef double MathFunc_t(double);


typedef struct {  /* struct containing the arguments each thread has access to */
    double *total;  /* the final result of the intergration */
    double rangeStart;
	double rangeEnd;
    size_t i;
	size_t numSteps;
	size_t funcId;

    pthread_mutex_t *lock;
    pthread_t thread;

} Worker;


double gaussian(double x)
{
	return exp(-(x*x)/2) / (sqrt(2 * M_PI));
}


double chargeDecay(double x)
{
	if (x < 0) {
		return 0;
	} else if (x < 1) {
		return 1 - exp(-5*x);
	} else {
		return exp(-(x-1));
	}
}

#define NUM_FUNCS 3
static MathFunc_t* const FUNCS[NUM_FUNCS] = {&sin, &gaussian, &chargeDecay};


//Integrate using the trapezoid method. 
void *integrateTrap(void *ptr)
{
    Worker *worker = (Worker*)ptr;
    MathFunc_t *func = FUNCS[worker->funcId]; /* */

	double rangeSize = worker->rangeEnd - worker->rangeStart;
	double dx = rangeSize / worker->numSteps;
    double area = 0;

	for (size_t i = worker->i; i < worker->numSteps; i+=NUM_THREADS) {  /* modified so worker i increments by the number of threads, ensuring each thread does different work */
		double smallx = worker->rangeStart + i*dx;
		double bigx = worker->rangeStart + (i+1)*dx;

		area += dx * ( func(smallx) + func(bigx) ) / 2; //Would be more efficient to multiply area by dx once at the end. 
	}

    pthread_mutex_lock(worker->lock);  /* lock so that the total area is updated properly and clashes dont happen between threads */
    (*worker->total) += area;
    pthread_mutex_unlock(worker->lock); 
    
    return NULL;
}


bool getValidInput(double* start, double* end, size_t* numSteps, size_t* funcId)
{
	printf("Query: [start] [end] [numSteps] [funcId]\n");

	//Read input numbers and place them in the given addresses:
	size_t numRead = scanf("%lf %lf %zu %zu", start, end, numSteps, funcId);

	//Return whether the given range is valid:
	return (numRead == 4 && *end >= *start && *numSteps > 0 && *funcId < NUM_FUNCS);
}


int main(void)
{
	double rangeStart;
	double rangeEnd;
	size_t numSteps;
	size_t funcId;

	while (getValidInput(&rangeStart, &rangeEnd, &numSteps, &funcId)) {

        pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
        Worker workers[NUM_THREADS];

        double total = 0;

        for (int i = 0; i < NUM_THREADS; i++) {
            Worker *worker = &workers[i];
            worker->total = &total;  /* Pass the global total into each thread */
            worker->i = i;  /* the worker index to determine which range of values the thread will take */
            worker->rangeStart = rangeStart;
            worker->rangeEnd = rangeEnd;
            worker->numSteps = numSteps;
            worker->funcId = funcId;
            worker->lock = &lock;

            pthread_create(&worker->thread, NULL, integrateTrap, worker);
        }

        for(int j = 0; j < NUM_THREADS; ++j) {
		    pthread_join(workers[j].thread, NULL);
	    }

		printf("The integral of function %zu in range %g to %g is %.10g\n", funcId, rangeStart, rangeEnd, total);
	}

	exit(0);
}