/* File containing descriptions of types, typedefs, and structures
   used in this program.

   19.07.12.AW	Created.
*/

//#define DEBUG	1	/* notes when entering and exiting routines */
//#undef DEBUG

/********** error codes **********/
#define OK	0
#define ERROR	-1

/********** other defines **********/
#define INPUT_LINE_LEN 512
#define ENDOFFILE 59
#define MAX_TASKS 256
//#define MAX_VALUE 999999
//#define MIN_VALUE -999999

/********** typedefs **********/

/* structure for storing output file information.  Includes
   standard output files and additional ones that may be added
   at the start of a run.
*/
typedef struct
   {
   int on;              /* if on=1, print; if on=0, don't print */
   char *extension;
   char *filename;
   FILE *fp;
   }  OUTPUT_FILE;

/********** problem specific typedefs **********/

/* structure for storing general simulation information */
//typedef struct
//   {
//   int time;			/* current time step of simulation */
//
//   double width;		/* width of working environment */
//   double height;		/* height of working environment */
//				/* 0,0 is in top left corner */
//   }  SIM;

/* structure for storing target information */
typedef struct
   {
   char *abbrev;		/* abbreviation of path for output files */
//   double width;		/* width of working environment */
//   double height;		/* height of working environment */
				/* 0,0 is in bottom left corner */

   double x;			/* current x coordinate */
   double y;			/* current y coordinate */
   double length;		/* length traveled since start */
   double step_len;		/* length traveled in each step */

   /* WL; 2021.9.27 */
   /* New path generation variables */
   double t;			/* current t-value in parametric curve, differs from a simulation time step */
   double xOff;			/* current x offset when dealing with mult-stage path */
   double yOff;			/* current y offset when dealing with multi-stage path */

   // path specific variables
   char side[20];		/* for moving in square */
   double angle;		/* for random and sharp movement */
				/* also used for zigzag path */
   double change_probability;	/* for sharp movement */
   double change;		/* used for acceleration and deceleration */
				/* also used for scurve to give angle change */
   double amplitude;		/* for zigzag and scurve */
   double period;		/* for zigzag and scurve */
   int direction;		/* for zigzag, 1 = up, -1 = down */
				/* for scurve, 1 = above zero, -1 = below */
   }  TARGET;

/* structure for storing information on each agent */
typedef struct
   {
   int index;			/* name of agent */

   /* the raw thresholds are values between 0 and 1 */
   double raw_thresh_north;	/* threshold for tasks [0.0: 1.0] */
   double raw_thresh_south;	/* threshold for tasks [0.0: 1.0] */
   double raw_thresh_east;	/* threshold for tasks [0.0: 1.0] */
   double raw_thresh_west;	/* threshold for tasks [0.0: 1.0] */

   /* the actual thresholds are normalized to the Range */
   double thresh_north;		/* threshold for tasks [0.0: Range] */
   double thresh_south;		/* threshold for tasks [0.0: Range] */
   double thresh_east;		/* threshold for tasks [0.0: Range] */
   double thresh_west;		/* threshold for tasks [0.0: Range] */

   /* 200416.ASW:  Specifies range of initial thresholds for when */
   /* range is from 0.0 to 1.0,                                   */
   /* i.e. when Thresh_dynamic=0 and Thresh_dynamic=1             */
   double raw_thresh_max;
   double raw_thresh_min;

   /* 200416.ASW:  Specifies possible threshold ranges for dynamic */
   /* thresholds, i.e. when Thresh_dynamic=1 and Thresh_dynamic=2  */
   double raw_thresh_max_north;
   double raw_thresh_max_south;
   double raw_thresh_max_east;
   double raw_thresh_max_west;
   double raw_thresh_min_north;
   double raw_thresh_min_south;
   double raw_thresh_min_east;
   double raw_thresh_min_west;
   double thresh_max_north;
   double thresh_max_south;
   double thresh_max_east;
   double thresh_max_west;
   double thresh_min_north;
   double thresh_min_south;
   double thresh_min_east;
   double thresh_min_west;

   /* Multitask channel state. Task channels are independent abstract tasks. */
   double raw_thresh_task[MAX_TASKS + 1];
   double thresh_task[MAX_TASKS + 1];
   double raw_thresh_min_task[MAX_TASKS + 1];
   double raw_thresh_max_task[MAX_TASKS + 1];
   double thresh_min_task[MAX_TASKS + 1];
   double thresh_max_task[MAX_TASKS + 1];

   /* 220404.ASW: dynamic response probabilities unique to each task.
      0 is idle; task channels are 1..Num_tasks. */
   double resprob[MAX_TASKS + 1];
   double resprob_min[MAX_TASKS + 1];
   double resprob_max[MAX_TASKS + 1];

   /* Experimental controller heterogeneity: one PID triplet per agent,
      shared across that agent's task channels. */
   double agent_P_gain;
   double agent_D_gain;
   double agent_I_gain;

   int chosen;			/* agent chosen for this timestep, 0=no 1=yes */
   int current_task;		/* task agent takes in current timestep; 0=idle */
   int previous_task;		/* task from previous timestep */
   int previous_task_noidle;	/* previous non-idle task */

   int count_multi_tasks;   /* count of times multiple thresholds met */
    
   /* keep track of number of times agent acts on each task */
   int count_north;
   int count_east;
   int count_south;
   int count_west;
   int count_idle;
   int count_task[MAX_TASKS + 1];

   /* keep track of number of times agent switches tasks */
   int count_switch;
   int count_switch_noidle;	// does not count idle as a task, so switches
				// to and from idle don't count
   int count_switch_spontaneous;  // NB; Spontaneous Response Prob; 2020.06.03

   /* HDM; intensity variation; 2019.09.12 */
   /* agent's intensity factors for each task */
   /* bounds defined in params */
   double intensity_north;
   double intensity_east;
   double intensity_south;
   double intensity_west;
   double intensity_task[MAX_TASKS + 1];
   double int_min_task[MAX_TASKS + 1];
   double int_max_task[MAX_TASKS + 1];
   double int_aging_min_task[MAX_TASKS + 1];
   double int_aging_max_task[MAX_TASKS + 1];
   double int_tot_task[MAX_TASKS + 1];
   double int_no_act_task[MAX_TASKS + 1];

   /* HDM; 2019.09.19 */
   /* total intensity for agent when pushing, by direction */
   double int_tot_n;
   double int_tot_e;
   double int_tot_s;
   double int_tot_w;

   double int_no_act_n;
   double int_no_act_e;
   double int_no_act_s;
   double int_no_act_w;

   double int_min_n;
   double int_min_e;
   double int_min_s;
   double int_min_w;

   double int_max_n;
   double int_max_e;
   double int_max_s;
   double int_max_w;

   /* LR; 2020.01.29 */
   /* agent's intensity range */
   double int_aging_max_n;
   double int_aging_max_e;
   double int_aging_max_s;
   double int_aging_max_w;

   double int_aging_min_n;
   double int_aging_min_e;
   double int_aging_min_s;
   double int_aging_min_w;
 
   double prob_check;		/* prob agent choose task each timestep */
//rp 220521ASW  Remove old response probability variable now that
//rp dynamic one has been added (resprob)
//rp   double response_prob;    /* for heterogeneous response probabilities */
   double spontaneous_response_prob;   /* for heterogeneous spontaneous response probabilities */
    
   /* HDM; 2020.03.19 */
   int dead;                /* indicates if agent has been killed; values 0 or 1 */
   int time_killed;         /* time step agent was killed */
    
   }  AGENT;

/* structure for storing information on swarm */
typedef struct
   {
   double step_ratio;		/* ratio of swarm-step-len/target-step-len */

   double x;			/* current x location */
   double y;			/* current y location */
   double length;		/* length traveled since start */
   double current_step_len;	/* length of current step */
   double max_step_len;		/* max length travel-able in each step */
   double total_difference;	/* sum of all distances from all steps */
   double max_difference;
   double min_difference;
   double total_pre_service_difference;
   double max_pre_service_difference;
   double min_pre_service_difference;

   /* True post-service metrics for the confirmatory paired-task runs. */
   double metric_sum_R;
   double metric_sum_R_abs;
   double metric_sum_R2;
   double metric_sum_R2_norm;
   double metric_max_R2_norm;
   double post_removal_sum_R;
   double post_removal_sum_R_abs;
   double post_removal_sum_R2_norm;
   double post_removal_max_R2_norm;
   int post_removal_steps;

   /* data per step */
   double difference;		/* distance to target in each step */
   double pre_service_difference;
   double diff_to_north;	/* how far north of tracker target is */
   double diff_to_south;	/* how far south of tracker target is */
   double diff_to_east;		/* how far east of tracker target is */
   double diff_to_west;		/* how far west of tracker target is */

   double prev_diff_to_north;	/* how far north of tracker target is */
   double prev_diff_to_south;	/* how far south of tracker target is */
   double prev_diff_to_east;		/* how far east of tracker target is */
   double prev_diff_to_west;		/* how far west of tracker target is */


   double sum_diff_to_north;	/* how far north of tracker target is */
   double sum_diff_to_south;	/* how far south of tracker target is */
   double sum_diff_to_east;		/* how far east of tracker target is */
   double sum_diff_to_west;		/* how far west of tracker target is */
     
   int time;
	/* how far south of tracker target is */
   		/* how far west of tracker target is */

   int push_north;		/* number of agents pushing north */
   int push_south;		/* number of agents pushing south */
   int push_east;		/* number of agents pushing east */
   int push_west;		/* number of agents pushing west */

   /* 201204-ASW track # switches and pct actors that switched per timestep */
   int num_switch;		/* number of switches per timestep */
   int num_prev_actors;		/* number of actors in previous timestep */
   double pct_actors_switch;	/* pct actors that switched tasks */
				/* switch_count/num_prev_actors */
   double pct_all_switch;	/* pct of all population that switched tasks */
   // same as switch but does not count idle as a task, so switches
   // to and from idle don't count
   int num_switch_noidle;

   // HDM; intensity variation; 2019.09.12
   double intensity_push_north;  /* sum of north intensities of agents pushing north */
   double intensity_push_south;  /* sum of south intensities of agents pushing south */
   double intensity_push_east;   /* sum of east intensities of agents pushing east */
   double intensity_push_west;   /* sum of west intensities of agents pushing west */

   double intensity_all_north;   /* sum of north intensities of all agents */
   double intensity_all_south;   /* sum of south intensities of all agents */
   double intensity_all_east;    /* sum of east intensities of all agents */
   double intensity_all_west;    /* sum of west intensities of all agents */

   // HDM; intensity variation; 2019.10.10
   /*
    arrays for tracking agent intensities in a given timestep.
    The intensity range is divided into bins of size 0.1.
    Each array element stores the count of intensities in that bin.
   */
//   int intensity_north_dist[10];
//   int intensity_south_dist[10];
//   int intensity_east_dist[10];
//   int intensity_west_dist[10];
   int* intensity_north_dist;
   int* intensity_south_dist;
   int* intensity_east_dist;
   int* intensity_west_dist;

   // be able to choose direct comparison or Bonabeau response threshold

   // statistics about swarm, e.g. who acted, how many acted, pool size,
   // last agent (highest threshold) acted
   }  TRACKER;
