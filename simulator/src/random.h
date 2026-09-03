/* random.h
   191017.ASW   Rewrote the functions that used to be in this file
                (the code from Terry Jones) so that the function calls are
                all still available, but the code uses the basic srandom()
                and random() functions from stdlib.h.
*/

long seed_random(long seed);
double knuth_random();
int uniform(int n);
double funiform(double x);
double xfuniform(double x);
double box_muller(double mu, double st_dev);
double random_double(double a, double b);              // LR; 2020.02.02
