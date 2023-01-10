#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


#define MAX_CHILDREN 5

typedef double MathFunc_t(double);


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

static int numChildren = 0;


//Integrate using the trapezoid method. 
double integrateTrap(MathFunc_t* func, double rangeStart, double rangeEnd, size_t numSteps)
{
	double rangeSize = rangeEnd - rangeStart;
	double dx = rangeSize / numSteps;

	double area = 0;
	for (size_t i = 0; i < numSteps; i++) {
		double smallx = rangeStart + i*dx;
		double bigx = rangeStart + (i+1)*dx;

		area += dx * ( func(smallx) + func(bigx) ) / 2; //Would be more efficient to multiply area by dx once at the end. 
	}

	return area;
}


bool getValidInput(double* start, double* end, size_t* numSteps, size_t* funcId)
{
	printf("Query: [start] [end] [numSteps] [funcId]\n");

	//Read input numbers and place them in the given addresses:
	size_t numRead = scanf("%lf %lf %zu %zu", start, end, numSteps, funcId);

	//Return whether the given range is valid:
	return (numRead == 4 && *end >= *start && *numSteps > 0 && *funcId < NUM_FUNCS);
}


void sig_handler(int signum) 
{
    numChildren --;  /* child process is dead so decrement the number of children */
}  


void spawn_child(void)
{
    double rangeStart;
	double rangeEnd;
    size_t numSteps;
    size_t funcId;
    pid_t child_pid = 0;
    
    if (!getValidInput(&rangeStart, &rangeEnd, &numSteps, &funcId)) { /* exit the process if input not valid */
        while (numChildren > 0) {  
            /* waits for all children to finish processing before exiting the process */
            wait(NULL);
        }
        exit(0);
    }

    signal(SIGCHLD, sig_handler); /* registering the signal handler for when child process exits */
    child_pid = fork();
    numChildren ++; /* started child process so increment the number of children */
    
    if (child_pid < 0) {  /* check if there were errors */
        perror("fork");
        exit(EXIT_FAILURE);
    }

    else if (child_pid == 0) {  /* Child code */
        double area = integrateTrap(FUNCS[funcId], rangeStart, rangeEnd, numSteps);
        printf("The integral of function %zu in range %g to %g is %.10g\n", funcId, rangeStart, rangeEnd, area);
        exit(0); /* exit child process and send to parent */
    }

    else {  /* Parent code */
        while (numChildren >= MAX_CHILDREN) { /* waits until a child is available if the max number has been reached */
            wait(NULL);
        }
        spawn_child(); /* recursive call to ask for more input and spawn another child */
    }
}


int main(void)
{
    spawn_child(); /* enter the recursive function to spawn children */
	exit(0);
}