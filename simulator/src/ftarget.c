/* ftarget.c
   19.07.15.AW	Created.
		File containing target movement functions.
*/

#include <stdio.h>
#include <string.h>
#include "math.h"
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

#include "types.h"
#include "extern.h"
#include "ftarget.h"
#include "random.h"

// Independent RNG for target path
unsigned long target_path_random() {
    // Simple LCG (linear congruential generator)
    TargetPathState = TargetPathState * 1103515245 + 12345;
    return (TargetPathState >> 16) & 0x7fff;
}

// Call this in your initialization/setup code (e.g., main or before simulation loop)
void init_target_path_rng() {
    if (FixedTargetPath) {
        TargetPathState = TargetPathSeed;
    } else {
        TargetPathState = (unsigned long)time(NULL);
    }
}

// Helper: uniform integer in [0, n-1] using target path RNG
int target_path_uniform(int n) {
    if (n <= 0) return 0;
    return target_path_random() % n;
}
// Helper: uniform float in [0, max) using target path RNG
float target_path_funiform(double max) {
    return (target_path_random() / 32767.0) * max;
}

/********** move_target **********/
/* parameters:	t	current timestep
   called by:   step_run(), fxn.c
   actions:
*/
int move_target(int t)
   {
#ifdef DEBUG
printf("---in move_target()---\n");
#endif

   if ( !strcmp(Target_path, "square") )
      {
      path_square(t);
      }
   else if ( !strcmp(Target_path, "square10") )
      {
      path_square10(t);
      }
   else if( !strcmp(Target_path, "diamond"))
      {
      path_diamond(t);
      }
   else if( !strcmp(Target_path, "square_flex"))
      {
      path_square_flex(t);
      }
   else if ( !strcmp(Target_path, "circle") )
      {
      path_circle(t);
      }
   else if ( !strcmp(Target_path, "random") )
      {
      path_random(t);
      }
   else if ( !strcmp(Target_path, "random_manhattan") )
      {
      path_random_manhattan(t);
      }
   else if ( !strcmp(Target_path, "west") )
      {
      path_west(t);
      }
   else if ( !strcmp(Target_path, "northeast") )
      {
      path_northeast(t);
      }
   else if ( !strcmp(Target_path, "northeast_accel") )
      {
      path_northeast_accel(t);
      }
   else if ( !strcmp(Target_path, "sharp") )
      {
      path_sharp(t);
      }
   else if ( !strcmp(Target_path, "random_steep") )
      {
      path_random_steep(t);
      }
   else if ( !strcmp(Target_path, "zigzag") )
      {
      path_zigzag(t);
      }
   else if ( !strcmp(Target_path, "stationary_biased") )
      {
      path_stationary_biased(t);
      }
   else if ( !strcmp(Target_path, "single_switch") )
      {
      path_single_switch(t);
      }
   else if ( !strcmp(Target_path, "periodic_switch") )
      {
      path_periodic_switch(t);
      }
   else if ( !strcmp(Target_path, "step") )
      {
      path_step(t);
      }
   else if ( !strcmp(Target_path, "scurve") )
      {
      path_scurve(t);
      }
   else if ( !strcmp(Target_path, "serpentine"))
      {
      path_serpentine(t);
      }
   else if ( !strcmp(Target_path, "sine") )
      {
      path_sine(t);
      }
   else if ( !strcmp(Target_path, "sine2") )
      {
      path_sine2(t);
      }
   else if ( !strcmp(Target_path, "figure_8") )
      {
      path_figure8(t);
      }
   else if ( !strcmp(Target_path, "sineNew") )
      {
      path_sineNew(t);
      }
   else if ( !strcmp(Target_path, "rose") )
      {
      path_rose(t);
      }
   else if ( !strcmp(Target_path, "arcThenSine") )
      {
      path_arcThenSine(t);
      }
   else if ( !strcmp(Target_path, "sineThenIncreasingSine") )
      {
      path_sineThenIncreasingSine(t);
      }
   else if ( !strcmp(Target_path, "twoParabolas") )
      {
      path_twoParabolas(t);
      }
   else
      {
      /* this code will never hit because it is checked in init_target */
      printf(" Error(move_target): invalid target path: %s\n", Target_path);
      return ERROR;
      }

#ifdef DEBUG
printf("---end move_target()---\n");
#endif

   return OK;
   }  /* move_target */

/********** path_square **********/
/* parameters:  t       current timestep
   called by:   move_target(), fxn.c
   actions:
*/
void path_square(int t)
   {
   char side[20];

#ifdef DEBUG
printf("---in path_square()---\n");
#endif

   // move one step
   if ( !strcmp(Target.side, "left") )
      {
      if (Target.y < 80)
         {
         Target.y = Target.y + Target.step_len;
         }
      else
         {
         sprintf(Target.side, "top");
         }
      }
   else if ( !strcmp(Target.side, "top") )
      {
      if (Target.x < 80)
         {
         Target.x = Target.x + Target.step_len;
         }
      else
         {
         sprintf(Target.side, "right");
         }
      }
   else if ( !strcmp(Target.side, "right") )
      {
      if (Target.y > 20)
         {
         Target.y = Target.y - Target.step_len;
         }
      else
         {
         sprintf(Target.side, "bottom");
         }
      }
   else if ( !strcmp(Target.side, "bottom") )
      {
      if (Target.x > 20)
         {
         Target.x = Target.x - Target.step_len;
         }
      else
         {
         sprintf(Target.side, "left");
         }
      }

   // update target length
   Target.length += Target.step_len;

#ifdef DEBUG
printf("---end path_square()---\n");
#endif
   }  /* path_square */

/********** path_square10 **********/
/* parameters:  t       current timestep
   called by:   move_target(), fxn.c
   actions:
*/
void path_square10(int t)
   {
   char side[20];

#ifdef DEBUG
printf("---in path_square10()---\n");
#endif

   // move one step
   if ( !strcmp(Target.side, "left") )
      {
      if (Target.y < 30)
         {
         Target.y = Target.y + Target.step_len;
         }
      else
         {
         sprintf(Target.side, "top");
         }
      }
   else if ( !strcmp(Target.side, "top") )
      {
      if (Target.x < 30)
         {
         Target.x = Target.x + Target.step_len;
         }
      else
         {
         sprintf(Target.side, "right");
         }
      }
   else if ( !strcmp(Target.side, "right") )
      {
      if (Target.y > 20)
         {
         Target.y = Target.y - Target.step_len;
         }
      else
         {
         sprintf(Target.side, "bottom");
         }
      }
   else if ( !strcmp(Target.side, "bottom") )
      {
      if (Target.x > 20)
         {
         Target.x = Target.x - Target.step_len;
         }
      else
         {
         sprintf(Target.side, "left");
         }
      }

   // update target length
   Target.length += Target.step_len;

#ifdef DEBUG
printf("---end path_square10()---\n");
#endif
   }  /* path_square10 */

/********** path_diamond **********/
/* parameters:  t       current timestep
   called by:   move_target(), fxn.c
   actions: Moves like a diamond.
*/
void path_diamond(int t)
   {
      int perimeter = (int)Edge_length*4;
      int period = perimeter/Target.step_len; // how many timesteps it takes to complete the shape
      int numTimestepsPerQuadrant = period/4;
      double temp = sqrt(2)*Edge_length/2, incrementVal = Target.step_len*sqrt(2)/2;
      if(t%period < numTimestepsPerQuadrant)
         {
         if(t%period == 0) 
            {
            Target.x = 0;
            Target.y = 0;
            }
         Target.x += incrementVal;
         Target.y = Target.x;
         }
      else if(t%period < (2*numTimestepsPerQuadrant) && t%period >= numTimestepsPerQuadrant)
         {
         Target.x += incrementVal;
         Target.y -= incrementVal;
         }
      else if(t%period < (3*numTimestepsPerQuadrant) && t%period >= (2*numTimestepsPerQuadrant))
         {
         Target.x -= incrementVal;
         Target.y -= incrementVal;
         }
      else if(t%period < 4*numTimestepsPerQuadrant && t%period >= 3*numTimestepsPerQuadrant)
         {
         Target.x -= incrementVal;
         Target.y = -1*(Target.x);
         }
   }

/********** path_square_flex **********/
/* parameters:  t       current timestep
   called by:   move_target(), fxn.c
   actions: Moves like a square but you can control the edge length.
*/
void path_square_flex(int t)
   {
      int perimeter = (int)Edge_length*4;
      int period = perimeter/Target.step_len; // how many timesteps it takes to complete the shape
      int numTimestepsPerQuadrant = period/4;
      double incrementVal = Target.step_len;
      // left
      if(t%period < numTimestepsPerQuadrant)
         {
         if(t%period == 0) 
            {
            Target.x = 0;
            Target.y = 0;
            }
         Target.y += incrementVal;
         }
      // top
      else if(t%period < (2*numTimestepsPerQuadrant) && t%period >= numTimestepsPerQuadrant)
         {
         Target.x += incrementVal;
         }
      // right
      else if(t%period < (3*numTimestepsPerQuadrant) && t%period >= (2*numTimestepsPerQuadrant))
         {
         Target.y -= incrementVal;
         }
      // bottom
      else if(t%period < 4*numTimestepsPerQuadrant && t%period >= 3*numTimestepsPerQuadrant)
         {
         Target.x -= incrementVal;
         }
      Target.length += Target.step_len;
   }


/********** path_circle **********/
/* parameters:  t       current timestep
   called by:   move_target(), fxn.c
   actions:     Move target in a circle.  Each step is of length
		Target.step_len, which is an angle of 2 * arcsin(step_len/2R).
		This angle is called theta0.

		The trig functions, e.g. cos(x), assume that x is in radians.

		r is the radius of the circle.
		x1, y1 is the starting coordinates.
		x2, y2 is the ending coordinates after moving one step of
		length Target.step_len along the circle.
*/
void path_circle(int t)
   {
   double x1, y1, x2, y2;
   double theta0;
   double r;

#ifdef DEBUG
printf("---in path_circle()---\n");
#endif

   x1 = Target.x;
   y1 = Target.y;
   r = Circle_radius;
   //r = 30;
   theta0 = 2 * asin(Target.step_len / (2 * r) );

/*
printf(" x1 %lf y1 %lf r %lf theta0 %lf %lf\n", x1, y1, r, theta0,
theta0/(2*3.14159)*360);
*/
    
   if (x1 >= 0 && y1 >= 0)
      {
      // NE quadrant
      Target.y = y2 = r * sin( asin(y1/r) - theta0 );
      Target.x = x2 = r * cos( acos(x1/r) - theta0 );
      }
   else if (x1 >= 0 && y1 < 0)
      {
      // SE quadrant
      Target.y = y2 = r * sin( asin(y1/r) - theta0 );
      Target.x = x2 = r * cos( asin(y1/r) - theta0 );
      }
   else if (x1 < 0 && y1 < 0)
      {
      // SW quadrant
      //Target.y = y2 = (-1) * r * sin( 2 * M_PI - asin(y1/r) + theta0 );
      //Target.x = x2 = (-1) * r * cos( 2 * M_PI - acos(x1/r) + theta0 );
      Target.y = y2 = (-1) * r * sin( acos((-1)*x1/r) - theta0 );
      Target.x = x2 = (-1) * r * cos( asin((-1)*y1/r) - theta0 );
      }
   else if (x1 < 0 && y1 >= 0)
      {
      Target.y = y2 = r * sin( asin(y1/r) + theta0 );
      Target.x = x2 = (-1) * r * cos( acos((-1)*x1/r) + theta0 );
      }

   // update target length
   Target.length += sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );

#ifdef DEBUG
printf("---end path_circle()---\n");
#endif
   }  /* path_circle */

/********** path_random **********/
/* parameters:  t       current timestep
   called by:   move_target(), fxn.c
   actions:     Move target randomly with Gaussian shift in angle in each step.
                Each step is of length Target.step_len.
*/
void path_random(int t)
   {
   double rand1, rand2, r, theta;		// box muller variables
   double gaussian_rand;
   double x1, y1, x2, y2;

#ifdef DEBUG
printf("---in path_random()---\n");
#endif
   x1 = Target.x;
   y1 = Target.y;

   // Use independent RNG for target path
   rand1 = target_path_random() / 32767.0;
   while(rand1 == 0.0)
      rand1 = target_path_random() / 32767.0;
   rand2 = target_path_random() / 32767.0;
   r = sqrt( -2 * log(rand1) );
   theta = 2 * M_PI * rand2;
   if ( t%2 == 0)  gaussian_rand = r * cos(theta);
   else            gaussian_rand = r * sin(theta);

   Target.angle += gaussian_rand;
   if (Target.angle > 2 * M_PI)  Target.angle = Target.angle - (2 * M_PI);
   else if (Target.angle < 0)    Target.angle = (2 * M_PI) + Target.angle;

   x2 = cos(Target.angle) * Target.step_len + Target.x;
   y2 = sin(Target.angle) * Target.step_len + Target.y;

   Target.x = x2;
   Target.y = y2;

   // update target length
   Target.length += sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );

#ifdef DEBUG
printf("---end path_random()---\n");
#endif
   }  /* path_random */

/********** path_random_manhattan **********/
/* parameters:  t       current timestep
   called by:   move_target(), fxn.c
   actions:	Move target randomly.  At each cell, randomly chooses one of
		the four possible neighbors.
		Each step is of length Target.step_len.
*/
void path_random_manhattan(int t)
   {
   char side[20];
   int rand_num;

#ifdef DEBUG
printf("---in path_random_manhattan()---\n");
#endif

   rand_num = target_path_uniform(4);

   if (rand_num == 0)
      {
      // move north
      Target.y += Target.step_len;
      }
   else if (rand_num == 1)
      {
      // move east
      Target.x += Target.step_len;
      }
   else if (rand_num == 2)
      {
      // move south
      Target.y -= Target.step_len;
      }
   else if (rand_num == 3)
      {
      // move west
      Target.x -= Target.step_len;
      }

   // update target length
   Target.length += Target.step_len;

#ifdef DEBUG
printf("---end path_random_manhattan()---\n");
#endif
   }  /* path_random_manhattan */

/********** path_west **********/
/* parameters:  t       current timestep
   called by:   move_target(), fxn.c
   actions:     Move target due west in each step.
                Each step is of length Target.step_len.
*/
void path_west(int t)
   {

#ifdef DEBUG
printf("---in path_west()---\n");
#endif

   Target.x += Target.step_len;

   // update target length
   Target.length += Target.step_len;

#ifdef DEBUG
printf("---end path_west()---\n");
#endif
   }  /* path_west */

/********** path_northeast **********/
/* parameters:  t       current timestep
   called by:   move_target(), fxn.c
   actions:     Move target northeast in each step.
                Each step is of length Target.step_len.
*/
void path_northeast(int t)
   {

#ifdef DEBUG
printf("---in path_northeast()---\n");
#endif

   Target.x += sqrt((Target.step_len * Target.step_len)/2.0);
   Target.y += sqrt((Target.step_len * Target.step_len)/2.0);
    
   // for testing different angles above horizontal for the northeast line
   // Target.x = Target.x + Target.step_len * cos(M_PI/6);
   // Target.y = Target.y + Target.step_len * sin(M_PI/6);



   // update target length
   Target.length += Target.step_len;

#ifdef DEBUG
printf("---end path_northeast()---\n");
#endif
   }  /* path_northeast */

/********** path_northeast_accel **********/
/* parameters:  t       current timestep
   called by:   move_target(), fxn.c
   actions:     Move target northeast in each step.
                Each step is of length Target.step_len.
*/
void path_step(int t){
   if (t<500){
        Target.y += Target.step_len;
        Target.length += Target.step_len;
   }
   else {
      Target.x += Target.step_len;
      Target.length += Target.step_len;
   }
  
}

/********** path_stationary_biased **********/
/* Stationary biased demand: task A (north) high for entire run; B,C,D low.
   Target drifts north each timestep — persistent directional bias for PI tests.
*/
void path_stationary_biased(int t)
   {
   (void)t;
   Target.y += Target.step_len;
   Target.length += Target.step_len;
   }

/********** path_single_switch **********/
/* One demand switch: phase 1 task A (north) high; phase 2 task B (east) high.
   First timestep of phase B is Demand_switch_step (default 500 → A for t=0..499, B for t>=500).
*/
void path_single_switch(int t)
   {
   if (t < Demand_switch_step)
      {
      Target.y += Target.step_len;
      }
   else
      {
      Target.x += Target.step_len;
      }
   Target.length += Target.step_len;
   }

/********** path_periodic_switch **********/
/* Rotating high demand A→B→C→D→A… each Demand_segment_len steps (default 200).
   A=north, B=east, C=south, D=west.
*/
void path_periodic_switch(int t)
   {
   int L = Demand_segment_len;
   if (L <= 0)
      L = 200;
   int phase = (t / L) % 4;
   if (phase == 0)
      Target.y += Target.step_len;
   else if (phase == 1)
      Target.x += Target.step_len;
   else if (phase == 2)
      Target.y -= Target.step_len;
   else
      Target.x -= Target.step_len;
   Target.length += Target.step_len;
   }
void path_northeast_accel(int t)
   {

#ifdef DEBUG
printf("---in path_northeast_accel()---\n");
#endif

   Target.change = Target.change * 0.9;

   Target.x += sqrt(((Target.step_len-Target.change) *
                     (Target.step_len-Target.change))/2.0);
   Target.y += sqrt(((Target.step_len-Target.change) *
                     (Target.step_len-Target.change))/2.0);

   // update target length
   Target.length += Target.step_len-Target.change;

#ifdef DEBUG
printf("---end path_northeast_accel()---\n");
#endif
   }  /* path_northeast_accel */

/********** path_sharp **********/
/* parameters:  t       current timestep
   called by:   move_target(), fxn.c
   actions:
*/
void path_sharp(int t)
   {
   double randnum;
   double x1, y1, x2, y2;

#ifdef DEBUG
printf("---in path_sharp()---\n");
#endif

   x1 = Target.x;
   y1 = Target.y;

   if (Target.angle >= 0 && Target.angle <= M_PI_2)  // NE quadrant
      {
      Target.x = x2 = Target.x + Target.step_len * sin(Target.angle);
      Target.y = y2 = Target.y + Target.step_len * cos(Target.angle);
      }
   else if (Target.angle >= 0 && Target.angle > M_PI_2)  // NW quadrant
      {
      Target.x = x2 = Target.x - Target.step_len * cos(Target.angle - M_PI_2);
      Target.y = y2 = Target.y + Target.step_len * sin(Target.angle - M_PI_2);
      }
   if (Target.angle < 0 && Target.angle <= M_PI_2)  // SE quadrant
      {
      Target.x = x2 = Target.x + Target.step_len * sin(Target.angle);
      Target.y = y2 = Target.y - Target.step_len * cos(Target.angle);
      }
   else if (Target.angle < 0 && Target.angle > M_PI_2)  // SW quadrant
      {
      Target.x = x2 = Target.x - Target.step_len * cos(Target.angle - M_PI_2);
      Target.y = y2 = Target.y - Target.step_len * sin(Target.angle - M_PI_2);
      }

   // update target length
   Target.length += sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );

   // should target change direction in next timestep?
   // if changing direction, also change the change_probability -- that way
   // the straight segments will vary in length
   randnum = target_path_random() / 32767.0;
   if (randnum < Target.change_probability)
      {
      // randomly pick a direction to move in
      Target.angle += target_path_funiform(2 * M_PI);

//      // randomly pick a change percent between 20 and 30 percent
//      Target.change_probability = knuth_random() * 0.1 + 0.2;
      // randomly pick a change percent between 5 and 10 percent
      Target.change_probability = (target_path_random() / 32767.0) * 0.05 + 0.05;
      }

#ifdef DEBUG
printf("---end path_sharp()---\n");
#endif
   }  /* path_sharp */

/********** path_random_steep **********/
/* parameters:  t       current timestep
                r       previous random
   called by:   move_target(), fxn.c
   actions:
*/
void path_random_steep(int t)
   {
   char side[20];

#ifdef DEBUG
printf("---in path_random_steep()---\n");
#endif

   int rand_num_decision = target_path_uniform(4);
   int rand_chance = target_path_uniform(101); 
   float directions[] = {0, 180.0, 270.0, 360.0};
   double x1, y1, x2, y2;

   x1 = Target.x;
   y1 = Target.y;
   
   if (t % 50 == 0 || t == 0 || rand_chance >= 90)
   {  
      Target.angle = directions[rand_num_decision];
      printf("CHANGING DIRECTION. Target Angle is %f\n", Target.angle);
   }
   
   if (Target.angle == 0)
   {  
      Target.x += sqrt((Target.step_len * Target.step_len)/2.0);
      Target.y += sqrt((Target.step_len * Target.step_len)/2.0);
   }
   else if (Target.angle == 180)
   {  
      Target.x -=  sqrt((Target.step_len * Target.step_len)/2.0);
      Target.y +=  sqrt((Target.step_len * Target.step_len)/2.0);
   }
   else if (Target.angle == 270)
   {  
      Target.x -= sqrt((Target.step_len * Target.step_len)/2.0);
      Target.y -= sqrt((Target.step_len * Target.step_len)/2.0);
   }
   else if (Target.angle == 360)
   {  
      Target.x +=  sqrt((Target.step_len * Target.step_len)/2.0);
      Target.y -= sqrt((Target.step_len * Target.step_len)/2.0);
   }

   x2 = Target.x;
   y2 = Target.y;
   
   // update target length
   Target.length += sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );

#ifdef DEBUG
printf("---end path_random_steep()---\n");
#endif
   }  /* path_random_steep */

/********** path_zigzag **********/
/* parameters:  t       current timestep
   called by:   move_target(), fxn.c
   actions:	Start at 0, 0.  Move up by Target.amplitude for Target.period/4,
		then move down by Target.amplitude * 2 for Target.period/2,
		then move up by Target.amplitude for Target.period/4.  Repeat.
*/
void path_zigzag(int t)
   {
#ifdef DEBUG
printf("---in path_zigzag()---\n");
#endif

   double x1, y1, x2, y2;
   double direction_multiplier;

   if ( (Target.y >=0 && Target.y >= Target.amplitude) || 
        (Target.y < 0 && Target.y <= -Target.amplitude) )
      {
      // change directions
      Target.direction = Target.direction * -1;
      }

   x1 = Target.x;
   y1 = Target.y;

   Target.x = x2 = Target.x + 
                   Target.step_len * sin(M_PI/2 - Target.angle);
   Target.y = y2 = Target.y + 
                   Target.step_len * 
                   (Target.direction * cos(M_PI/2 - Target.angle) ) ;

   // update target length
//   printf(" ---------- %lf --\n", sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) ) );
   Target.length += sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );


#ifdef DEBUG
printf("---end path_zigzag()---\n");
#endif
   }  /* path_zigzag */

/********** path_scurve **********/
/* parameters:  t       current timestep
   called by:   move_target(), fxn.c
   actions:     Start at 0, 0.  Move up by Target.amplitude for Target.period/4,
                then move down by Target.amplitude * 2 for Target.period/2,
                then move up by Target.amplitude for Target.period/4.  Repeat.
		Curved like s-curve instead of straight zigzag.
*/
void path_scurve(int t)
   {
#ifdef DEBUG
printf("---in path_scurve()---\n");
#endif

   double x1, y1, x2, y2;
   double direction_multiplier;

   if (Target.y >=0)
      {
      Target.direction = -1;
      }
   else if (Target.y < 0) 
      {
      Target.direction = 1;
      }

   x1 = Target.x;
   y1 = Target.y;

   Target.x = x2 = Target.x +
                   Target.step_len * sin(M_PI/2 - Target.angle);
   Target.y = y2 = Target.y +
                   Target.step_len *
                   (cos(M_PI/2 - Target.angle) );

   // update target length
   // printf(" ----------Dist = %lf --\n", sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) ) );
   Target.length += sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );

   // update target angle
   if (y1 >= 0 && y2 < 0)
      Target.angle = -M_PI/2;
   else if (y1 < 0 && y2 >= 0)
      Target.angle = M_PI/2;
   else
      Target.angle = Target.angle + (Target.direction * Target.change);
//printf("** %lf angle\n", Target.angle/M_PI*180);

#ifdef DEBUG//doing something right nboiwlsgnslrighnsgnsrgnngkswjvirbnvlsgfir4b
printf("---end path_scurve()---\n");
#endif
   }  /* path_scurve */

/********** path_serpentine **********/
/* parameters:  t       current timestep
   called by:   move_target(), fxn.c
*/
void path_serpentine(int t)
   {
   #ifdef DEBUG
   printf("---in path_serpentine()---\n");
   #endif

   double x1 = Target.x, y1 = Target.y, x2 = x1, y2;
   // calculating dx
   // double dx = Target.step_len/sqrt(1 + Target.amplitude*Target.amplitude*cos(x1)*cos(x1));
   
   int temp = 100, count = 0;
   double dx = 0.001;
   while(count++ <= temp)
      {
      x2 += dx;
      if(condition(Target.step_len, x2, x1) == 1)
         {
         break;
         }
      }

   if(count == 100) printf("exceeded my threshold");
   y2 = sin(x2);

   Target.x = x2; Target.y = y2;

   printf("\n----distance between target points = %lf--------\n", sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1)));

   #ifdef DEBUG
   printf("---end path_serpentine()---\n");
   #endif
   }  /* path_serpentine */

int condition(double dist, double x2, double x1)
   {
   if((dist*dist + 0.005) <= ((x2 - x1)*(x2 - x1) + (sin(x2) - sin(x1))*(sin(x2) - sin(x1))) || \
      ((x2 - x1)*(x2 - x1) + (sin(x2) - sin(x1))*(sin(x2) - sin(x1))) >= (dist*dist - 0.005))
      return 1;
   return 0;
   }

/********** path_sine **********/
/* parameters:  t       current timestep
   called by:   move_target(), fxn.c
   actions:
*/
void path_sine(int t)
   {
   double x1, y1, x2, y2;
   double amplitude = 20;
   double phase_shift = 0;
   double x_modifier = 0.2; // The period is 2pi / x_modifier

#ifdef DEBUG
printf("---in path_sine()---\n");
#endif

   x1 = Target.x;
   y1 = Target.y;

   // y = asin(bx + c)
   Target.y = y2 = amplitude*sin((x_modifier * Target.x) + phase_shift) + sqrt(Target.x)*5;
   Target.x = x2 = Target.x + Target.step_len;

//   printf(" ---------- %lf --\n", sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) ) );
   Target.length += sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );

#ifdef DEBUG
printf("---end path_sine()---\n");
#endif
   }  /* path_sine */

/********** path_sine2 **********/
/* parameters:  t       current timestep
   called by:   move_target(), fxn.c
   actions: 
*/
void path_sine2(int t)
   {
   double x1, y1, x2, y2;
   double c;		// constant to adjust so that basic stepsize is ~1

#ifdef DEBUG
printf("---in path_sine2()---\n");
#endif

   x1 = Target.x;
   y1 = Target.y;

   Target.x = x2 = Target.x + fabs( sin( (t + M_PI)/Target.period ) );
   Target.y = y2 = Target.step_len * sin( t/(Target.amplitude) );

   // update target length
//   printf(" %lf --------------------\n",
//         sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) ) );
   Target.length += sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );

#ifdef DEBUG
printf("---end path_sine2()---\n");
#endif
   }  /* path_sine2 */

/************ path_figure8 ***********/
/* parameters:  t       current timestep
   called by:   move_target(), fxn.c
   actions:     moves target along figure eight
                trajectory. Each step is of
                length Target.step_len
*/
void path_figure8(int t)
   {
   double x1, y1, x2, y2;

#ifdef DEBUG
printf("--in path_figure8()---\n");
#endif

   x1 = Target.x;
   y1 = Target.y;

   /* Gerono lemniscate given by the parametric
      equations: x = a*sin(t), y = a*sin(t)*cos(t)
   */
   int a = 5;
   Target.x = x2 = a * sin(t);
   Target.y = y2 = a * sin(t) * cos(t);

   Target.length += sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );

#ifdef DEBUG
printf("---end path_figure8()---\n");
#endif
   } /* path_figure8 */











/********** f_sineNew **********/
/* parameters:  x       a given x position
   called by:   path_sineNew() and g_sineNew, ftarget.c
   actions:	returns f(x) for the given path
*/
double f_sineNew(double x)
   {
   double amplitude = Target.amplitude;
   double period = Target.period;
   double innerMult = (M_PI*2) / period;
   return amplitude*sin(innerMult*x)+ sqrt(x);
   }  /* f_sineNew */

/********** g_sineNew **********/
/* parameters:  x	a given x position
		x1	the current x1 value in
			path_sineNew()
		y1	the current y1 value in
			path_sineNew()
   called by:   path_sineNew() and gder_sineNew, ftarget.c
   actions:	returns the distance between (x1, y1)
		and (x, f_sineNew(x)) offset by the 
		square of Target.step_len
*/
double g_sineNew(double x, double x1, double y1)
   {
   double step_len = Target.step_len;
   double y_diff = f_sineNew(x) - y1;
   double x_diff = x - x1;
   return ((x_diff * x_diff) + (y_diff * y_diff)) 
           - (step_len * step_len);
   }  /* g_sineNew */

/********** gder_sineNew **********/
/* parameters:  x       a given x position
		x1	the current x1 value in
			path_sineNew()
		y1	the current y1 value in
			path_sineNew()
   called by:   path_sineNew(), ftarget.c
   actions:	returns g'(x) for the given path
*/
double gder_sineNew(double x, double x1, double y1)
   {
   double h = 0.000001;   // numerical differentiation parameter
                          // 0.000001 has performed well in testing
                          // Smaller values may introduce machine error
   return (g_sineNew(x + h, x1, y1) - g_sineNew(x - h, x1, y1)) 
          / (2 * h);
   }  /* gder_sineNew */

/********** path_sineNew **********/
/* parameters:  t       current timestep
   called by:   move_target(), fxn.c
   actions:	moves target along sine wave
		trajectory. Each step is of
                approximate length Target.step_len.
		Uses generalized path method.
*/
void path_sineNew(int t)
   {
   double x1, y1, x2, y2;
   double tol;
   double newton_iter;

#ifdef DEBUG
printf("---in path_sineNew()---\n");
#endif

   tol = -0.01;   // The final value of g of the improved first guess must land
                  // between tol and zero.  By increasing
                  // the bound, less work will be done finding
                  // the first guess, but there is a greater
                  // potential for results to go awry in the 
                  // Newton's Method process

   newton_iter = 6;   // Around six Newton's method iterations
                      // have worked well.  More is overkill, too
                      // few has far less accuracy

   x1 = Target.x;
   y1 = Target.y;

   double guess = x1;   // The current but not final improved first guess

   double step = 1;    // The value the current first guess increases by if the
                       // guess is below the allowed range.  For maximum
                       // efficiency, this value can be changed to be around
                       // half of the distance between the initial first guess 
                       // and the x value computed at the end of this method

   double stepReduction = 0.5;  // The rate by which the value of step is reduced
                                // after g(guess) exceeds zero.  0.5 has worked well

   int guessInTol = 0; // Whether the first guess is currently in the allowed tolerance range

   double yGuess = g_sineNew(guess,x1,y1);  // The g function value of the first guess

   // Finds a better first guess than x1 using the above parameters
   // in a guess and check style method
   while (guessInTol == 0)
      {
      if (yGuess > tol && yGuess < 0)
         // The improved first guess is in the allowed tolerance range
         guessInTol = 1;
      else if (yGuess > 0)
         {
         // The improved first guess has overshot the tolerance range
         guess -= step;
         step = step * stepReduction;
         yGuess = g_sineNew(guess,x1,y1);
         }
      else
         {
         // The improved first guess is under the tolerance range
         guess = guess + step;
         yGuess = g_sineNew(guess,x1,y1);
         }
      }

   // The final improved first guess
   x2 = guess;

   // Apply Newton's method to g starting with the improved
   // first guess
   int i;
   for (i = 0; i < newton_iter; i++)
      {
      x2 = x2 - (g_sineNew(x2,x1,y1) / gder_sineNew(x2,x1,y1));
      }

    // Update the tracker position using the approximation
    // from Newton's method
    y2 = f_sineNew(x2); 

    Target.x = x2;
    Target.y = y2;


   // The step made is very close to Target.step_len, but not exact
   // Add distance between old and new points for exact length
   Target.length += sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );

#ifdef DEBUG
printf("---end path_sineNew()---\n");
#endif
   }  /* path_sineNew */







/********** xt_rose **********/
/* parameters:  x       a given x position
   called by:   path_sineNew(), ftarget.c
   actions:	returns f(x) for the given path
*/
double xt_rose(double t)
   {
   return 30*cos(2*t)*cos(t);
   }  /* xt_rose */

/********** yt_rose **********/
/* parameters:  x       a given x position
   called by:   path_sineNew(), ftarget.c
   actions:	returns f(x) for the given path
*/
double yt_rose(double t)
   {
   return 30*cos(2*t)*sin(t);
   }  /* yt_rose */

/********** g_rose **********/
/* parameters:  x	a given x position
		x1	the current x1 value in
			path_sinNew()
		y1	the current y1 value in
			path_sinNew()
   called by:   path_sineNew(), ftarget.c
   actions:	returns the distance between (x1, y1)
		and (x, f_sinNew(x)) offset by the 
		square of Target.step_len
*/
double g_rose(double t, double x1, double y1)
   {
   double steplen;
   steplen = Target.step_len;
   double first = yt_rose(t)-y1;
   double second = xt_rose(t) - x1;
   return ((first*first)+(second*second))-(steplen*steplen);
   }  /* g_rose */

/********** gder_rose **********/
/* parameters:  x       a given x position
   called by:   path_sineNew(), ftarget.c
   actions:	returns g'(x) for the given path
*/
double gder_rose(double x, double x1, double y1)
   {
   double h;
   h = 0.000001;	// a constant
   return (g_rose(x+h,x1,y1) - g_rose(x-h,x1,y1)) / (2*h);
   }  /* f_sineNew */

/********** path_sineNew **********/
/* parameters:  t       current timestep
   called by:   move_target(), fxn.c
   actions:	moves target along sine wave
		trajectory. Each step is of
                approximate length Target.step_len.
		Uses generalized path method.
*/
void path_rose(int t) // note: The path is functionally correct, but the documentation is not
   {
   double x1, y1, x2, y2, t1, t2;
   double tol;
   double newtonIterations;

   tol = -0.01;
   newtonIterations = 6;

#ifdef DEBUG
printf("---in path_sineNew()---\n");
#endif

   x1 = Target.x;
   y1 = Target.y;
   t1 = Target.t;

   // Finding a better first guess than t1

   double guess;
   guess = t1;

   double step;
   step = 1;

   double stepReduction;
   stepReduction = 0.5;

   int guessInTol;
   guessInTol = 0;

   double gGuess;
   gGuess = g_rose(guess,x1,y1);

   while (guessInTol == 0)		
      {
      if (gGuess > tol && gGuess < 0)
         guessInTol = 1;
      else if (gGuess > 0)
         {
         guess -= step;
         step = step * stepReduction;
         gGuess = g_rose(guess,x1,y1);
         printf("here %f\n",guess);
         }
      else
         guess = guess + step;
         gGuess = g_rose(guess,x1,y1);
         printf("there %f\n",guess);
      }

   t2 = guess;
   printf("%f\n",t2);

   //Apply Newton's

   int i;
   for (i = 0; i < newtonIterations; i++)
      {
      t2 = t2 - (g_rose(t2,x1,y1) / gder_rose(t2,x1,y1));
      }
   //printf("%f\n",x2);

    x2 = xt_rose(t2);
    y2 = yt_rose(t2); 

    Target.x = x2;
    Target.y = y2;
    Target.t = t2;


   // The step made is very close to Target.step_len, but not exact
   // Add distance between old and new points for exact length
   Target.length += sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );

#ifdef DEBUG
printf("---end path_sineNew()---\n");
#endif
   }  /* path_rose */








/********** xt_arcThenSine **********/
/* parameters:  x       a given x position
   called by:   path_sineNew(), ftarget.c
   actions:	returns f(x) for the given path
*/
double xt_arcThenSine(double t, int stage)
   {
   if (stage == 0)
   	return 60*sin(t);
   else
        return t;
   }  /* xt_arcThenSine */

/********** yt_arcThenSine **********/
/* parameters:  x       a given x position
   called by:   path_sineNew(), ftarget.c
   actions:	returns f(x) for the given path
*/
double yt_arcThenSine(double t, int stage)
   {
   if (stage == 0)
   	return 60*cos(t);
   else {
        //printf("stage1\n");
        return 30*sin(0.25*t);
   }
   }  /* yt_arcThenSine */

/********** g_arcThenSine **********/
/* parameters:  x	a given x position
		x1	the current x1 value in
			path_sinNew()
		y1	the current y1 value in
			path_sinNew()
   called by:   path_sineNew(), ftarget.c
   actions:	returns the distance between (x1, y1)
		and (x, f_sinNew(x)) offset by the 
		square of Target.step_len
*/
double g_arcThenSine(double t, double x1, double y1, int stage)
   {
   double steplen;
   steplen = Target.step_len;
   double first = yt_arcThenSine(t,stage)-y1;
   double second = xt_arcThenSine(t,stage) - x1;
   return ((first*first)+(second*second))-(steplen*steplen);
   }  /* g_arcThenSine */

/********** gder_arcThenSine **********/
/* parameters:  x       a given x position
   called by:   path_sineNew(), ftarget.c
   actions:	returns g'(x) for the given path
*/
double gder_arcThenSine(double x, double x1, double y1, int stage)
   {
   double h;
   h = 0.000001;	// a constant
   return (g_arcThenSine(x+h,x1,y1,stage) - g_arcThenSine(x-h,x1,y1,stage)) / (2*h);
   }  /* f_sineNew */

/********** path_sineNew **********/
/* parameters:  t       current timestep
   called by:   move_target(), fxn.c
   actions:	moves target along sine wave
		trajectory. Each step is of
                approximate length Target.step_len.
		Uses generalized path method.
*/
void path_arcThenSine(int t) // note: The path is functionally correct, but the documentation is not
   {
   double x1, y1, x2, y2, t1, t2, xOff, yOff;
   double tol;
   double newtonIterations;

   tol = -0.01;
   newtonIterations = 6;

#ifdef DEBUG
printf("---in path_sineNew()---\n");
#endif

   x1 = Target.x;
   y1 = Target.y;
   t1 = Target.t;
   xOff = Target.xOff;
   yOff = Target.yOff;

   int enterX = xOff;
   int enterY = yOff;

   //new
   int stage;
   if (t == 100)
      {
      stage = 1;
      xOff = x1;
      yOff = y1;
      Target.xOff = x1;
      Target.yOff = y1;
      t1 = 0;
      x1 = 0;
      y1 = 0;
      }
   else if (t > 100)
      {
      stage = 1;
      x1 -= xOff;
      y1 -= yOff;
      }
   else
      stage = 0;

   // Finding a better first guess than t1

   double guess;
   guess = t1;

   double step;
   step = 1;

   double stepReduction;
   stepReduction = 0.5;

   int guessInTol;
   guessInTol = 0;

   double gGuess;
   gGuess = g_arcThenSine(guess,x1,y1,stage);

   while (guessInTol == 0)		
      {
      if (gGuess > tol && gGuess < 0)
         guessInTol = 1;
      else if (gGuess > 0)
         {
         guess -= step;
         step = step * stepReduction;
         gGuess = g_arcThenSine(guess,x1,y1,stage);
         //printf("here %f\n",guess);
         //printf("%d %d\n", enterX, enterY);
         }
      else
         guess = guess + step;
         gGuess = g_arcThenSine(guess,x1,y1,stage);
         //printf("there %f\n",guess);
      }

   t2 = guess;
   printf("%f\n",t2);

   //Apply Newton's

   int i;
   for (i = 0; i < newtonIterations; i++)
      {
      t2 = t2 - (g_arcThenSine(t2,x1,y1,stage) / gder_arcThenSine(t2,x1,y1,stage));
      }
   //printf("%f\n",x2);

    x2 = xt_arcThenSine(t2,stage);
    y2 = yt_arcThenSine(t2,stage); 

    Target.x = x2 + xOff;
    Target.y = y2 + yOff;
    Target.t = t2;


   // The step made is very close to Target.step_len, but not exact
   // Add distance between old and new points for exact length
   Target.length += sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );

#ifdef DEBUG
printf("---end path_sineNew()---\n");
#endif
   }  /* path_arcThenSine */






/********** xt_sineThenIncreasingSine **********/
/* parameters:  x       a given x position
   called by:   path_sineNew(), ftarget.c
   actions:	returns f(x) for the given path
*/
double xt_sineThenIncreasingSine(double t, int stage)
   {
   if (stage == 0)
   	return t;
   else
        return t;
   }  /* xt_sineThenIncreasingSine */

/********** yt_sineThenIncreasingSine **********/
/* parameters:  x       a given x position
   called by:   path_sineNew(), ftarget.c
   actions:	returns f(x) for the given path
*/
double yt_sineThenIncreasingSine(double t, int stage)
   {
   if (stage == 0)
   	return 30*sin(0.25*t);
   else {
        return 15*t*sin(0.25*t);
   }
   }  /* yt_sineThenIncreasingSine */

/********** g_sineThenIncreasingSine **********/
/* parameters:  x	a given x position
		x1	the current x1 value in
			path_sinNew()
		y1	the current y1 value in
			path_sinNew()
   called by:   path_sineNew(), ftarget.c
   actions:	returns the distance between (x1, y1)
		and (x, f_sinNew(x)) offset by the 
		square of Target.step_len
*/
double g_sineThenIncreasingSine(double t, double x1, double y1, int stage)
   {
   double steplen;
   steplen = Target.step_len;
   double first = yt_sineThenIncreasingSine(t,stage)-y1;
   double second = xt_sineThenIncreasingSine(t,stage) - x1;
   return ((first*first)+(second*second))-(steplen*steplen);
   }  /* g_sineThenIncreasingSine */

/********** gder_sineThenIncreasingSine **********/
/* parameters:  x       a given x position
   called by:   path_sineNew(), ftarget.c
   actions:	returns g'(x) for the given path
*/
double gder_sineThenIncreasingSine(double x, double x1, double y1, int stage)
   {
   double h;
   h = 0.000001;	// a constant
   return (g_sineThenIncreasingSine(x+h,x1,y1,stage) - g_sineThenIncreasingSine(x-h,x1,y1,stage)) / (2*h);
   }  /* f_sineNew */

/********** path_sineNew **********/
/* parameters:  t       current timestep
   called by:   move_target(), fxn.c
   actions:	moves target along sine wave
		trajectory. Each step is of
                approximate length Target.step_len.
		Uses generalized path method.
*/
void path_sineThenIncreasingSine(int t) // note: The path is functionally correct, but the documentation is not
   {
   double x1, y1, x2, y2, t1, t2, xOff, yOff;
   double tol;
   double newtonIterations;

   tol = -0.01;
   newtonIterations = 6;

#ifdef DEBUG
printf("---in path_sineNew()---\n");
#endif

   x1 = Target.x;
   y1 = Target.y;
   t1 = Target.t;
   xOff = Target.xOff;
   yOff = Target.yOff;

   int enterX = xOff;
   int enterY = yOff;

   //new
   int stage;
   if (t == 100)
      {
      stage = 1;
      xOff = x1;
      yOff = y1;
      Target.xOff = x1;
      Target.yOff = y1;
      t1 = 0;
      x1 = 0;
      y1 = 0;
      }
   else if (t > 100)
      {
      stage = 1;
      x1 -= xOff;
      y1 -= yOff;
      }
   else
      stage = 0;

   // Finding a better first guess than t1

   double guess;
   guess = t1;

   double step;
   step = 1;

   double stepReduction;
   stepReduction = 0.5;

   int guessInTol;
   guessInTol = 0;

   double gGuess;
   gGuess = g_sineThenIncreasingSine(guess,x1,y1,stage);

   while (guessInTol == 0)		
      {
      if (gGuess > tol && gGuess < 0)
         guessInTol = 1;
      else if (gGuess > 0)
         {
         guess -= step;
         step = step * stepReduction;
         gGuess = g_sineThenIncreasingSine(guess,x1,y1,stage);
         //printf("here %f\n",guess);
         }
      else
         guess = guess + step;
         gGuess = g_sineThenIncreasingSine(guess,x1,y1,stage);
         //printf("there %f\n",guess);
      }

   t2 = guess;
   printf("%f\n",t2);

   //Apply Newton's

   int i;
   for (i = 0; i < newtonIterations; i++)
      {
      t2 = t2 - (g_sineThenIncreasingSine(t2,x1,y1,stage) / gder_sineThenIncreasingSine(t2,x1,y1,stage));
      }
   //printf("%f\n",x2);

    x2 = xt_sineThenIncreasingSine(t2,stage);
    y2 = yt_sineThenIncreasingSine(t2,stage); 

    Target.x = x2 + xOff;
    Target.y = y2 + yOff;
    Target.t = t2;


   // The step made is very close to Target.step_len, but not exact
   // Add distance between old and new points for exact length
   Target.length += sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );

#ifdef DEBUG
printf("---end path_sineNew()---\n");
#endif
   }  /* path_sineThenIncreasingSine */




/********** xt_twoParabolas **********/
/* parameters:  x       a given x position
   called by:   path_sineNew(), ftarget.c
   actions:	returns f(x) for the given path
*/
double xt_twoParabolas(double t, int stage)
   {
   if (stage == 0)
   	return t;
   else
        return t;
   }  /* xt_twoParabolas */

/********** yt_twoParabolas **********/
/* parameters:  x       a given x position
   called by:   path_sineNew(), ftarget.c
   actions:	returns f(x) for the given path
*/
double yt_twoParabolas(double t, int stage)
   {
   if (stage == 0)
   	return 0.25*t*t;
   else {
        return 0.25*-t*t;
   }
   }  /* yt_twoParabolas */

/********** g_twoParabolas **********/
/* parameters:  x	a given x position
		x1	the current x1 value in
			path_sinNew()
		y1	the current y1 value in
			path_sinNew()
   called by:   path_sineNew(), ftarget.c
   actions:	returns the distance between (x1, y1)
		and (x, f_sinNew(x)) offset by the 
		square of Target.step_len
*/
double g_twoParabolas(double t, double x1, double y1, int stage)
   {
   double steplen;
   steplen = Target.step_len;
   double first = yt_twoParabolas(t,stage)-y1;
   double second = xt_twoParabolas(t,stage) - x1;
   return ((first*first)+(second*second))-(steplen*steplen);
   }  /* g_twoParabolas */

/********** gder_twoParabolas **********/
/* parameters:  x       a given x position
   called by:   path_sineNew(), ftarget.c
   actions:	returns g'(x) for the given path
*/
double gder_twoParabolas(double x, double x1, double y1, int stage)
   {
   double h;
   h = 0.000001;	// a constant
   return (g_twoParabolas(x+h,x1,y1,stage) - g_twoParabolas(x-h,x1,y1,stage)) / (2*h);
   }  /* f_sineNew */

/********** path_sineNew **********/
/* parameters:  t       current timestep
   called by:   move_target(), fxn.c
   actions:	moves target along sine wave
		trajectory. Each step is of
                approximate length Target.step_len.
		Uses generalized path method.
*/
void path_twoParabolas(int t) // note: The path is functionally correct, but the documentation is not
   {
   double x1, y1, x2, y2, t1, t2, xOff, yOff;
   double tol;
   double newtonIterations;

   tol = -0.01;
   newtonIterations = 6;

#ifdef DEBUG
printf("---in path_sineNew()---\n");
#endif

   x1 = Target.x;
   y1 = Target.y;
   t1 = Target.t;
   xOff = Target.xOff;
   yOff = Target.yOff;

   int enterX = xOff;
   int enterY = yOff;

   //new
   int stage;
   if (t == 100)
      {
      stage = 1;
      xOff = x1;
      yOff = y1;
      Target.xOff = x1;
      Target.yOff = y1;
      t1 = 0;
      x1 = 0;
      y1 = 0;
      }
   else if (t > 100)
      {
      stage = 1;
      x1 -= xOff;
      y1 -= yOff;
      }
   else
      stage = 0;

   // Finding a better first guess than t1

   double guess;
   guess = t1;

   double step;
   step = 1;

   double stepReduction;
   stepReduction = 0.5;

   int guessInTol;
   guessInTol = 0;

   double gGuess;
   gGuess = g_twoParabolas(guess,x1,y1,stage);

   while (guessInTol == 0)		
      {
      if (gGuess > tol && gGuess < 0)
         guessInTol = 1;
      else if (gGuess > 0)
         {
         guess -= step;
         step = step * stepReduction;
         gGuess = g_twoParabolas(guess,x1,y1,stage);
         //printf("here %f\n",guess);
         }
      else
         guess = guess + step;
         gGuess = g_twoParabolas(guess,x1,y1,stage);
         //printf("there %f\n",guess);
      }

   t2 = guess;
   printf("%f\n",t2);

   //Apply Newton's

   int i;
   for (i = 0; i < newtonIterations; i++)
      {
      t2 = t2 - (g_twoParabolas(t2,x1,y1,stage) / gder_twoParabolas(t2,x1,y1,stage));
      }
   //printf("%f\n",x2);

    x2 = xt_twoParabolas(t2,stage);
    y2 = yt_twoParabolas(t2,stage); 

    Target.x = x2 + xOff;
    Target.y = y2 + yOff;
    Target.t = t2;


   // The step made is very close to Target.step_len, but not exact
   // Add distance between old and new points for exact length
   Target.length += sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );

#ifdef DEBUG
printf("---end path_sineNew()---\n");
#endif
   }  /* path_twoParabolas */
