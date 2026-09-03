/* random.c
   191017.ASW	Rewrote the functions that used to be in this file
		(the code from Terry Jones) so that the function calls are
		all still available, but the code uses the basic srandom()
		and random() functions from stdlib.h.
*/

#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "random.h"
#include "types.h"

#define MBIGVAL 1000000

/* knuth_random returns a random number between 0 and 1
   with one millionth precision */
// return a float in the range
// [0,.....,1]
double knuth_random()
   {
   long r;
   double x;

   r = random();
   x = (double)(r % MBIGVAL) / (double)MBIGVAL;

   return(x);
   }

long seed_random(long seed)
   {
   srandom((unsigned)seed);
   return seed;
   }

/* uniform(n) returns a random integer value in the
   range of 0, ..., n-1 */
int uniform(int n)
   {
   if (n <= 0) return 0;
   return((int)(knuth_random() * (double)n));
   }  /* uniform */

// return a float in the range
// [0,.....,X]
double funiform(double x)
   {
   return(knuth_random() * x);
   }  /* funiform */

// return a float in the range
// [-X,....,0,.....,+X]
double xfuniform(double x)
   {
   return( (knuth_random() * (double)2.00 * x) - x );
   }  /* xfuniform */


// return a random from normal distribution with mean mu
// and standard deviation st_dev
double box_muller(double mu, double st_dev)
{
    double rand1, rand2;
    double r, theta;

    do {
        rand1 = knuth_random();
    } while (rand1 <= 0.0);
    rand2 = knuth_random();
    r = sqrt( -2 * log(rand1) );
    theta = 2 * M_PI * rand2;
    rand1 = (r * cos(theta)) * st_dev + mu;
    rand2 = (r * sin(theta)) * st_dev + mu;

    // Box Muller transform creates 2 independent randoms
    // Here we randomly choose which to return
    if(knuth_random() < 0.5)
        return rand1;
    else
        return rand2;
}

/* LR; 2020.02.01;
/********** random_double **********/
/* paramters: a - lower bound; b - upper bound
  called by:   init_agent(), fxn.c
  actions:	return a double in a range of two doubles
*/
double random_double(double a, double b) {
   double random_variable = ((double) random()) / (double) RAND_MAX;
   double diff = b - a;
   double r = random_variable * diff;
   return a + r;
}

