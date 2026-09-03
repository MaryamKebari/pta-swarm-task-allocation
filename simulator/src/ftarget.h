/* ftarget.h
   19.07.15.AW	Created.
*/

/* prototypes */
int move_target(int t);
void path_square(int t);
void path_square10(int t);
void path_circle(int t);
void path_random_manhattan(int t);
void path_random(int t);
void path_west(int t);
void path_northeast(int t);
void path_northeast_accel(int t);
void path_sharp(int t);
void path_random_steep(int t);
void path_zigzag(int t);
void path_stationary_biased(int t);
void path_single_switch(int t);
void path_periodic_switch(int t);
void path_step(int t);
void path_scurve(int t);
void path_serpentine(int t);
void path_sine(int t);
void path_sine2(int t);
void path_sineNew(int t);
void path_step(int t);
void path_rose(int t);
void path_arcThenSine(int t);
void path_sineThenIncreasingSine(int t);
void path_twoParabolas(int t);
void path_figure8(int t);
void path_diamond(int t);
void path_square_flex(int t);
int condition(double dist, double x2, double x1);
void init_target_path_rng();
int target_path_uniform(int n);
float target_path_funiform(double max);
unsigned long target_path_random();