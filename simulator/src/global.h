/* global.h
   19.07.11.AW	Created.
		File of global variables included in main.c.
		All other .c files should include extern.h instead.
*/

/********** run parameters read in **********/
int Rerun;
char *Run_num_file;
char *Output_path;
int Print_params;               /* if 1, print params at start of run */
int Print_step;                 /* if 1, print status each timestep */

int Max_steps;			/* max timesteps to run simulation */
int Pop_size;
double Thresh_init;		/* initialize thresholds for static and homogen dynamic */
int Thresh_dynamic;		/* turn on/off dynamic thresholds */
int Thresh_dynamic_init;	/* how to initialize dynamic thresholds */
double Thresh_increase;
double Thresh_decrease;
double Prob_check;
char *Target_path;
double Circle_radius;		/* if Target_path is circle, specify radius */
double Edge_length;        /* if Target_path is square or diamond, specify edge length */
double Range;			/* radius around zero of ok values */
   /* Range gives the range of the distribution of values */
int Gnuplot_plots;		/* automatically generate plots */
int Animate_thresh;		/* Flag to turn on printing of files for      */
				/* animating dynamic thresholds.  Will result */
				/* in creation of files not specified in      */
				/* opfiles.default and gnu.c.  All related    */
				/* code in animate.c                          */
int Animate_stepwise;		/* flag for allowing user to step thru */
				/* animation manually                  */
double P_gain;
double D_gain;
double I_gain;
int Pid;                          /* 0=LFTA, 1=PTA, 2=SETA, 3=SBTA, 4=CT */
int Pid_latent_thresholds;        /* 1=PTA/SETA keeps latent thresholds outside min/max */
double Pid_integral_leak;         /* leaky integrator decay per step, e.g. 0.99 */
double Pid_integral_bound;        /* symmetric integral clamp; <=0 disables */
int Agent_pid_gains;              /* 1=sample per-agent PID gain multipliers */
double Agent_pid_gain_spread;     /* legacy single spread alias */
double Agent_pid_p_spread;        /* log2 spread for per-agent P gain */
double Agent_pid_i_spread;        /* log2 spread for per-agent I gain */
double Agent_pid_d_spread;        /* log2 spread for per-agent D gain */
int Agent_pid_gain_apply_id;      /* legacy: 1=apply spread to I and D as well as P */
int Feedback_noise_enabled;        /* 1=perturb task-error feedback used by PID */
double Feedback_noise_alpha;       /* sigma multiplier for feedback noise */
long Feedback_noise_seed;          /* paired seed for deterministic feedback noise */
int Feedback_noise_clip;           /* 1=clip noisy feedback to [-Pop_size, Pop_size] */
double Feedback_bias_alpha;        /* fixed task-bias multiplier for feedback error */
long Feedback_bias_seed;           /* paired seed for deterministic task-bias signs */
int Feedback_bias_mode;            /* 0=none, 1=fixed random task signs, 2=positive */
/********** parameters read in from elsewhere **********/
int Run_num;                    /* read in from Run_num_file */
int Max_num_output_files;       /* read in from opfiles.default */
long Seed;                      /* random seed, read in or generated */

/********** array of output files **********/
OUTPUT_FILE *Output_file;

/********** simulation global variables **********/

AGENT *Agent;
TRACKER Tracker;
TARGET Target;
int Num_tasks;
char *Task_demand_pattern;
int Task_opposition_mode;              /* 0=relative task channels, 1=paired opposites */
double Task_demand[MAX_TASKS + 1];      /* current workload stimulus for each task channel */
double Task_prev_demand[MAX_TASKS + 1];
double Task_error_integral[MAX_TASKS + 1];
double Task_arrival[MAX_TASKS + 1];
double Task_total_arrival[MAX_TASKS + 1];
double Task_service[MAX_TASKS + 1];
double Task_total_service[MAX_TASKS + 1];
double Task_target_vector[MAX_TASKS + 1];
double Task_tracker_vector[MAX_TASKS + 1];
double Task_signed_error[MAX_TASKS + 1];
double Task_feedback_error[MAX_TASKS + 1];
double Task_prev_feedback_error[MAX_TASKS + 1];
int Task_actor_count[MAX_TASKS + 1];
int Num_alive;                  /* number of agents alive - initially Pop_size */
int Current_timestep;           /* timestep currently being evaluated */

/* HDM; intensity variation; 2019.09.12 */
/*********** variations being used ***************/
int Intensity_variation;
int Intensity_aging;
double Intensity_aging_min;
double Intensity_aging_max;
double Intensity_aging_up;
double Intensity_aging_down;
int Intensity_distribution;
/* LR; heterogeneous intensity ranges; 2020.02.08 */
int Hetero_center_distr;
int Hetero_radius_distr;
/* 2020.02.15 */
double Hetero_center_mu;
double Hetero_center_std;
double Hetero_radius_mu;
double Hetero_radius_std;
/* 2020.02.27 */
double Hetero_range_max;
double Hetero_range_min;
double Hetero_radius_max;
double Hetero_radius_min;

/* HDM; intensity variation; 2019.10.24 */
//rp 220521ASW replace old RP with dynamic response probability
//rp double Response_prob;
/* NB; Response_prob standard distribution; 2020.07.06 */
double RP_gaussian_mu;
double RP_gaussian_std; 

/* HDM; related to response probability; 2020.03.19 */
int Kill_number;
int First_extinction;
int Extinction_period;
int Removal_capacity_mode;      /* 0=survivor-normalized service, 1=capacity loss using original Pop_size */

/* NB; Spontaneous Response Probability; 2020.05.19 */
double Spontaneous_response_prob;
/* NB; Spontaneous_response_prob standard distribution; 2020.07.06 */
double SRP_gaussian_mu;
double SRP_gaussian_std; 

// not currently used
char *Task_selection;

/* ASW 22.04.04: individual response probability for each task and dynamic RP */
int Prob_dynamic;		// 0=static, 1=dynamic, indv task RP
double Prob_dynamic_init;	// <=1.0 homogeneous, 2.0 uniform between
				// Prob_dynamic_min and Prob_dynamic_max
double Prob_increase;		// how much to increase if act on task
double Prob_decrease;		// how much to decrease if did not act on task
double Prob_idle_increase;	// how much to increase if idle
double Prob_dynamic_min;	// minimum possible response probability value
double Prob_dynamic_max;	// maximum possible response probability value

int FixedTargetPath = 1; // 1 = fixed target path each run, 0 = random
unsigned long TargetPathSeed = 42; // Default seed for target path
unsigned long TargetPathState = 1; // State for target path RNG

/* Demand-oriented abstract task paths. random gives each task channel its
   own stochastic arrivals; cyclic rotates arrivals across task ids. The
   multitask tracker is a task-space service vector, not a 2D direction map. */
int Demand_segment_len;   /* periodic_switch: steps per high-demand phase (default 200) */
int Demand_switch_step;   /* single_switch: first timestep t using phase B (default 500 → A for t<500, B for t>=500) */
