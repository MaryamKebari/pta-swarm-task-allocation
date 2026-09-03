/* fxn.c
   14.10.15.AW	Created.
   4.10.19.JG Updated distributions.
*/

#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "types.h"
#include "extern.h"
#include "fxn.h"
#include "random.h"
#include "output.h"
#include "ftarget.h"
#include "ftracker.h"

//#define DEBUG 1
//#define DEBUG_AGENT 1

#define LAMBDA_P 5
#define LAMBDA_E 5
#define POISSON_K_RANGE 20
#define EXP_X_RANGE 5
#define LOG_K_RANGE 20
#define LOG_P .50

#define HISTOGRAM_SIZE ((Intensity_aging_max - Intensity_aging_min) * 10)
//#define HISTOGRAM_SIZE 10

FILE *Thresh_fp;
FILE *Intensity_fp;

/*************** factorial *****************/
/* parameters: k
   called by: init_agent(), fxn.c
   actions: factorial for poisson calculation
*/
int factorial(double k)
   {
   int n = (int)k;
   return (n == 0) ? 1 : n * factorial(n - 1);
   }

/********** init_fxn **********/
/* paramters:
   called by:	run_sim(), sim.c
   actions:
*/
int init_fxn()
   {
   int error;
   int i;

#ifdef DEBUG
printf("---in init_fxn()---\n");
#endif

   if (Num_tasks <= 0)  Num_tasks = 4;
   if (Num_tasks > MAX_TASKS)
      {
      printf(" Error(init_fxn): Num_tasks %d exceeds MAX_TASKS %d\n",
             Num_tasks, MAX_TASKS);
      return ERROR;
      }
   if (Task_opposition_mode < 0 || Task_opposition_mode > 1)
      {
      printf(" Error(init_fxn): Task_opposition_mode must be 0 or 1, got %d\n",
             Task_opposition_mode);
      return ERROR;
      }
   if (Task_opposition_mode == 1 &&
       !strncmp(Task_demand_pattern, "legacy_vector", 13) &&
       (Num_tasks % 2))
      {
      printf(" Error(init_fxn): paired-opposite vector mode requires even Num_tasks, got %d\n",
             Num_tasks);
      return ERROR;
      }
   init_task_metadata();

   /* initialize number of agents still alive */
   Num_alive = Pop_size;

   /* allocate space */
   Agent = (AGENT *)malloc(Pop_size * sizeof(AGENT) );
   if (Agent == NULL)
      {
      printf(" Error(init_fxn): cannot allocate space: Agent\n");
      return ERROR;
      }   /* if */

   if(Intensity_aging)
      {
         int histogram_size = (Intensity_aging_max - Intensity_aging_min) * 10;
         Tracker.intensity_north_dist = malloc(HISTOGRAM_SIZE * sizeof(int));
         Tracker.intensity_south_dist = malloc(HISTOGRAM_SIZE * sizeof(int));
         Tracker.intensity_east_dist = malloc(HISTOGRAM_SIZE * sizeof(int));
         Tracker.intensity_west_dist = malloc(HISTOGRAM_SIZE * sizeof(int));
      }

   /* initialize target */
   error = init_target();
   if (error == ERROR)  return ERROR;

   /* initialize agents */
   // if thresh == 10.0
   if (Thresh_init == 10.0){
      Thresh_fp = fopen("thresholds.txt", "r");
      if(Thresh_fp == NULL)
         {
         printf( "Error(init_fxn): cannot open thresholds.txt\n");
         return ERROR;
         }
   }
   // if inten == 10.0
   if (Intensity_variation == 10) {
      Intensity_fp = fopen("intensities.txt", "r");
      if(Intensity_fp == NULL)
         {
         printf( "Error(init_fxn): cannot open intensities.txt\n");
         return ERROR;
         }
   }

   for (i=0; i<Pop_size; i++)
      {
      error = init_agent(i);
      if (error == ERROR)  return ERROR;
      }  /* for */

   if (Thresh_init == 10.0){
      fclose(Thresh_fp);
   }
   if (Intensity_variation == 10) {
      fclose(Intensity_fp);
   }

   if (Print_params)  fprint_pop(stdout);

   /* HDM; moved initialize tracker to after initialize agents; 2019.09.12 */
   /* initialize tracker */
   error = init_tracker();
   if (error == ERROR)  return ERROR;

   /* print anything that needs to be printed */
   start_output();

#ifdef DEBUG
printf("---end init_fxn()---\n");
#endif
   return OK;
   }  /* init_fxn */

/********** end_fxn **********/
/* paramters:
   called by:	run_sim(), sim.c
   actions:
*/
int end_fxn()
   {
   int r;
#ifdef DEBUG
printf("---in end_fxn()---\n");
#endif

   free(Agent);

#ifdef DEBUG
printf("---end end_fxn()---\n");
#endif
   return OK;
   }  /* end_fxn */

/********** run_fxn **********/
/* paramters:
   called by:   run_sim(), sim.c
   actions:
*/
int run_fxn()
   {
   int t;
   int error;
#ifdef DEBUG
printf("---in run_fxn()---\n");
#endif

   for (t=0; t<Max_steps; t++)
      {
      if (Print_step)
         {
         printf(" T %3d target_vector_norm %lf tracker_vector_norm %lf residual_norm %lf",
             t, Target.length, Tracker.length, Tracker.difference);
         printf(" service_step_norm %lf\n", Tracker.current_step_len);
         }

      error = step_run(t);
      if (error == ERROR)  return ERROR;

      step_output(t);
      }  /* for h */

#ifdef DEBUG
printf("---end run_fxn()---\n");
#endif
   return OK;
   }  /* run_fxn */

/********** step_run **********/
/* paramters:   t       which timestep
   called by:   run_fxn(), fxn.c
   actions:
*/
int step_run(int t)
   {
   int error;
   int i, j;
   double rand, temp_prob_check;

#ifdef DEBUG
printf("---in step_run()---\n");
#endif
// fprintf(stderr, "before move_target\n");
   // move target
   error = move_target(t);
   if (error == ERROR)  return ERROR;
// fprintf(stderr, "after move_target\n");

   // move tracker
   error = move_tracker(t);
   if (error == ERROR)  return ERROR;
// fprintf(stderr, "after move_tracker\n");

   /* Multitask mode: the tracker is a task-space service vector, not a
      2D object. Difference is the norm of unmet task demand. */
   update_task_vector_stats();
   update_run_metrics(t);
   Tracker.total_pre_service_difference += Tracker.pre_service_difference;
   if (Tracker.pre_service_difference > Tracker.max_pre_service_difference)
      Tracker.max_pre_service_difference = Tracker.pre_service_difference;
   if (Tracker.pre_service_difference < Tracker.min_pre_service_difference)
      Tracker.min_pre_service_difference = Tracker.pre_service_difference;
   // sum so that average can be calculated
   Tracker.total_difference += Tracker.difference;
   // keep track of max and min differences
   if (Tracker.difference > Tracker.max_difference)
      Tracker.max_difference = Tracker.difference;
   if (Tracker.difference < Tracker.min_difference)
      Tracker.min_difference = Tracker.difference;

#ifdef DEBUG
printf("---end step_run()---\n");
#endif
   return OK;
   }  /* step_run */

// HDM; 2019.09.19
/********** rand_agent_intensity **********/
/* parameters:  none
   called by:   init_agent(), fxn.c
   actions: calculate random intensity value using Box Muller
*/
double rand_agent_intensity()
    {
#ifdef DEBUG
printf("---in rand_agent_intensity()---\n");
#endif

    double rand1, rand2, r, theta;	// box muller variables
    double intensity = 0.0;         // return value
    double divisor = 1.0;           // 1/divisor is std deviation

    // NOTE: r * cos(theta) and r * sin(theta) give values
    // approximately -3.0 -- 3.0
    
    // midpt of range
    if(Intensity_distribution == 0)
    {
        intensity = (Intensity_aging_min + Intensity_aging_max)/2.0;
    }
    // Gaussian
    else if(Intensity_distribution == 1)
        {
        double range = Intensity_aging_max - Intensity_aging_min;       // size of the intensity range
        double st_dev = range/6.0;                                      // standard deviation of the distribution
        double mu = Intensity_aging_min + range/2.0;                    // mean of the distribution

        intensity = box_muller(mu, st_dev);
        }
    // uniform
    else if(Intensity_distribution == 2)
        {
        rand1 = funiform(Intensity_aging_max - Intensity_aging_min);
        intensity = rand1 + Intensity_aging_min;
        }

#ifdef DEBUG
printf("---end rand_agent_intensity()---\n");
#endif

    return intensity;

    }  /* rand_agent_intensity */


static void init_agent_pid_gains(int n);

/********** init_agent **********/
/* paramters:	n	agent number
   called by:	init_fxn(), fxn.c
   actions:
*/
int init_agent(int n)
   {
   int i;
   int j;
   int error;

#ifdef DEBUG
printf("---in init_agent()---\n");
#endif

   Agent[n].index = n;

   // Initialize agent thresholds and (if applicable) threshold ranges.
   // If dynamic thresholds are turned on, initialize the threshold range for
   // each agent; else initialize static threshold w/value within range.
   // Thresh_dynamic = 0   dynamic thresholds off, static thresholds
   // Thresh_dynamic = 1   dynamic thresholds on w/range [0.0,1.0]
   // Thresh_dynamic = 2   dynamic thresholds on, range min/max uniformly 
   //                      distributed within bottom/top half of [0.0,1.0]
   // Thresh_dynamic = 3   dynamic thresholds on, range within two uniformly
   //                      random values within [0.0,1.0]
   if (Thresh_dynamic == 0)
      {
      // Static thresholds.  Thresholds expected to fall within [0.0,1.0].
      // initialize agent raw thresholds
      error = init_raw_thresholds(n);
      if (error == ERROR)  return ERROR;
      }
   else if (Thresh_dynamic == 1)
      {
      // Dynamic thresholds within the range [0.0,1.0].
      // initialize agent raw thresholds
      error = init_raw_thresholds(n);
      if (error == ERROR)  return ERROR;

      // Set range of [0.0,1.0] for all tasks
      Agent[n].raw_thresh_min_north = 0.0;
      Agent[n].raw_thresh_min_south = 0.0;
      Agent[n].raw_thresh_min_east = 0.0;
      Agent[n].raw_thresh_min_west = 0.0;
      Agent[n].raw_thresh_max_north = 1.0;
      Agent[n].raw_thresh_max_south = 1.0;
      Agent[n].raw_thresh_max_east = 1.0;
      Agent[n].raw_thresh_max_west = 1.0;
      }
   else if (Thresh_dynamic == 2)
      {
      /* Task-channel thresholds are initialized below in
         init_agent_task_channels(). Each task gets its own min/max range. */
      }
   else if (Thresh_dynamic == 3)
      {
      /* Task-channel thresholds are initialized below in
         init_agent_task_channels(). Each task gets its own min/max range. */
      }
   else
      {
      printf(" Error(init_agent):  Invalid value for Thresh_dynamic: %d\n",
		Thresh_dynamic);
      return ERROR;
      }

   /* scale thresholds from [0.0,1.0] to [0.0,Range] */
   scale_thresholds(n);
   error = init_agent_task_channels(n);
   if (error == ERROR)  return ERROR;

//rp  220521ASW replace with dynamic response probability
//rp  start of block to delete
   // set the response_prob value for each agent:
   // if Response_prob == 3, use normal distribution
   // if Response_prob == 2, each agent gets a random response_prob value
   // else if 0 <= Response_prob <= 1, set all agents' response_prob equal
   // to Response_prob
/*
   if (Response_prob == 3.0)
      {
      do 
         {
         Agent[n].response_prob = box_muller(RP_gaussian_mu, RP_gaussian_std);
         }
      while (Agent[n].response_prob > 1.0 || Agent[n].response_prob < 0.0);
      }
   else if (Response_prob == 2.0)
      {
      // initialize to random value 
      Agent[n].response_prob = knuth_random();
      } 
   else if (Response_prob <=1.0)
      {
      Agent[n].response_prob = Response_prob;
      }
   else
      {
      printf(" Error(init_agent): Invalid value for Response_prob: %lf\n",
             Response_prob);
      return ERROR;
      }
*/
//rp  end of block to delete

   // 220404.ASW
   // Initialize task specific response thresholds for each agent.
   // These values are independent of the Response_prob parameter and the
   // Agent[n].response_prob field.
   // Goal is to replace those fields with this one, but am adding this code 
   // first and then will delete that code.
   // Array elements: 0=idle, 1..Num_tasks=selectable task channels.
   // Task ids are abstract allocation channels, independent of tracker
   // physical directions.
   Agent[n].resprob[0] = -1;
   Agent[n].resprob_min[0] = -1;
   Agent[n].resprob_max[0] = -1;
   for (i=1; i<=Num_tasks; i++)
      {
      Agent[n].resprob_min[i] = Prob_dynamic_min;
      Agent[n].resprob_max[i] = Prob_dynamic_max;
      }
   if (Prob_dynamic_init >= 0.0 && Prob_dynamic_init <= 1.0)
      {
      for (i=1; i<=Num_tasks; i++)  Agent[n].resprob[i] = Prob_dynamic_init;
      }
   else if (Prob_dynamic_init == 2.0)
      {
      for (i=1; i<=Num_tasks; i++)
         {
         Agent[n].resprob[i] = knuth_random() * 
                   (Prob_dynamic_max - Prob_dynamic_min) + Prob_dynamic_min;
         }
      }
   else
      {
      printf(" Error(init_agent): Invalid value for Prob_dynamic_init: %lf\n", 
             Prob_dynamic_init);
      return ERROR;
      }

   // NB; 2020.06.23
   // set the spontaneous_response_prob value for each agent:
   // if Spontaneous_response_prob == 3, use normal distribution
   // if Spontaneous_response_prob == 2, each agent gets a random value
   // else if 0 <= Spontaneous_response_prob < 1, set all agents' equal to it
   // else if Spontaneous_response_prob == 1, set agent = 1-Agent[i].response_prob
   if (Spontaneous_response_prob == 3.0)
      {
         do {
            Agent[n].spontaneous_response_prob = box_muller(SRP_gaussian_mu, SRP_gaussian_std);
         }
         while (Agent[n].spontaneous_response_prob > 1.0 || Agent[n].spontaneous_response_prob < 0.0);
      }
   else if (Spontaneous_response_prob == 2.0)
      {
      /* initialize to random value */
      Agent[n].spontaneous_response_prob = knuth_random();
      }  /* if */
   else if (Spontaneous_response_prob == 1.0)
      {
//rp  220521ASW  the response_prob field is no longer used, replaced by
//rp  dynamic individual response probabilities for each task.
//rp  Old statement replaced with averag of all task RPs
//rp      Agent[n].spontaneous_response_prob = 1-Agent[n].response_prob;
      double avg_resprob = 0.0;
      for (i=1; i<=Num_tasks; i++)  avg_resprob += Agent[n].resprob[i];
      Agent[n].spontaneous_response_prob = 1 -
		(avg_resprob / (double)Num_tasks);
      }  /* else if */
   else if (Spontaneous_response_prob < 1.0)
      {
         Agent[n].spontaneous_response_prob = Spontaneous_response_prob;
      }  /* else if */
   else
      {
      printf(" Error(init_agent): Invalid value for Spontaneous_response_prob: %lf\n",
             Spontaneous_response_prob);
      return ERROR;
      }  /* else */

   // set the prob_check value for each agent:
   // if Prob_check == 0, each agent gets a random prob_check value
   // else if 0 < Prob_check <= 1, set all agents' prob_check to Prob_check
   if (Prob_check == 0.0)
      {
      /* initialize to random value */
      Agent[n].prob_check = knuth_random();
      }  /* if */
   else if (Prob_check <=1.0)
      {
      Agent[n].prob_check = Prob_check;
      }  /* else if */
   else
      {
      printf(" Error(init_agent): Invalid value for Prob_check: %lf\n",
             Prob_check);
      return ERROR;
      }  /* else */

   /* init agent activity to nothing */
   Agent[n].current_task = 0;
   Agent[n].previous_task = 0;
   Agent[n].previous_task_noidle = 0;

   /* init activity tracking to zero */
   Agent[n].count_north = 0;
   Agent[n].count_east = 0;
   Agent[n].count_south = 0;
   Agent[n].count_west = 0;
   Agent[n].count_idle = 0;
   for (i=0; i<=MAX_TASKS; i++)  Agent[n].count_task[i] = 0;
   Agent[n].count_switch = 0;
   Agent[n].count_switch_noidle = 0;
   Agent[n].count_switch_spontaneous = 0;

   Agent[n].count_multi_tasks = 0;
    
   // initialize agent intensities
   error = init_intensities(n);
   if (error == ERROR)  return ERROR;
   init_agent_task_intensities(n);
   init_agent_pid_gains(n);

   /* HDM; 2020.03.19 */
   Agent[n].dead = 0;
   Agent[n].time_killed = -1;

#ifdef DEBUG
printf("---end init_agent()---\n");
#endif
   return OK;
   }  /* init_agent */

void init_task_metadata()
   {
   int i;

   Task_demand[0] = 0.0;
   Task_prev_demand[0] = 0.0;
   Task_error_integral[0] = 0.0;
   Task_arrival[0] = 0.0;
   Task_total_arrival[0] = 0.0;
   Task_service[0] = 0.0;
   Task_total_service[0] = 0.0;
   Task_target_vector[0] = 0.0;
   Task_tracker_vector[0] = 0.0;
   Task_signed_error[0] = 0.0;
   Task_feedback_error[0] = 0.0;
   Task_prev_feedback_error[0] = 0.0;
   Task_actor_count[0] = 0;
   for (i=1; i<=MAX_TASKS; i++)
      {
      Task_demand[i] = 0.0;
      Task_prev_demand[i] = 0.0;
      Task_error_integral[i] = 0.0;
      Task_arrival[i] = 0.0;
      Task_total_arrival[i] = 0.0;
      Task_service[i] = 0.0;
      Task_total_service[i] = 0.0;
      Task_target_vector[i] = 0.0;
      Task_tracker_vector[i] = 0.0;
      Task_signed_error[i] = 0.0;
      Task_feedback_error[i] = 0.0;
      Task_prev_feedback_error[i] = 0.0;
      Task_actor_count[i] = 0;
      }
   }  /* init_task_metadata */

static double task_focus_weight(int task, double focus_task, double width)
   {
   double distance = fabs((double)task - focus_task);

   if (width <= 0.0)
      return (distance < 0.5) ? 1.0 : 0.0;
   if (distance >= width)
      return 0.0;
   return 1.0 - (distance / width);
   }  /* task_focus_weight */

static double smoothstep(double x)
   {
   if (x < 0.0) return 0.0;
   if (x > 1.0) return 1.0;
   return x * x * (3.0 - 2.0 * x);
   }  /* smoothstep */

static int nsew_expanded_task_id(int position)
   {
   int lanes = Num_tasks / 4;
   int zero_based = position - 1;
   int lane = zero_based / 4;
   int direction = zero_based % 4;

   /* Direction order is N,E,S,W. With four lanes this gives:
      N1,E1,S1,W1,N2,E2,S2,W2,... */
   return (direction * lanes) + lane + 1;
   }  /* nsew_expanded_task_id */

static int use_nsew_expanded_vector_path()
   {
   return (Task_opposition_mode == 1 && Num_tasks >= 4 &&
           (Num_tasks % 4) == 0);
   }  /* use_nsew_expanded_vector_path */

static void add_ordered_focus_to_raw_step(double raw_step[], double focus,
                                          double width)
   {
   int pos;

   for (pos=1; pos<=Num_tasks; pos++)
      {
      int task = use_nsew_expanded_vector_path() ?
         nsew_expanded_task_id(pos) : pos;
      raw_step[task] = task_focus_weight(pos, focus, width);
      }
   }  /* add_ordered_focus_to_raw_step */

static int demand_segment_len_for_all_tasks()
   {
   int segment_len = (Demand_segment_len > 0) ? Demand_segment_len : 100;
   int min_segments = (Num_tasks > 1) ? Num_tasks : 1;
   int max_segment_len = (Max_steps > 0) ? Max_steps / min_segments : segment_len;

   if (max_segment_len < 1) max_segment_len = 1;
   if (segment_len > max_segment_len) segment_len = max_segment_len;
   return segment_len;
   }  /* demand_segment_len_for_all_tasks */

static void legacy_vector_raw_step(int t, double raw_step[])
   {
   int task;
   int segment_len = demand_segment_len_for_all_tasks();
   double focus = 1.0;
   double width = 1.0;

   for (task=1; task<=Num_tasks; task++) raw_step[task] = 0.0;

   if (!strcmp(Task_demand_pattern, "legacy_vector_random"))
      {
      int focus_task = target_path_uniform(Num_tasks) + 1;
      raw_step[focus_task] = 1.0;
      return;
      }

   if (!strcmp(Task_demand_pattern, "legacy_vector_sharp"))
      {
      int focus_task = ((t / segment_len) % Num_tasks) + 1;
      raw_step[focus_task] = 1.0;
      return;
      }

   if (!strcmp(Task_demand_pattern, "legacy_vector_zigzag"))
      {
      int span = (Num_tasks > 1) ? Num_tasks - 1 : 1;
      int cycle_len = 2 * span * segment_len;
      int cycle_t = (cycle_len > 0) ? (t % cycle_len) : 0;
      double progress = (double)cycle_t / (double)segment_len;

      if (progress <= (double)span)
         focus = 1.0 + progress;
      else
         focus = 1.0 + (double)(2 * span) - progress;
      width = (Num_tasks > 2) ? 1.25 : 1.0;
      add_ordered_focus_to_raw_step(raw_step, focus, width);
      return;
      }
   else if (!strcmp(Task_demand_pattern, "legacy_vector_scurve"))
      {
      int switch_step = (Demand_switch_step > 0 &&
                         Demand_switch_step < Max_steps) ?
                        Demand_switch_step : 0;
      double transition_len = (switch_step > 0) ?
         (double)(segment_len * Num_tasks) : (double)Max_steps;
      double progress = smoothstep(((double)t - (double)switch_step) /
                                   transition_len);

      focus = 1.0 + progress * (double)(Num_tasks - 1);
      width = (Num_tasks > 2) ? 1.50 : 1.0;
      add_ordered_focus_to_raw_step(raw_step, focus, width);
      return;
      }
   else if (!strcmp(Task_demand_pattern, "legacy_vector") ||
            !strcmp(Task_demand_pattern, "legacy_vector_wave"))
      {
      double period = (double)segment_len;
      for (task=1; task<=Num_tasks; task++)
         {
         double phase = 2.0 * M_PI *
            ((double)t / period + (double)(task - 1) / (double)Num_tasks);
         raw_step[task] = 0.20 + 0.30 * (0.5 + 0.5 * sin(phase));
         }
      return;
      }
   else
      {
      printf(" Error(legacy_vector_raw_step): invalid Task_demand_pattern: %s\n",
             Task_demand_pattern);
      raw_step[1] = 1.0;
      return;
      }

   add_ordered_focus_to_raw_step(raw_step, focus, width);
   }  /* legacy_vector_raw_step */

static int is_legacy_vector_pattern()
   {
   return (!strcmp(Task_demand_pattern, "legacy_vector") ||
           !strcmp(Task_demand_pattern, "legacy_vector_random") ||
           !strcmp(Task_demand_pattern, "legacy_vector_sharp") ||
           !strcmp(Task_demand_pattern, "legacy_vector_scurve") ||
           !strcmp(Task_demand_pattern, "legacy_vector_zigzag") ||
           !strcmp(Task_demand_pattern, "legacy_vector_wave"));
   }  /* is_legacy_vector_pattern */

static void update_legacy_vector_error_and_demand()
   {
   int task;
   double mean_error = 0.0;

   if (Task_opposition_mode == 1)
      {
      int half_tasks = Num_tasks / 2;

      for (task=1; task<=half_tasks; task++)
         {
         int opposite_task = task + half_tasks;
         double paired_error =
            (Task_target_vector[task] - Task_tracker_vector[task]) -
            (Task_target_vector[opposite_task] -
             Task_tracker_vector[opposite_task]);

         Task_signed_error[task] = paired_error;
         Task_signed_error[opposite_task] = -paired_error;
         Task_demand[task] = (paired_error > 0.0) ? paired_error : 0.0;
         Task_demand[opposite_task] =
            (paired_error < 0.0) ? -paired_error : 0.0;
         }
      return;
      }

   for (task=1; task<=Num_tasks; task++)
      mean_error += Task_target_vector[task] - Task_tracker_vector[task];
   mean_error /= (double)Num_tasks;

   for (task=1; task<=Num_tasks; task++)
      {
      Task_signed_error[task] =
         Task_target_vector[task] - Task_tracker_vector[task] - mean_error;
      Task_demand[task] =
         (Task_signed_error[task] > 0.0) ? Task_signed_error[task] : 0.0;
      }
   }  /* update_legacy_vector_error_and_demand */

static void update_legacy_vector_pre_service_error()
   {
   update_legacy_vector_error_and_demand();
   }  /* update_legacy_vector_pre_service_error */

static void update_legacy_vector_post_service_demand()
   {
   update_legacy_vector_error_and_demand();
   }  /* update_legacy_vector_post_service_demand */

void update_task_demands(int t)
   {
   int task;
   int segment_len = demand_segment_len_for_all_tasks();
   double period = (double)segment_len;
   double max_workload = Range * 2.0;
   double total_arrival = Range * 0.80;
   double raw_arrival[MAX_TASKS + 1];
   double raw_sum = 0.0;
   double phase;
   double baseline = 0.0;
   double high = 1.00;
   double focus;
   double width;

   Current_timestep = t;

   if (is_legacy_vector_pattern())
      {
      double raw_step[MAX_TASKS + 1];
      double raw_norm = 0.0;
      double step_len = Target.step_len;

      legacy_vector_raw_step(t, raw_step);

      for (task=1; task<=Num_tasks; task++)
         raw_norm += raw_step[task] * raw_step[task];
      raw_norm = sqrt(raw_norm);
      if (raw_norm <= 0.0) raw_norm = 1.0;

      for (task=1; task<=Num_tasks; task++)
         {
         Task_prev_demand[task] = (Task_opposition_mode == 1) ?
            Task_demand[task] : Task_signed_error[task];
         Task_arrival[task] = step_len * raw_step[task] / raw_norm;
         Task_target_vector[task] += Task_arrival[task];
         Task_total_arrival[task] = Task_target_vector[task];
         }
      update_legacy_vector_pre_service_error();
      return;
      }

   for (task=1; task<=Num_tasks; task++)
      {
      Task_prev_demand[task] = Task_demand[task];
      if (!strcmp(Task_demand_pattern, "random"))
         {
         raw_arrival[task] = 0.10 + knuth_random();
         }
      else if (!strcmp(Task_demand_pattern, "zigzag"))
         {
         /* Gradual iterative demand: a high-demand focus sweeps through
            task ids and then reverses, like a zigzag path in task space.
            The segment length is capped so every task becomes dominant at
            least once in a standard run. */
         int span = (Num_tasks > 1) ? Num_tasks - 1 : 1;
         int cycle_len = 2 * span * segment_len;
         int cycle_t = (cycle_len > 0) ? (t % cycle_len) : 0;
         double progress = (double)cycle_t / (double)segment_len;
         if (progress <= (double)span)
            focus = 1.0 + progress;
         else
            focus = 1.0 + (double)(2 * span) - progress;

         width = (Num_tasks > 2) ? 1.25 : 1.0;
         raw_arrival[task] = baseline + high *
            task_focus_weight(task, focus, width);
         }
      else if (!strcmp(Task_demand_pattern, "sharp"))
         {
         /* Sudden iterative demand: the high-demand task changes abruptly
            every segment, wrapping over 1..Num_tasks. The effective segment
            length is capped so every task gets a high-demand phase. */
         int focus_task = ((t / segment_len) % Num_tasks) + 1;
         raw_arrival[task] = baseline + ((task == focus_task) ? high : 0.0);
         }
      else if (!strcmp(Task_demand_pattern, "scurve"))
         {
         /* Gradual non-iterative demand: one smooth S-curve transition from
            task 1 dominance to task Num_tasks dominance. The transition spans
            the run by default, so every task becomes dominant along the way. */
         int switch_step = (Demand_switch_step > 0 &&
                            Demand_switch_step < Max_steps) ?
                           Demand_switch_step : 0;
         double transition_len = (switch_step > 0) ?
            (double)(segment_len * Num_tasks) : (double)Max_steps;
         double progress = smoothstep(((double)t - (double)switch_step) /
                                      transition_len);
         focus = 1.0 + progress * (double)(Num_tasks - 1);
         width = (Num_tasks > 2) ? 1.50 : 1.0;
         raw_arrival[task] = baseline + high *
            task_focus_weight(task, focus, width);
         }
      else if (!strcmp(Task_demand_pattern, "cyclic") ||
               !strcmp(Task_demand_pattern, "wave"))
         {
         /* Smooth iterative demand retained as a sinusoidal wave option. */
         phase = 2.0 * M_PI *
            ((double)t / period + (double)(task - 1) / (double)Num_tasks);
         raw_arrival[task] = 0.20 + 0.30 * (0.5 + 0.5 * sin(phase));
         }
      else
         {
         printf(" Error(update_task_demands): invalid Task_demand_pattern: %s\n",
                Task_demand_pattern);
         raw_arrival[task] = 1.0;
         }
      raw_sum += raw_arrival[task];
      }

   if (raw_sum <= 0.0) raw_sum = 1.0;
   for (task=1; task<=Num_tasks; task++)
      {
      Task_arrival[task] = total_arrival * raw_arrival[task] / raw_sum;
      Task_total_arrival[task] += Task_arrival[task];
      Task_demand[task] += Task_arrival[task];
      if (Task_demand[task] > max_workload)
         Task_demand[task] = max_workload;
      }
   }  /* update_task_demands */

static void apply_pid_integral_antiwindup(int task);
static double noisy_task_feedback(int task, double true_error);

void update_pre_service_task_error()
   {
   int task;
   double residual_norm = 0.0;
   double demand_mean = 0.0;

   if (is_legacy_vector_pattern())
      {
      for (task=1; task<=Num_tasks; task++)
         {
         Task_prev_feedback_error[task] = Task_feedback_error[task];
         Task_feedback_error[task] =
            noisy_task_feedback(task, Task_signed_error[task]);
         residual_norm += Task_signed_error[task] * Task_signed_error[task];
         Task_error_integral[task] += Task_feedback_error[task];
         apply_pid_integral_antiwindup(task);
         }
      Tracker.pre_service_difference = sqrt(residual_norm);
      return;
      }

   for (task=1; task<=Num_tasks; task++)
      demand_mean += Task_demand[task];
   demand_mean /= (double)Num_tasks;

   for (task=1; task<=Num_tasks; task++)
      {
      double centered_error = Task_demand[task] - demand_mean;
      Task_prev_feedback_error[task] = Task_feedback_error[task];
      Task_feedback_error[task] = noisy_task_feedback(task, centered_error);
      residual_norm += Task_demand[task] * Task_demand[task];
      Task_error_integral[task] += Task_feedback_error[task];
      apply_pid_integral_antiwindup(task);
      }

   Tracker.pre_service_difference = sqrt(residual_norm);
   }  /* update_pre_service_task_error */

static unsigned long long feedback_noise_mix(unsigned long long x)
   {
   x += 0x9e3779b97f4a7c15ULL;
   x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
   x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
   return x ^ (x >> 31);
   }  /* feedback_noise_mix */

static double feedback_noise_uniform(int task, int salt)
   {
   unsigned long long x = (unsigned long long)Feedback_noise_seed;
   x ^= ((unsigned long long)(Current_timestep + 1)) * 0x9e3779b97f4a7c15ULL;
   x ^= ((unsigned long long)(task + 17)) * 0xbf58476d1ce4e5b9ULL;
   x ^= ((unsigned long long)(salt + 101)) * 0x94d049bb133111ebULL;
   x = feedback_noise_mix(x);
   return ((double)(x >> 11) + 0.5) * (1.0 / 9007199254740992.0);
   }  /* feedback_noise_uniform */

static double feedback_error_scale()
   {
   int task;
   int active = 0;
   double active_sum = 0.0;

   if (!Feedback_noise_enabled)
      return 0.0;

   for (task=1; task<=Num_tasks; task++)
      {
      if (Task_demand[task] > 0.0)
         {
         active_sum += Task_demand[task];
         active++;
         }
      }

   if (active <= 0) return 0.0;
   return active_sum / (double)active;
   }  /* feedback_error_scale */

static double feedback_bias_value(int task, double scale)
   {
   double sign = 1.0;
   unsigned long long x;

   if (Feedback_bias_alpha == 0.0 || Feedback_bias_mode == 0 || scale <= 0.0)
      return 0.0;

   if (Feedback_bias_mode == 1)
      {
      x = (unsigned long long)Feedback_bias_seed;
      x ^= ((unsigned long long)(task + 31)) * 0xbf58476d1ce4e5b9ULL;
      x = feedback_noise_mix(x);
      sign = (x & 1ULL) ? 1.0 : -1.0;
      }

   return Feedback_bias_alpha * scale * sign;
   }  /* feedback_bias_value */

static double noisy_task_feedback(int task, double true_error)
   {
   double scale = feedback_error_scale();
   double sigma = Feedback_noise_alpha * scale;
   double bias = feedback_bias_value(task, scale);
   double noisy_error;
   double u1;
   double u2;
   double z;

   if (sigma <= 0.0 && bias == 0.0) return true_error;

   noisy_error = true_error + bias;
   if (sigma > 0.0)
      {
      u1 = feedback_noise_uniform(task, 0);
      u2 = feedback_noise_uniform(task, 1);
      if (u1 <= 0.0) u1 = 1.0e-12;
      z = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
      noisy_error += sigma * z;
      }

   if (Feedback_noise_clip)
      {
      if (noisy_error > (double)Pop_size) noisy_error = (double)Pop_size;
      else if (noisy_error < -(double)Pop_size) noisy_error = -(double)Pop_size;
      }

   return noisy_error;
   }  /* noisy_task_feedback */

void apply_task_service()
   {
   int task;
   int service_denominator = (Removal_capacity_mode == 1) ? Pop_size : Num_alive;
   double service_per_agent = (service_denominator > 0) ?
      (Tracker.max_step_len / (double)service_denominator) : 0.0;

   if (is_legacy_vector_pattern())
      {
      for (task=1; task<=Num_tasks; task++)
         {
         Task_service[task] =
            (double)Task_actor_count[task] * service_per_agent;
         Task_tracker_vector[task] += Task_service[task];
         Task_total_service[task] = Task_tracker_vector[task];
         }
      update_legacy_vector_post_service_demand();
      update_task_vector_stats();
      return;
      }

   for (task=1; task<=Num_tasks; task++)
      {
      Task_service[task] = (double)Task_actor_count[task] * service_per_agent;
      if (Task_service[task] > Task_demand[task])
         Task_service[task] = Task_demand[task];
      Task_demand[task] -= Task_service[task];
      Task_total_service[task] += Task_service[task];
      }
   update_task_vector_stats();
   }  /* apply_task_service */

void update_task_vector_stats()
   {
   int task;
   double target_norm = 0.0;
   double tracker_norm = 0.0;
   double service_step_norm = 0.0;
   double residual_norm = 0.0;

   if (is_legacy_vector_pattern())
      {
      for (task=1; task<=Num_tasks; task++)
         {
         target_norm += Task_target_vector[task] * Task_target_vector[task];
         tracker_norm += Task_tracker_vector[task] * Task_tracker_vector[task];
         service_step_norm += Task_service[task] * Task_service[task];
         residual_norm += Task_signed_error[task] * Task_signed_error[task];
         }
      }
   else
      {
      for (task=1; task<=Num_tasks; task++)
         {
         target_norm += Task_total_arrival[task] * Task_total_arrival[task];
         tracker_norm += Task_total_service[task] * Task_total_service[task];
         service_step_norm += Task_service[task] * Task_service[task];
         residual_norm += Task_demand[task] * Task_demand[task];
         }
      }

   Target.length = sqrt(target_norm);
   Tracker.length = sqrt(tracker_norm);
   Tracker.current_step_len = sqrt(service_step_norm);
   Tracker.difference = sqrt(residual_norm);
   }  /* update_task_vector_stats */

void update_run_metrics(int t)
   {
   int task;
   int half_tasks;
   double raw_residual[MAX_TASKS + 1];
   double raw_squared = 0.0;
   double paired_squared = 0.0;
   double step_R = 0.0;
   double step_R_abs;
   double step_R2;
   double step_R2_norm;
   double task_scale;

   if (Num_tasks < 2 || (Num_tasks % 2) != 0) return;
   half_tasks = Num_tasks / 2;
   task_scale = sqrt((double)Num_tasks);

   for (task=1; task<=Num_tasks; task++)
      {
      raw_residual[task] = is_legacy_vector_pattern() ?
         Task_target_vector[task] - Task_tracker_vector[task] :
         Task_total_arrival[task] - Task_total_service[task];
      raw_squared += raw_residual[task] * raw_residual[task];
      }

   for (task=1; task<=half_tasks; task++)
      {
      double paired_error =
         raw_residual[task] - raw_residual[task + half_tasks];
      step_R += fabs(paired_error);
      paired_squared += paired_error * paired_error;
      }

   step_R /= (double)half_tasks;
   step_R_abs = sqrt(raw_squared) / task_scale;
   /* The full paired-feedback vector contains e and -e for each pair. */
   step_R2 = sqrt(2.0 * paired_squared);
   step_R2_norm = step_R2 / task_scale;

   Tracker.metric_sum_R += step_R;
   Tracker.metric_sum_R_abs += step_R_abs;
   Tracker.metric_sum_R2 += step_R2;
   Tracker.metric_sum_R2_norm += step_R2_norm;
   if (step_R2_norm > Tracker.metric_max_R2_norm)
      Tracker.metric_max_R2_norm = step_R2_norm;

   /* Setting First_extinction=500 also gives the matched 0% control its
      post-removal observation window, even when Kill_number is zero. */
   if (First_extinction >= 0 && t >= First_extinction)
      {
      Tracker.post_removal_sum_R += step_R;
      Tracker.post_removal_sum_R_abs += step_R_abs;
      Tracker.post_removal_sum_R2_norm += step_R2_norm;
      if (step_R2_norm > Tracker.post_removal_max_R2_norm)
         Tracker.post_removal_max_R2_norm = step_R2_norm;
      Tracker.post_removal_steps++;
      }
   }  /* update_run_metrics */

static void apply_pid_integral_antiwindup(int task)
   {
   if (Pid_integral_leak > 0.0 && Pid_integral_leak < 1.0)
      Task_error_integral[task] *= Pid_integral_leak;
   if (Pid_integral_bound > 0.0)
      {
      if (Task_error_integral[task] > Pid_integral_bound)
         Task_error_integral[task] = Pid_integral_bound;
      else if (Task_error_integral[task] < -Pid_integral_bound)
         Task_error_integral[task] = -Pid_integral_bound;
      }
   }  /* apply_pid_integral_antiwindup */

static double agent_pid_gain_uniform(int agent_num, int gain_component)
   {
   unsigned long long x = (unsigned long long)Seed;

   /* Keep gain sampling independent of the simulator's main random stream so
      Global and Agent runs retain matched threshold and task-choice draws. */
   x ^= ((unsigned long long)(agent_num + 1)) * 0x9e3779b97f4a7c15ULL;
   x ^= ((unsigned long long)(gain_component + 1)) * 0xbf58476d1ce4e5b9ULL;
   x = feedback_noise_mix(x);
   return ((double)(x >> 11) + 0.5) * (1.0 / 9007199254740992.0);
   }  /* agent_pid_gain_uniform */

static double sample_agent_pid_multiplier(
   double spread, int agent_num, int gain_component)
   {
   double exponent;

   if (spread <= 0.0) return 1.0;
   exponent =
      (agent_pid_gain_uniform(agent_num, gain_component) * 2.0 - 1.0) *
      2.0 * spread;
   return pow(2.0, exponent);
   }  /* sample_agent_pid_multiplier */

static void init_agent_pid_gains(int n)
   {
   double p_mult = 1.0;
   double i_mult = 1.0;
   double d_mult = 1.0;
   double p_spread = Agent_pid_p_spread;
   double i_spread = Agent_pid_i_spread;
   double d_spread = Agent_pid_d_spread;

   if (p_spread <= 0.0 && Agent_pid_gain_spread > 0.0) p_spread = Agent_pid_gain_spread;
   if (i_spread <= 0.0 && Agent_pid_gain_spread > 0.0) i_spread = Agent_pid_gain_spread;
   if (d_spread <= 0.0 && Agent_pid_gain_spread > 0.0) d_spread = Agent_pid_gain_spread;

   if (Agent_pid_gains)
      {
      /* The manuscript defines an independent draw for each gain component.
         A zero spread produces multiplier 1 for that component. */
      p_mult = sample_agent_pid_multiplier(p_spread, n, 0);
      i_mult = sample_agent_pid_multiplier(i_spread, n, 1);
      d_mult = sample_agent_pid_multiplier(d_spread, n, 2);
      }

   Agent[n].agent_P_gain = P_gain * p_mult;
   Agent[n].agent_I_gain = I_gain * i_mult;
   Agent[n].agent_D_gain = D_gain * d_mult;
   }  /* init_agent_pid_gains */

static double clamp_raw_threshold(double value)
   {
   if (value < 0.0) return 0.0;
   if (value > 1.0) return 1.0;
   return value;
   }

static double sample_task_raw_threshold(int n, int task)
   {
   double rand1, rand2, r, theta;
   double k, x;
   double p;

   if (Thresh_init <= 1.0 && Thresh_init >= 0.0)
      return clamp_raw_threshold(Thresh_init);
   else if (Thresh_init == 2.0)
      return clamp_raw_threshold(funiform(1.0));
   else if (Thresh_init == 3.0)
      {
      rand1 = knuth_random();
      rand2 = knuth_random();
      r = sqrt(-2 * log(rand1));
      theta = 2 * M_PI * rand2;
      return clamp_raw_threshold((r * cos(theta))/6+0.5);
      }
   else if (Thresh_init == 4.0)
      {
      k = (int)(knuth_random() * (POISSON_K_RANGE + 1));
      p = pow(LAMBDA_P, k) * exp(-1.0 * LAMBDA_P) / (double)factorial(k);
      return clamp_raw_threshold(p * LAMBDA_P);
      }
   else if (Thresh_init == 5.0)
      {
      x = knuth_random() * EXP_X_RANGE;
      return clamp_raw_threshold(exp(-1.0 * LAMBDA_E * x));
      }
   else if (Thresh_init == 6.0)
      {
      k = (int)(knuth_random() * LOG_K_RANGE + 1);
      p = (-1.0 / log(1.0 - LOG_P)) * (pow(LOG_P, k) / k);
      return clamp_raw_threshold(p * 5);
      }
   else if (Thresh_init == 10.0)
      {
      if (task == 1) return clamp_raw_threshold(Agent[n].raw_thresh_north);
      if (task == 2) return clamp_raw_threshold(Agent[n].raw_thresh_east);
      if (task == 3) return clamp_raw_threshold(Agent[n].raw_thresh_south);
      if (task == 4) return clamp_raw_threshold(Agent[n].raw_thresh_west);
      return clamp_raw_threshold(funiform(1.0));
      }

   printf(" Error(init_agent_task_channels): Invalid value for Thresh_init: %lf\n",
          Thresh_init);
   return -1.0;
   }

void sync_direction_thresholds_from_tasks(int n)
   {
   int task;
   double raw[4] = {0, 0, 0, 0};
   double thresh[4] = {0, 0, 0, 0};
   double minv[4] = {0, 0, 0, 0};
   double maxv[4] = {0, 0, 0, 0};
   double raw_min[4] = {0, 0, 0, 0};
   double raw_max[4] = {0, 0, 0, 0};

   for (task=1; task<=4 && task<=Num_tasks; task++)
      {
      raw[task-1] = Agent[n].raw_thresh_task[task];
      thresh[task-1] = Agent[n].thresh_task[task];
      minv[task-1] = Agent[n].thresh_min_task[task];
      maxv[task-1] = Agent[n].thresh_max_task[task];
      raw_min[task-1] = Agent[n].raw_thresh_min_task[task];
      raw_max[task-1] = Agent[n].raw_thresh_max_task[task];
      }

   Agent[n].raw_thresh_north = raw[0];
   Agent[n].raw_thresh_east = raw[1];
   Agent[n].raw_thresh_south = raw[2];
   Agent[n].raw_thresh_west = raw[3];
   Agent[n].thresh_north = thresh[0];
   Agent[n].thresh_east = thresh[1];
   Agent[n].thresh_south = thresh[2];
   Agent[n].thresh_west = thresh[3];
   Agent[n].thresh_min_north = minv[0];
   Agent[n].thresh_min_east = minv[1];
   Agent[n].thresh_min_south = minv[2];
   Agent[n].thresh_min_west = minv[3];
   Agent[n].thresh_max_north = maxv[0];
   Agent[n].thresh_max_east = maxv[1];
   Agent[n].thresh_max_south = maxv[2];
   Agent[n].thresh_max_west = maxv[3];
   Agent[n].raw_thresh_min_north = raw_min[0];
   Agent[n].raw_thresh_min_east = raw_min[1];
   Agent[n].raw_thresh_min_south = raw_min[2];
   Agent[n].raw_thresh_min_west = raw_min[3];
   Agent[n].raw_thresh_max_north = raw_max[0];
   Agent[n].raw_thresh_max_east = raw_max[1];
   Agent[n].raw_thresh_max_south = raw_max[2];
   Agent[n].raw_thresh_max_west = raw_max[3];
   }  /* sync_direction_thresholds_from_tasks */

int init_agent_task_channels(int n)
   {
   int task;
   double temp;
   double range;

   Agent[n].raw_thresh_task[0] = -1.0;
   Agent[n].thresh_task[0] = -1.0;
   Agent[n].raw_thresh_min_task[0] = -1.0;
   Agent[n].raw_thresh_max_task[0] = -1.0;
   Agent[n].thresh_min_task[0] = -1.0;
   Agent[n].thresh_max_task[0] = -1.0;

   for (task=1; task<=Num_tasks; task++)
      {
      if (Thresh_dynamic == 2)
         {
         Agent[n].raw_thresh_min_task[task] = knuth_random() * 0.49;
         Agent[n].raw_thresh_max_task[task] = knuth_random() * 0.49 + 0.51;
         }
      else if (Thresh_dynamic == 3)
         {
         Agent[n].raw_thresh_min_task[task] = knuth_random();
         Agent[n].raw_thresh_max_task[task] = knuth_random();
         if (Agent[n].raw_thresh_min_task[task] >
             Agent[n].raw_thresh_max_task[task])
            {
            temp = Agent[n].raw_thresh_min_task[task];
            Agent[n].raw_thresh_min_task[task] =
               Agent[n].raw_thresh_max_task[task];
            Agent[n].raw_thresh_max_task[task] = temp;
            }
         }
      else
         {
         Agent[n].raw_thresh_min_task[task] = 0.0;
         Agent[n].raw_thresh_max_task[task] = 1.0;
         }

      if (Thresh_dynamic == 2 || Thresh_dynamic == 3)
         {
         if (Thresh_dynamic_init == 0)
            {
            range = Agent[n].raw_thresh_max_task[task] -
                    Agent[n].raw_thresh_min_task[task];
            Agent[n].raw_thresh_task[task] =
               knuth_random() * range + Agent[n].raw_thresh_min_task[task];
            }
         else if (Thresh_dynamic_init == 1)
            {
            Agent[n].raw_thresh_task[task] =
               (Agent[n].raw_thresh_max_task[task] +
                Agent[n].raw_thresh_min_task[task]) / 2.0;
            }
         else if (Thresh_dynamic_init == 2)
            {
            Agent[n].raw_thresh_task[task] = 0.5;
            }
         else
            {
            printf(" Error(init_agent_task_channels): Invalid value for Thresh_dynamic_init: %d\n",
                   Thresh_dynamic_init);
            return ERROR;
            }
         }
      else
         {
         Agent[n].raw_thresh_task[task] =
            sample_task_raw_threshold(n, task);
         if (Agent[n].raw_thresh_task[task] < 0.0) return ERROR;
         }

      Agent[n].thresh_task[task] = Agent[n].raw_thresh_task[task] * Range;
      Agent[n].thresh_min_task[task] =
         Agent[n].raw_thresh_min_task[task] * Range;
      Agent[n].thresh_max_task[task] =
         Agent[n].raw_thresh_max_task[task] * Range;
      }

   sync_direction_thresholds_from_tasks(n);
   return OK;
   }  /* init_agent_task_channels */

/********** scale_thresholds **********/
/* created:	20.04.16.ASW
   parameters:	n	agent number
   called by:   init_agent(), fxn.c
   actions:	scale the raw thresholds which are relative to the range
		[0.0,1.0] to a range of [0.0,Range].
*/
void scale_thresholds(int n)
   {
#ifdef DEBUG
printf("---in scale_thresholds()---\n");
#endif

   // scale thresholds
   Agent[n].thresh_north = Agent[n].raw_thresh_north * Range;
   Agent[n].thresh_south = Agent[n].raw_thresh_south * Range;
   Agent[n].thresh_east = Agent[n].raw_thresh_east * Range;
   Agent[n].thresh_west = Agent[n].raw_thresh_west * Range;

   // scale threshold ranges
   if (Thresh_dynamic == 1 || Thresh_dynamic == 2 || Thresh_dynamic == 3)
      {
      Agent[n].thresh_min_north = Agent[n].raw_thresh_min_north * Range;
      Agent[n].thresh_min_south = Agent[n].raw_thresh_min_south * Range;
      Agent[n].thresh_min_east = Agent[n].raw_thresh_min_east * Range;
      Agent[n].thresh_min_west = Agent[n].raw_thresh_min_west * Range;
      Agent[n].thresh_max_north = Agent[n].raw_thresh_max_north * Range;
      Agent[n].thresh_max_south = Agent[n].raw_thresh_max_south * Range;
      Agent[n].thresh_max_east = Agent[n].raw_thresh_max_east * Range;
      Agent[n].thresh_max_west = Agent[n].raw_thresh_max_west * Range;
      }

#ifdef DEBUG
printf("---end scale_thresholds()---\n");
#endif
   }  /* scale_thresholds */


/********** init_target **********/
/* parameters:
   called by:   init_fxn(), fxn.c
   actions:	initialize target
*/
int init_target()
   {
   int i;

#ifdef DEBUG
printf("---in init_target()---\n");
#endif

   /* initialize working area and length of path travelled */
//   Target.width = 100;
//   Target.height = 100;
   /* initialize length of path travelled */
   Target.length = 0;
   /* angle is only use in random path right now, but init to zero
      so it can be printed as a parameter */
   Target.angle = 0;

   /* allocate space */
   Target.abbrev = (char *)malloc(INPUT_LINE_LEN * sizeof(char));

   /* initialize path details */
   if ( !strcmp(Target_path, "square") )
      {
      /* target travels a 60x60 square with corners at 20,20 and 80,80 */
      sprintf(Target.abbrev, "squ");
      /* set start location to bottom left, go clockwise */
      Target.x = 20;
      Target.y = 20;
      /* as a result, initial movement will be along left side */
      sprintf(Target.side, "left");
      }
   else if ( !strcmp(Target_path, "square10") )
      {
      /* target travels a 10x10 square with corners at 20,20 and 30,30 */
      sprintf(Target.abbrev, "s10");
      /* set start location to bottom left, go clockwise */
      Target.x = 20;
      Target.y = 20;
      /* as a result, initial movement will be along left side */
      sprintf(Target.side, "left");
      }
      else if ( !strcmp(Target_path, "step") )
      {
      
      /* set start location to bottom left, go clockwise */
      Target.x = 0;
      Target.y = 0;
      /* as a result, initial movement will be along left side */
      sprintf(Target.side, "left");
      }
   else if( !strcmp(Target_path, "diamond"))
      {
      Target.x = 0;
      Target.y = 0;
      }
   else if( !strcmp(Target_path, "square_flex"))
      {
      Target.x = 0;
      Target.y = 0;
      }
   else if ( !strcmp(Target_path, "circle") )
      {
      sprintf(Target.abbrev, "cir");
      /* set start location to top center, go clockwise, radius of 30 */
      /* center at 0, 0 */
      Target.x = 0;
      Target.y = Circle_radius;
      // the next three lines will initialize the target to a random
      // position on the circle
      // float angle = knuth_random() * 2 * M_PI;
      // Target.x = Circle_radius * cos(angle);
      // Target.y = Circle_radius * sin(angle);
      }
   else if ( !strcmp(Target_path, "random") )
      {
      sprintf(Target.abbrev, "rnd");
      /* set start location to 0,0 */
      Target.x = 0;
      Target.y = 0;
      /* reproducible random angle between 0 and 2*PI for init orientation */
      Target.angle = target_path_funiform(2 * M_PI);
      }
   else if ( !strcmp(Target_path, "random_manhattan") )
      {
      sprintf(Target.abbrev, "man");
      /* set start location to 0,0 */
      Target.x = 0;
      Target.y = 0;
      }
   else if ( !strcmp(Target_path, "west") )
      {
      sprintf(Target.abbrev, "wes");
      /* set start location to 0,0 */
      Target.x = 0;
      Target.y = 0;
      }
   else if ( !strcmp(Target_path, "step") )
      {
      sprintf(Target.abbrev, "stp");
      /* set start location to 0,0 */
      Target.x = 0;
      Target.y = 0;
      }
   else if ( !strcmp(Target_path, "northeast") )
      {
      sprintf(Target.abbrev, "noe");
      /* set start location to 0,0 */
      Target.x = 0;
      Target.y = 0;
      }
   else if ( !strcmp(Target_path, "northeast_accel") )
      {
      sprintf(Target.abbrev, "nea");
      /* set start location to 0,0 */
      Target.x = 0;
      Target.y = 0;
      Target.change = 1.0;
      }
   else if ( !strcmp(Target_path, "sharp") )
      {
      sprintf(Target.abbrev, "sharp");
      Target.x = 0;
      Target.y = 0;
      /* reproducible random angle between -PI and PI for init orientation */
      Target.angle = target_path_funiform(2 * M_PI) - M_PI;
      /* reproducible random probability of changing directions
         between 10 and 50 percent */
      Target.change_probability = (target_path_random() / 32767.0) * 0.4 + 0.1;
      }
   else if ( !strcmp(Target_path, "random_steep") )
      {
      sprintf(Target.abbrev, "rand_steep");
      Target.x = 0;
      Target.y = 0;
      }
   else if ( !strcmp(Target_path, "zigzag") )
      {
      sprintf(Target.abbrev, "zigzag");
      Target.x = 0;
      Target.y = 0;
      Target.direction = 1;  // Start at one, moving up
      Target.angle = atan(Target.amplitude/(Target.period/4));
#ifdef DEBUG
printf(" zigzag angle %lf %lf\n", Target.angle, Target.angle/M_PI*180);
#endif
      }
   else if ( !strcmp(Target_path, "stationary_biased") )
      {
      sprintf(Target.abbrev, "stat_bias");
      Target.x = 0;
      Target.y = 0;
      }
   else if ( !strcmp(Target_path, "single_switch") )
      {
      sprintf(Target.abbrev, "sing_sw");
      Target.x = 0;
      Target.y = 0;
      }
   else if ( !strcmp(Target_path, "periodic_switch") )
      {
      sprintf(Target.abbrev, "per_sw");
      Target.x = 0;
      Target.y = 0;
      }
   else if ( !strcmp(Target_path, "scurve") )
      {
      sprintf(Target.abbrev, "scurve");
      Target.x = 0;
      Target.y = 0.01;
      Target.direction = 1;  // Start at one, add angle
      Target.angle = M_PI/2;
      Target.change = M_PI / (Target.period/Target.step_len);
                // Pi divided by min steps needed per period
//printf(" sine_like angle %lf %lf angle change %lf %lf\n",
//       Target.angle, Target.angle/M_PI*180,
//       Target.change, Target.change/M_PI*180);
      }
   else if ( !strcmp(Target_path, "sine") )
      {
      sprintf(Target.abbrev, "sine");
      Target.x = 0;
      Target.y = 0;
      }
   else if ( !strcmp(Target_path, "sine2") )
      {
      sprintf(Target.abbrev, "sine2");
      Target.x = 0;
      Target.y = 0;
      }
   else if ( !strcmp(Target_path, "figure_8") )
      {
      sprintf(Target.abbrev, "figure_8");
      Target.x = 0;
      Target.y = 0;
      }
      else if ( !strcmp(Target_path, "cross_square") )
      {
         sprintf(Target.abbrev, "cross");
         Target.x = 20;
         Target.y = 20;
      }
   else if ( !strcmp(Target_path, "steep") )
      {
         sprintf(Target.abbrev, "steep");
         Target.x = 0;
         Target.y = 0;
      }
   else if ( !strcmp(Target_path, "sineNew") )
      {
      sprintf(Target.abbrev, "sineNew");
      Target.x = 0;
      Target.y = 0;
      }
   else if ( !strcmp(Target_path, "rose") )
      {
      sprintf(Target.abbrev, "rose");
      Target.x = 30;
      Target.y = 0;
      Target.t = 0;
      }
   else if ( !strcmp(Target_path, "arcThenSine") )
      {
      sprintf(Target.abbrev, "arcThenSine");
      Target.x = 0;
      Target.y = 60;
      Target.t = 0;
      Target.xOff = 0;
      Target.yOff = 0;
      }
   else if ( !strcmp(Target_path, "sineThenIncreasingSine") )
      {
      sprintf(Target.abbrev, "sineThenIncreasingSine");
      Target.x = 0;
      Target.y = 0;
      Target.t = 0;
      Target.xOff = 0;
      Target.yOff = 0;
      }
   else if ( !strcmp(Target_path, "twoParabolas") )
      {
      sprintf(Target.abbrev, "twoParabolas");
      Target.x = 0;
      Target.y = 0;
      Target.t = 0;
      Target.xOff = 0;
      Target.yOff = 0;
      }

   else
      {
      printf(" Error(init_target): invalid target path: %s\n", Target_path);
      return ERROR;
      }

#ifdef DEBUG
printf("---end init_target()---\n");
#endif
   return OK;
   }  /* init_target */

/********** init_tracker **********/
/* paramters:
   called by:   init_fxn(), fxn.c
   actions:     initialize tracker
*/
int init_tracker()
   {
   int i;

#ifdef DEBUG
printf("---in init_tracker()---\n");
#endif

   Tracker.max_step_len = Tracker.step_ratio * Target.step_len;
   Tracker.length = 0;
   Tracker.x = Target.x;
   Tracker.y = Target.y;
   Tracker.total_difference = 0;
   Tracker.max_difference = -1;
   Tracker.min_difference = 9999999999;
   Tracker.pre_service_difference = 0;
   Tracker.total_pre_service_difference = 0;
   Tracker.max_pre_service_difference = -1;
   Tracker.min_pre_service_difference = 9999999999;
   Tracker.metric_sum_R = 0.0;
   Tracker.metric_sum_R_abs = 0.0;
   Tracker.metric_sum_R2 = 0.0;
   Tracker.metric_sum_R2_norm = 0.0;
   Tracker.metric_max_R2_norm = 0.0;
   Tracker.post_removal_sum_R = 0.0;
   Tracker.post_removal_sum_R_abs = 0.0;
   Tracker.post_removal_sum_R2_norm = 0.0;
   Tracker.post_removal_max_R2_norm = 0.0;
   Tracker.post_removal_steps = 0;

   /* HDM; sum intensities of all agents; 2019.09.12 */
   Tracker.intensity_all_north = 0.0;
   Tracker.intensity_all_south = 0.0;
   Tracker.intensity_all_east = 0.0;
   Tracker.intensity_all_west = 0.0;
   for (int i = 0; i < Pop_size; i++)
      {
      Tracker.intensity_all_north += Agent[i].intensity_north;
      Tracker.intensity_all_south += Agent[i].intensity_south;
      Tracker.intensity_all_east += Agent[i].intensity_east;
      Tracker.intensity_all_west += Agent[i].intensity_west;
      }
    // printf("total intensities north: %lf\n", Tracker.intensity_all_north);
    // printf("total intensities south: %lf\n", Tracker.intensity_all_south);
    // printf("total intensities east: %lf\n", Tracker.intensity_all_east);
    // printf("total intensities west: %lf\n", Tracker.intensity_all_west);

   // HDM; initialize histogram info for intensity aging
   if(Intensity_aging)
      {
      for (int i = 0; i < HISTOGRAM_SIZE; i++)
         {
         Tracker.intensity_north_dist[i] = 0;
         Tracker.intensity_south_dist[i] = 0;
         Tracker.intensity_east_dist[i] = 0;
         Tracker.intensity_west_dist[i] = 0;
         }
      }

#ifdef DEBUG
printf("---end init_tracker()---\n");
#endif
   return OK;
   }  /* init_tracker */

/********** print_fxn_params **********/
/* parameters:  fp      where to print, includes stdout (screen)
                        Assume that fp has already been fopened.
   called by:   start_output(), output.c
   actions:     prints out function related parameters.
*/
void print_fxn_params(FILE *fp)
   {
  /* things that were read in */
   fprintf(fp, "\n Target parameters:\n");
   fprintf(fp, " Target.abbrev =  %s\n", Target.abbrev);
   fprintf(fp, " Target.x =  %lf\n", Target.x);
   fprintf(fp, " Target.y =  %lf\n", Target.y);
   fprintf(fp, " Target.step_len =  %lf\n", Target.step_len);
   fprintf(fp, " Target.length =  %lf\n", Target.length);
   fprintf(fp, " Target.angle =  %lf\n", Target.angle);
   fprintf(fp, "\n Tracker parameters:\n");
   fprintf(fp, " Tracker.x =  %lf\n", Tracker.x);
   fprintf(fp, " Tracker.y =  %lf\n", Tracker.y);
   fprintf(fp, " Tracker.length =  %lf\n", Tracker.length);
   fprintf(fp, " Tracker.max_step_len =  %lf\n", Tracker.max_step_len);
   fprintf(fp, " Tracker.total_difference =  %lf\n", Tracker.total_difference);
   }  /* print_fxn_params */

/********** init_raw_thresholds **********/
/* created:     20.04.16.ASW moved from init_agent into this function
   parameters:	n	agent index number
   called by:   init_agent(), fxn.c
   actions:     initialize raw thresholds for each agent.
		Raw thresholds fall between 0.0 and 1.0.
		This function is used to set agent thresholds for static 
		thresholds (Thresh_dynamic = 0) and also used to set the 
		initial thresholds for dynamic thresholds with range [0,1]
		(Thresh_dynamic = 1).  It cannot be used for dynamic thresholds
		with variable ranges (Thresh_dynamic = 2) because we can't
		guarantee that the assigned threshold will be within the
		range if the range varies from agent to agent.
*/
int init_raw_thresholds(int n)
   {
   double rand1, rand2, r, theta;       // box muller variables
   double k_n, k_s, k_e, k_w; // discrete variables-poisson,log
   double x_n, x_s, x_e, x_w; // continuous variables-exponential
   double p_n, p_s, p_e, p_w; // poisson/exp result
   double l_n, l_s, l_e, l_w; // log result
   double range_width;  //difference between agent's thresh_max and thresh_min

#ifdef DEBUG
printf("---in init_raw_thresholds()---\n");
#endif

   Agent[n].raw_thresh_min = 0.0;
   Agent[n].raw_thresh_max = 1.0;

   // initialize raw thresholds
   if (Thresh_init <= 1.0 && Thresh_init >= 0.0)
      {
      /* make sure Thresh_init is within range */
      if (Thresh_init > Agent[n].raw_thresh_max)
         Thresh_init = Agent[n].raw_thresh_max;
      if (Thresh_init < Agent[n].raw_thresh_min)
         Thresh_init = Agent[n].raw_thresh_min;
      /* initialize all agents to same constant value */
      Agent[n].raw_thresh_north = Thresh_init;
      Agent[n].raw_thresh_south = Thresh_init;
      Agent[n].raw_thresh_east = Thresh_init;
      Agent[n].raw_thresh_west = Thresh_init;
      }  /* if */
   else if (Thresh_init == 2.0)
      {
      /* initialize randomly - uniform distribution */
      Agent[n].raw_thresh_north = funiform(1.0);
      Agent[n].raw_thresh_south = funiform(1.0);
      Agent[n].raw_thresh_east = funiform(1.0);
      Agent[n].raw_thresh_west = funiform(1.0);
      }  /* else if */
   else if (Thresh_init == 3.0)
      {
      /* initialize randomly - standard normal distribution */
      /* Use Box Muller equations.  Since sigma = 1, a range of 3 on */
      /* either side of zero should include most points.  Adding 0.5 moves */
      /* the range to 0.0 to 1.0 to match the uniform raw threshold range. */
      /* The divide by 6 is a range of 3 on either side of zero. */

      rand1 = knuth_random();
      rand2 = knuth_random();
      r = sqrt(-2 * log(rand1));
      theta = 2 * M_PI * rand2;

      Agent[n].raw_thresh_north = (r * cos(theta))/6+0.5;
      Agent[n].raw_thresh_south = (r * sin(theta))/6+0.5;
      rand1 = knuth_random();
      rand2 = knuth_random();
      r = sqrt(-2 * log(rand1));
      theta = 2 * M_PI * rand2;

      Agent[n].raw_thresh_east = (r * cos(theta))/6+0.5;
      Agent[n].raw_thresh_west = (r * sin(theta))/6+0.5;

/*
      printf("Raw threshold north: %lf\n", Agent[n].raw_thresh_north);
      printf("Raw threshold south: %lf\n", Agent[n].raw_thresh_south);
      printf("Raw threshold east: %lf\n", Agent[n].raw_thresh_east);
      printf("Raw threshold west: %lf\n", Agent[n].raw_thresh_west);
*/

      /* check to make sure all values are >= zero, in case any points */
      /* beyond 4 * sigma were generated. */
      if (Agent[n].raw_thresh_north < 0)  Agent[n].raw_thresh_north = 0;
      if (Agent[n].raw_thresh_south < 0)  Agent[n].raw_thresh_south = 0;
      if (Agent[n].raw_thresh_east < 0)  Agent[n].raw_thresh_east = 0;
      if (Agent[n].raw_thresh_west < 0)  Agent[n].raw_thresh_west = 0;
      }  /* else if */
   else if (Thresh_init == 4.0)
      {
      /* initialize randomly - poisson distribution */
      /* use the equation for Poisson probability dist by multiplying */
      /* our random floating pt value by a number of our choosing, we */
      /* can specify the range of 'k' values we can plug into formula.*/
      /* We arrive at our raw threshold values after scaling by lambda*/
      /* which is set as a constant for now. Change P_RANGE as needed.*/

      k_n = (int)(knuth_random() * (POISSON_K_RANGE + 1));
      k_s = (int)(knuth_random() * (POISSON_K_RANGE + 1));
      k_e = (int)(knuth_random() * (POISSON_K_RANGE + 1));
      k_w = (int)(knuth_random() * (POISSON_K_RANGE + 1));

      p_n = pow(LAMBDA_P, k_n) * exp(-1.0 * LAMBDA_P) / (double)factorial(k_n);
      p_s = pow(LAMBDA_P, k_s) * exp(-1.0 * LAMBDA_P) / (double)factorial(k_s);
      p_e = pow(LAMBDA_P, k_e) * exp(-1.0 * LAMBDA_P) / (double)factorial(k_e);
      p_w = pow(LAMBDA_P, k_w) * exp(-1.0 * LAMBDA_P) / (double)factorial(k_w);

      Agent[n].raw_thresh_north = p_n * LAMBDA_P;
      Agent[n].raw_thresh_south = p_s * LAMBDA_P;
      Agent[n].raw_thresh_east = p_e * LAMBDA_P;
      Agent[n].raw_thresh_west = p_w * LAMBDA_P;

      // check to make sure all values are >= zero
      if (Agent[n].raw_thresh_north < 0)  Agent[n].raw_thresh_north = 0;
      if (Agent[n].raw_thresh_south < 0)  Agent[n].raw_thresh_south = 0;
      if (Agent[n].raw_thresh_east < 0)  Agent[n].raw_thresh_east = 0;
      if (Agent[n].raw_thresh_west < 0)  Agent[n].raw_thresh_west = 0;
      }
   else if (Thresh_init == 5.0)
      {
      /* initialize randomly - exponential distribution use eqn.*/
      /* for exp dist. We generate random 'X' values  */
      /* on our range and evaluate with density function. */

      x_n = (knuth_random() * EXP_X_RANGE);
      x_s = (knuth_random() * EXP_X_RANGE);
      x_e = (knuth_random() * EXP_X_RANGE);
      x_w = (knuth_random() * EXP_X_RANGE);

      p_n = exp(-1.0 * LAMBDA_E * x_n);
      p_e = exp(-1.0 * LAMBDA_E * x_e);
      p_s = exp(-1.0 * LAMBDA_E * x_s);
      p_w = exp(-1.0 * LAMBDA_E * x_w);

      Agent[n].raw_thresh_north = p_n;
      Agent[n].raw_thresh_south = p_s;
      Agent[n].raw_thresh_east = p_e;
      Agent[n].raw_thresh_west = p_w;

      // check to make sure all values are >= zero
      if (Agent[n].raw_thresh_north < 0)  Agent[n].raw_thresh_north = 0;
      if (Agent[n].raw_thresh_south < 0)  Agent[n].raw_thresh_south = 0;
      if (Agent[n].raw_thresh_east < 0)  Agent[n].raw_thresh_east = 0;
      if (Agent[n].raw_thresh_west < 0)  Agent[n].raw_thresh_west = 0;
      }
   else if (Thresh_init == 6.0)
      {
      /* initialize randomly - logarithmic distribution */
      /* as with poisson distribution, we generate discrete random */
      /* variables on the desired range. Scaling by a factor of ~5 */
      /* seems to be effective as most results from the probability*/
      /* density function fall between [0,~.20].                   */

      k_n = (int)(knuth_random() * LOG_K_RANGE + 1);
      k_s = (int)(knuth_random() * LOG_K_RANGE + 1);
      k_e = (int)(knuth_random() * LOG_K_RANGE + 1);
      k_w = (int)(knuth_random() * LOG_K_RANGE + 1);

      l_n = (-1.0 / log(1.0 - LOG_P)) * (pow(LOG_P, k_n) / k_n);
      l_s = (-1.0 / log(1.0 - LOG_P)) * (pow(LOG_P, k_s) / k_s);
      l_e = (-1.0 / log(1.0 - LOG_P)) * (pow(LOG_P, k_e) / k_e);
      l_w = (-1.0 / log(1.0 - LOG_P)) * (pow(LOG_P, k_w) / k_w);

      Agent[n].raw_thresh_north = l_n * 5;
      Agent[n].raw_thresh_south = l_s * 5;
      Agent[n].raw_thresh_east = l_e * 5;
      Agent[n].raw_thresh_west = l_w * 5;

      // check to make sure all values are >= zero
      if (Agent[n].raw_thresh_north < 0)  Agent[n].raw_thresh_north = 0;
      if (Agent[n].raw_thresh_south < 0)  Agent[n].raw_thresh_south = 0;
      if (Agent[n].raw_thresh_east < 0)  Agent[n].raw_thresh_east = 0;
      if (Agent[n].raw_thresh_west < 0)  Agent[n].raw_thresh_west = 0;
      }
   else if (Thresh_init == 10.0)
      {
         //DANIEL'S CODE
         // printf("Loading Agent Threshold Values\n");
         // FILE *fp = fopen("thresholds.txt", "r");
         float thresh_n, thresh_e, thresh_s, thresh_w;

         // for(int i = 0; i<Pop_size; i++) {
         //    fscanf(fp, "%f %f %f %f", &thresh_n, &thresh_e, &thresh_s, &thresh_w);
         // }
         fscanf(Thresh_fp, "%f %f %f %f", &thresh_n, &thresh_e, &thresh_s, &thresh_w);
            Agent[n].raw_thresh_north = thresh_n;
            Agent[n].raw_thresh_east = thresh_e;
            Agent[n].raw_thresh_south = thresh_s;
            Agent[n].raw_thresh_west = thresh_w;
         
      }
   else
      {
      printf(" Error(init_agent): Invalid value for Thresh_init: %lf\n",
		Thresh_init);
      return ERROR;
      }  /* else */

   // 20.04.17.ASW:  I don't think the following code segment is being
   // used any more since I set the max and min to 1.0 and 0.0 at the
   // start of this function, so I'm commenting it out for now.
//   // for all non-constant thresholds, scale the distribution to be
//   // within agent's raw_thresh_max and raw_thresh_min
//   if (Thresh_init > 1.0)
//      {
//      range_width = Agent[n].raw_thresh_max - Agent[n].raw_thresh_min;
//      Agent[n].raw_thresh_north = Agent[n].raw_thresh_north 
//                                * range_width + Agent[n].raw_thresh_min;
//      Agent[n].raw_thresh_south = Agent[n].raw_thresh_south 
//                                * range_width + Agent[n].raw_thresh_min;
//      Agent[n].raw_thresh_east = Agent[n].raw_thresh_east 
//                               * range_width + Agent[n].raw_thresh_min;
//      Agent[n].raw_thresh_west = Agent[n].raw_thresh_west 
//                               * range_width + Agent[n].raw_thresh_min;
//      }

#ifdef DEBUG
printf("---end init_raw_thresholds()---\n");
#endif
   return OK;
   }  /* init_raw_thresholds */

/********** init_dynamic_threshold_range_TD2 **********/
/* created:     20.04.16.ASW
   revised:     20.10.15.ASW   Removed code for TD3 and place in new function.
   parameters:  n       agent index number
   called by:   init_agent(), fxn.c
   actions:     if dynamic thresholds are set to (Thresh_dynamic = 2),
                initialize the threshold range for each agent.
                The minimum of the range is a uniform random value within 
                [0.0:0.49].  The maximum of the range is a uniform random 
                value within [0.51:1.0].
*/
int init_dynamic_threshold_range_TD2(int n)
   {
   double temp;
   double range;

#ifdef DEBUG
printf("---in init_dynamic_threshold_range_TD2()---\n");
#endif

   // initialize unique range for each agent by randomly generating a
   // min and max for the range
   Agent[n].raw_thresh_min_north = knuth_random() * 0.49;
   Agent[n].raw_thresh_min_south = knuth_random() * 0.49;
   Agent[n].raw_thresh_min_east = knuth_random() * 0.49;
   Agent[n].raw_thresh_min_west = knuth_random() * 0.49;
   Agent[n].raw_thresh_max_north = knuth_random() * 0.49 + 0.51;
   Agent[n].raw_thresh_max_south = knuth_random() * 0.49 + 0.51;
   Agent[n].raw_thresh_max_east = knuth_random() * 0.49 + 0.51;
   Agent[n].raw_thresh_max_west = knuth_random() * 0.49 + 0.51;

   if (Thresh_dynamic_init == 0)
      {
      // initialize threshold uniformly randomly within range
      range = Agent[n].raw_thresh_max_north - Agent[n].raw_thresh_min_north;
      Agent[n].raw_thresh_north = knuth_random() * range
                             + Agent[n].raw_thresh_min_north;
      range = Agent[n].raw_thresh_max_south - Agent[n].raw_thresh_min_south;
      Agent[n].raw_thresh_south = knuth_random() * range
                             + Agent[n].raw_thresh_min_south;
      range = Agent[n].raw_thresh_max_east - Agent[n].raw_thresh_min_east;
      Agent[n].raw_thresh_east = knuth_random() * range
                             + Agent[n].raw_thresh_min_east;
      range = Agent[n].raw_thresh_max_west - Agent[n].raw_thresh_min_west;
      Agent[n].raw_thresh_west = knuth_random() * range
                             + Agent[n].raw_thresh_min_west;
      }
   else if (Thresh_dynamic_init == 1)
      {
      Agent[n].raw_thresh_north = 
	(double)(Agent[n].raw_thresh_max_north + Agent[n].raw_thresh_min_north)/
	(double)2.0;
      Agent[n].raw_thresh_south =
	(double)(Agent[n].raw_thresh_max_south + Agent[n].raw_thresh_min_south)/
	(double)2.0;
      Agent[n].raw_thresh_east =
	(double)(Agent[n].raw_thresh_max_east + Agent[n].raw_thresh_min_east)/
	(double)2.0;
      Agent[n].raw_thresh_west =
	(double)(Agent[n].raw_thresh_max_west + Agent[n].raw_thresh_min_west)/
	(double)2.0;
      }
   else if (Thresh_dynamic_init == 2)
      {
      Agent[n].raw_thresh_north = 0.5;
      Agent[n].raw_thresh_south = 0.5;
      Agent[n].raw_thresh_east = 0.5;
      Agent[n].raw_thresh_west = 0.5;
      }
   else
      {
      printf(" Error(init_dynamic_threshold_range_TD2):");
      printf(" Invalid value for Thresh_dynamic_init: %d\n",
		Thresh_dynamic_init);
      }

#ifdef DEBUG
printf("---end init_dynamic_threshold_range_TD2()---\n");
#endif
   return OK;
   }  /* init_dynamic_threshold_range_TD2 */

/********** init_dynamic_threshold_range_TD3 **********/
/* created:     20.04.16.ASW
   parameters:  n       agent index number
   called by:   init_agent(), fxn.c
   actions:     if dynamic thresholds are set to (Thresh_dynamic = 3), 
                initialize the threshold range for each agent.
		Max and min are both uniformly random values within 0 and 1.
                Lower value is the min; higher value is the max.
*/
int init_dynamic_threshold_range_TD3(int n)
   {
   double temp;
   double range;

#ifdef DEBUG
printf("---in init_dynamic_threshold_range_TD3()---\n");
#endif

   // initialize unique range for each agent by randomly generating a
   // min and max for the range
   Agent[n].raw_thresh_min_north = knuth_random();
   Agent[n].raw_thresh_min_south = knuth_random();
   Agent[n].raw_thresh_min_east = knuth_random();
   Agent[n].raw_thresh_min_west = knuth_random();
   Agent[n].raw_thresh_max_north = knuth_random();
   Agent[n].raw_thresh_max_south = knuth_random();
   Agent[n].raw_thresh_max_east = knuth_random();
   Agent[n].raw_thresh_max_west = knuth_random();

   // make sure that the min of the range is less than the max of the range
   if (Agent[n].raw_thresh_min_north > Agent[n].raw_thresh_max_north)
      {
      temp = Agent[n].raw_thresh_min_north;
      Agent[n].raw_thresh_min_north = Agent[n].raw_thresh_max_north;
      Agent[n].raw_thresh_max_north = temp;
      }
   if (Agent[n].raw_thresh_min_south > Agent[n].raw_thresh_max_south)
      {
      temp = Agent[n].raw_thresh_min_south;
      Agent[n].raw_thresh_min_south = Agent[n].raw_thresh_max_south;
      Agent[n].raw_thresh_max_south = temp;
      }
   if (Agent[n].raw_thresh_min_east > Agent[n].raw_thresh_max_east)
      {
      temp = Agent[n].raw_thresh_min_east;
      Agent[n].raw_thresh_min_east = Agent[n].raw_thresh_max_east;
      Agent[n].raw_thresh_max_east = temp;
      }
   if (Agent[n].raw_thresh_min_west > Agent[n].raw_thresh_max_west)
      {
      temp = Agent[n].raw_thresh_min_west;
      Agent[n].raw_thresh_min_west = Agent[n].raw_thresh_max_west;
      Agent[n].raw_thresh_max_west = temp;
      }

   if (Thresh_dynamic_init == 0)
      {
      // initialize threshold uniformly randomly within range
      range = Agent[n].raw_thresh_max_north - Agent[n].raw_thresh_min_north;
      Agent[n].raw_thresh_north = knuth_random() * range
                             + Agent[n].raw_thresh_min_north;
      range = Agent[n].raw_thresh_max_south - Agent[n].raw_thresh_min_south;
      Agent[n].raw_thresh_south = knuth_random() * range
                             + Agent[n].raw_thresh_min_south;
      range = Agent[n].raw_thresh_max_east - Agent[n].raw_thresh_min_east;
      Agent[n].raw_thresh_east = knuth_random() * range
                             + Agent[n].raw_thresh_min_east;
      range = Agent[n].raw_thresh_max_west - Agent[n].raw_thresh_min_west;
      Agent[n].raw_thresh_west = knuth_random() * range
                             + Agent[n].raw_thresh_min_west;
      }
   else if (Thresh_dynamic_init == 1)
      {
      Agent[n].raw_thresh_north =
        (double)(Agent[n].raw_thresh_max_north + Agent[n].raw_thresh_min_north)/
        (double)2.0;
      Agent[n].raw_thresh_south =
        (double)(Agent[n].raw_thresh_max_south + Agent[n].raw_thresh_min_south)/
        (double)2.0;
      Agent[n].raw_thresh_east =
        (double)(Agent[n].raw_thresh_max_east + Agent[n].raw_thresh_min_east)/
        (double)2.0;
      Agent[n].raw_thresh_west =
        (double)(Agent[n].raw_thresh_max_west + Agent[n].raw_thresh_min_west)/
        (double)2.0;
      }
   else
      {
      printf(" Error(init_dynamic_threshold_range_TD3):");
      printf(" Invalid value for Thresh_dynamic_init: %d\n",
                Thresh_dynamic_init);
      }

#ifdef DEBUG
printf("---end init_dynamic_threshold_range_TD3()---\n");
#endif
   return OK;
   }  /* init_dynamic_threshold_range_TD3 */

/********** init_intensities **********/
/* created:     20.04.16.ASW moved from init_agent into this function
   parameters:  n       agent index number
   called by:   init_agent(), fxn.c
   actions:     initialize intensities for each agent.
*/
int init_intensities(int n)
   {

#ifdef DEBUG
printf("---in init_intensities()---\n");
#endif

   /* HDM; intensity variation; 2019.09.12 */
   if (Intensity_variation == 1) // for homogeneous ranges
      {
      // generate initial intensity values
      Agent[n].intensity_north = rand_agent_intensity();
      Agent[n].intensity_south = rand_agent_intensity();
      Agent[n].intensity_east = rand_agent_intensity();
      Agent[n].intensity_west = rand_agent_intensity();
      // all range min and max are global values when Intensity_variation is 1
      Agent[n].int_aging_min_n = Intensity_aging_min;
      Agent[n].int_aging_max_n = Intensity_aging_max;
      Agent[n].int_aging_min_e = Intensity_aging_min;
      Agent[n].int_aging_max_e = Intensity_aging_max;
      Agent[n].int_aging_min_s = Intensity_aging_min;
      Agent[n].int_aging_max_s = Intensity_aging_max;
      Agent[n].int_aging_min_w = Intensity_aging_min;
      Agent[n].int_aging_max_w = Intensity_aging_max;
      }
   /* LR; intensity range variation; 2020.02.01 */
   else if (Intensity_variation == 2) //for heterogeneous ranges
     {
     /* LR; intensity range variation; 2020.02.01 */
     /* Randomly initialize the intensity range for each task of the agent.
      * It is essentially the same code for each task, but creating
      * functions would make the code more complex and less readable.*/
       double radius;
       double center;

       // north intensity range
       if (Hetero_radius_distr == 1) {//1 = normal/gaussian
           // Hetero_radius_mu is the mean of the normal distribution.
           // Hetero_radius_std is the standard deviation of the distribution.
           // box_muller is random number chosen from the described num distr.
           // fabs() takes the absolute value to prevent negative float values.
           radius = fabs(box_muller(Hetero_radius_mu, Hetero_radius_std));
       } else if (Hetero_radius_distr == 2) {// 2 = uniform
           // Hetero_radius_min offsets the range
           // knuth_random() is a random number chosen from range [0,1]
           // (Hetero_radius_max - Hetero_radius_min) scales the size of the range.
           radius = Hetero_radius_min + knuth_random()
               * (Hetero_radius_max - Hetero_radius_min);
       }

       if (Hetero_center_distr == 1) {//1 = normal/gaussian
           // Hetero_center_mu is the mean of the normal distribution.
           // Hetero_center_std is the standard deviation of the distribution.
           // box_muller is random number chosen from the described num distr.
           // fabs() takes the absolute value to prevent negative float values.
           center = box_muller(Hetero_center_mu, Hetero_center_std);
       } else if (Hetero_center_distr == 2) {// 2 = uniform
           // Hetero_center_min offsets the range
           // knuth_random() is a random number chosen from range [0,1]
           // (Hetero_range_max - Hetero_range_min) scales the size of the range.
           center = Hetero_range_min + radius + knuth_random()
           * ((Hetero_range_max-Hetero_range_min) - 2 * radius);
       }

       // However the center and radius of the range are chosen,
       // the max of the range = center + radius and the min = center - radius
       Agent[n].int_aging_max_n = center + radius;
       Agent[n].int_aging_min_n = center - radius;

       //correct if normal sends min or max out of allowed range
       if (Agent[n].int_aging_max_n > Hetero_range_max) {
         Agent[n].int_aging_max_n = Hetero_range_max;
         if (Agent[n].int_aging_min_n > Agent[n].int_aging_max_n) {
           Agent[n].int_aging_min_n = Agent[n].int_aging_max_n - radius;
         }
       }
       if (Agent[n].int_aging_min_n < Hetero_range_min) {
         Agent[n].int_aging_min_n = Hetero_range_min;
         if (Agent[n].int_aging_max_n < Agent[n].int_aging_min_n) {
           Agent[n].int_aging_max_n = Hetero_range_min + radius;
         }
       }

       // the ranges for the other tasks work the same so fewer comments are needed:

       // south intensity range
       if (Hetero_radius_distr == 1) {//1 = normal/gaussian
           radius = fabs(box_muller(Hetero_radius_mu, Hetero_radius_std));
       } else if (Hetero_radius_distr == 2) {// 2 = uniform
           radius = Hetero_radius_min + knuth_random()
               * (Hetero_radius_max - Hetero_radius_min);
       }

       if (Hetero_center_distr == 1) {//1 = normal/gaussian
           center = box_muller(Hetero_center_mu, Hetero_center_std);
       } else if (Hetero_center_distr == 2) {// 2 = uniform
           center = Hetero_range_min + radius + knuth_random()
           * ((Hetero_range_max-Hetero_range_min) - 2 * radius);
       }
       Agent[n].int_aging_max_s = center + radius;
       Agent[n].int_aging_min_s = center - radius;
       //correct if normal sends min or max out of allowed range
       if (Agent[n].int_aging_max_s > Hetero_range_max) {
         Agent[n].int_aging_max_s = Hetero_range_max;
         if (Agent[n].int_aging_min_s > Agent[n].int_aging_max_s) {
           Agent[n].int_aging_min_s = Agent[n].int_aging_max_s - radius;
         }
       }
       if (Agent[n].int_aging_min_s < Hetero_range_min) {
         Agent[n].int_aging_min_s = Hetero_range_min;
         if (Agent[n].int_aging_max_s < Agent[n].int_aging_min_s) {
           Agent[n].int_aging_max_s = Hetero_range_min + radius;
         }
       }

       // east intensity range
       if (Hetero_radius_distr == 1) {//1 = normal/gaussian
           radius = fabs(box_muller(Hetero_radius_mu, Hetero_radius_std));
       } else if (Hetero_radius_distr == 2) {// 2 = uniform
           radius = Hetero_radius_min + knuth_random()
               * (Hetero_radius_max - Hetero_radius_min);
       }

       if (Hetero_center_distr == 1) {//1 = normal/gaussian
           center = box_muller(Hetero_center_mu, Hetero_center_std);
       } else if (Hetero_center_distr == 2) {// 2 = uniform
           center = Hetero_range_min + radius + knuth_random()
           * ((Hetero_range_max-Hetero_range_min) - 2 * radius);
       }
       Agent[n].int_aging_max_e = center + radius;
       Agent[n].int_aging_min_e = center - radius;
       //correct if normal sends min or max out of allowed range
       if (Agent[n].int_aging_max_e > Hetero_range_max) {
         Agent[n].int_aging_max_e = Hetero_range_max;
         if (Agent[n].int_aging_min_e > Agent[n].int_aging_max_e) {
           Agent[n].int_aging_min_e = Agent[n].int_aging_max_e - radius;
         }
       }
       if (Agent[n].int_aging_min_e < Hetero_range_min) {
         Agent[n].int_aging_min_e = Hetero_range_min;
         if (Agent[n].int_aging_max_e < Agent[n].int_aging_min_e) {
           Agent[n].int_aging_max_e = Hetero_range_min + radius;
         }
       }

       // west intensity range
       if (Hetero_radius_distr == 1) {//1 = normal/gaussian
           radius = fabs(box_muller(Hetero_radius_mu, Hetero_radius_std));
       } else if (Hetero_radius_distr == 2) {// 2 = uniform
           radius = Hetero_radius_min + knuth_random()
               * (Hetero_radius_max - Hetero_radius_min);
       }

       if (Hetero_center_distr == 1) {//1 = normal/gaussian
           center = box_muller(Hetero_center_mu, Hetero_center_std);
       } else if (Hetero_center_distr == 2) {// 2 = uniform
           center = Hetero_range_min + radius + knuth_random()
           * ((Hetero_range_max-Hetero_range_min) - 2 * radius);
       }
       Agent[n].int_aging_max_w = center + radius;
       Agent[n].int_aging_min_w = center - radius;
       //correct if normal sends min or max out of allowed range
       if (Agent[n].int_aging_max_w > Hetero_range_max) {
         Agent[n].int_aging_max_w = Hetero_range_max;
         if (Agent[n].int_aging_min_w > Agent[n].int_aging_max_w) {
           Agent[n].int_aging_min_w = Agent[n].int_aging_max_w - radius;
         }
       }
       if (Agent[n].int_aging_min_w < Hetero_range_min) {
         Agent[n].int_aging_min_w = Hetero_range_min;
         if (Agent[n].int_aging_max_w < Agent[n].int_aging_min_w) {
           Agent[n].int_aging_max_w = Hetero_range_min + radius;
         }
       }

      // Select an intensity value within this range
      // midpoint of range
      if(Intensity_distribution == 0)
         {
         Agent[n].intensity_north = (Agent[n].int_aging_min_n + Agent[n].int_aging_max_n)/2.0;
         Agent[n].intensity_south = (Agent[n].int_aging_min_s + Agent[n].int_aging_max_s)/2.0;
         Agent[n].intensity_east = (Agent[n].int_aging_min_e + Agent[n].int_aging_max_e)/2.0;
         Agent[n].intensity_west = (Agent[n].int_aging_min_w + Agent[n].int_aging_max_w)/2.0;
         }
      // uniform
      else
         {
         Agent[n].intensity_north = random_double(Agent[n].int_aging_min_n, Agent[n].int_aging_max_n);
         Agent[n].intensity_south = random_double(Agent[n].int_aging_min_s, Agent[n].int_aging_max_s);
         Agent[n].intensity_east  = random_double(Agent[n].int_aging_min_e, Agent[n].int_aging_max_e);
         Agent[n].intensity_west  = random_double(Agent[n].int_aging_min_w, Agent[n].int_aging_max_w);
         }
       
      #ifdef DEBUG_PRINT
      printf("agent: %d   intensity aging ranges:\tn=[%f, %f], s=[%f, %f], e=[%f, %f], w=[%f, %f]\n",
        n,  Agent[n].int_aging_min_n, Agent[n].int_aging_max_n,
            Agent[n].int_aging_min_s, Agent[n].int_aging_max_s,
            Agent[n].int_aging_min_e, Agent[n].int_aging_max_e,
            Agent[n].int_aging_min_w, Agent[n].int_aging_max_w);
      }
      #endif
  } 
  else if (Intensity_variation == 10)
      {
         //DANIEL'S CODE
         // printf("Loading Agent Threshold Values\n");
         // FILE *fp = fopen("thresholds.txt", "r");
         float inten_n, inten_e, inten_s, inten_w;

         // for(int i = 0; i<Pop_size; i++) {
         //    fscanf(fp, "%f %f %f %f", &thresh_n, &thresh_e, &thresh_s, &thresh_w);
         // }
         fscanf(Intensity_fp, "%f %f %f %f", &inten_n, &inten_e, &inten_s, &inten_w);
            Agent[n].intensity_north = inten_n;
            Agent[n].intensity_east = inten_e;
            Agent[n].intensity_south = inten_s;
            Agent[n].intensity_west = inten_w;
         
      }
      else
      {
      Agent[n].intensity_north = 1.0;
      Agent[n].intensity_south = 1.0;
      Agent[n].intensity_east = 1.0;
      Agent[n].intensity_west = 1.0;
      Agent[n].int_aging_min_n = 1.0;
      Agent[n].int_aging_max_n = 1.0;
      Agent[n].int_aging_min_e = 1.0;
      Agent[n].int_aging_max_e = 1.0;
      Agent[n].int_aging_min_s = 1.0;
      Agent[n].int_aging_max_s = 1.0;
      Agent[n].int_aging_min_w = 1.0;
      Agent[n].int_aging_max_w = 1.0;
      }

    /* HDM; 2019.09.19 */
    Agent[n].int_tot_n = 0.0;
    Agent[n].int_tot_s = 0.0;
    Agent[n].int_tot_e = 0.0;
    Agent[n].int_tot_w = 0.0;

    Agent[n].int_no_act_n = 0.0;
    Agent[n].int_no_act_s = 0.0;
    Agent[n].int_no_act_e = 0.0;
    Agent[n].int_no_act_w = 0.0;

    Agent[n].int_min_n = Agent[n].intensity_north;
    Agent[n].int_min_s = Agent[n].intensity_south;
    Agent[n].int_min_e = Agent[n].intensity_east;
    Agent[n].int_min_w = Agent[n].intensity_west;

    Agent[n].int_max_n = Agent[n].intensity_north;
    Agent[n].int_max_s = Agent[n].intensity_south;
    Agent[n].int_max_e = Agent[n].intensity_east;
    Agent[n].int_max_w = Agent[n].intensity_west;

    // printf("agent: %d   intensities[n s e w]: %lf  %lf  %lf  %lf\n", n, Agent[n].intensity_north,
    //                 Agent[n].intensity_south, Agent[n].intensity_east, Agent[n].intensity_west);

#ifdef DEBUG
printf("---end init_intensities()---\n");
#endif
   return OK;
   }  /* init_intensities */

static double direction_intensity_value(int n, int direction)
   {
   if (direction == 1) return Agent[n].intensity_north;
   if (direction == 2) return Agent[n].intensity_east;
   if (direction == 3) return Agent[n].intensity_south;
   if (direction == 4) return Agent[n].intensity_west;
   return 1.0;
   }

static double direction_intensity_min(int n, int direction)
   {
   if (direction == 1) return Agent[n].int_aging_min_n;
   if (direction == 2) return Agent[n].int_aging_min_e;
   if (direction == 3) return Agent[n].int_aging_min_s;
   if (direction == 4) return Agent[n].int_aging_min_w;
   return 1.0;
   }

static double direction_intensity_max(int n, int direction)
   {
   if (direction == 1) return Agent[n].int_aging_max_n;
   if (direction == 2) return Agent[n].int_aging_max_e;
   if (direction == 3) return Agent[n].int_aging_max_s;
   if (direction == 4) return Agent[n].int_aging_max_w;
   return 1.0;
   }

void sync_direction_intensities_from_tasks(int n)
   {
   if (Num_tasks >= 1)
      {
      Agent[n].intensity_north = Agent[n].intensity_task[1];
      Agent[n].int_aging_min_n = Agent[n].int_aging_min_task[1];
      Agent[n].int_aging_max_n = Agent[n].int_aging_max_task[1];
      }
   if (Num_tasks >= 2)
      {
      Agent[n].intensity_east = Agent[n].intensity_task[2];
      Agent[n].int_aging_min_e = Agent[n].int_aging_min_task[2];
      Agent[n].int_aging_max_e = Agent[n].int_aging_max_task[2];
      }
   if (Num_tasks >= 3)
      {
      Agent[n].intensity_south = Agent[n].intensity_task[3];
      Agent[n].int_aging_min_s = Agent[n].int_aging_min_task[3];
      Agent[n].int_aging_max_s = Agent[n].int_aging_max_task[3];
      }
   if (Num_tasks >= 4)
      {
      Agent[n].intensity_west = Agent[n].intensity_task[4];
      Agent[n].int_aging_min_w = Agent[n].int_aging_min_task[4];
      Agent[n].int_aging_max_w = Agent[n].int_aging_max_task[4];
      }
   }  /* sync_direction_intensities_from_tasks */

void init_agent_task_intensities(int n)
   {
   int task;

   for (task=0; task<=MAX_TASKS; task++)
      {
      Agent[n].intensity_task[task] = 0.0;
      Agent[n].int_min_task[task] = 0.0;
      Agent[n].int_max_task[task] = 0.0;
      Agent[n].int_aging_min_task[task] = 0.0;
      Agent[n].int_aging_max_task[task] = 0.0;
      Agent[n].int_tot_task[task] = 0.0;
      Agent[n].int_no_act_task[task] = 0.0;
      }

   for (task=1; task<=Num_tasks; task++)
      {
      if (task <= 4)
         {
         Agent[n].intensity_task[task] = direction_intensity_value(n, task);
         Agent[n].int_aging_min_task[task] = direction_intensity_min(n, task);
         Agent[n].int_aging_max_task[task] = direction_intensity_max(n, task);
         }
      else
         {
         Agent[n].intensity_task[task] = rand_agent_intensity();
         Agent[n].int_aging_min_task[task] = Intensity_aging_min;
         Agent[n].int_aging_max_task[task] = Intensity_aging_max;
         }
      Agent[n].int_min_task[task] = Agent[n].intensity_task[task];
      Agent[n].int_max_task[task] = Agent[n].intensity_task[task];
      }

   sync_direction_intensities_from_tasks(n);
   }  /* init_agent_task_intensities */
