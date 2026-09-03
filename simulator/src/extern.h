/* extern.h
   91.07.11.AW	Created.
*/

extern int Rerun;
extern char *Run_num_file;
extern char *Output_path;
extern int Print_params;
extern int Print_step;
extern int Max_steps;
extern int Pop_size;
extern double Thresh_init;
extern int Thresh_dynamic;
extern int Thresh_dynamic_init;
extern double Thresh_increase;
extern double Thresh_decrease;
extern double Prob_check;
extern char *Target_path;
extern double Circle_radius;
extern double Edge_length;
extern double Range;
extern int Gnuplot_plots;
extern int Animate_thresh;
extern int Animate_stepwise;
extern int Run_num;
extern int Max_num_output_files;
extern long Seed;
extern OUTPUT_FILE *Output_file;
extern AGENT *Agent;
extern TRACKER Tracker;
extern TARGET Target;
extern int Num_tasks;
extern char *Task_demand_pattern;
extern int Task_opposition_mode;
extern double Task_demand[MAX_TASKS + 1];
extern double Task_prev_demand[MAX_TASKS + 1];
extern double Task_error_integral[MAX_TASKS + 1];
extern double Task_arrival[MAX_TASKS + 1];
extern double Task_total_arrival[MAX_TASKS + 1];
extern double Task_service[MAX_TASKS + 1];
extern double Task_total_service[MAX_TASKS + 1];
extern double Task_target_vector[MAX_TASKS + 1];
extern double Task_tracker_vector[MAX_TASKS + 1];
extern double Task_signed_error[MAX_TASKS + 1];
extern double Task_feedback_error[MAX_TASKS + 1];
extern double Task_prev_feedback_error[MAX_TASKS + 1];
extern int Task_actor_count[MAX_TASKS + 1];
extern char *Task_selection;
extern int Intensity_variation;     /* HDM; intensity variation; 2019.09.12 */
extern int Intensity_aging;         /* HDM; intensity variation; 2019.09.19 */
extern double Intensity_aging_min;  /* HDM; intensity variation; 2019.09.19 */
extern double Intensity_aging_max;  /* HDM; intensity variation; 2019.09.19 */
extern double Intensity_aging_up;   /* HDM; intensity variation; 2019.09.19 */
extern double Intensity_aging_down; /* HDM; intensity variation; 2019.09.19 */
extern int Intensity_distribution;  /* HDM; intensity variation; 2019.10.14 */
extern int Hetero_center_distr;     /* LR; heterogeneous intensity ranges; 2020.02.08 */
extern int Hetero_radius_distr;     /* LR; heterogeneous intensity ranges; 2020.02.08 */
extern double Hetero_center_mu;     /* LR; heterogeneous intensity ranges; 2020.02.15 */
extern double Hetero_center_std;    /* LR; heterogeneous intensity ranges; 2020.02.15 */
extern double Hetero_radius_mu;     /* LR; heterogeneous intensity ranges; 2020.02.15 */
extern double Hetero_radius_std;    /* LR; heterogeneous intensity ranges; 2020.02.15 */
extern double Hetero_range_max;     /* LR; heterogeneous intensity ranges; 2020.02.27 */
extern double Hetero_range_min;     /* LR; heterogeneous intensity ranges; 2020.02.27 */
extern double Hetero_radius_max;    /* LR; heterogeneous intensity ranges; 2020.02.27 */
extern double Hetero_radius_min;    /* LR; heterogeneous intensity ranges; 2020.02.27 */
//rp extern double Response_prob;        /* HDM; variable response prob; 2019.10.24 */
extern int Kill_number;             /* HDM; related to response prob; 2020.03.19 */
extern int First_extinction;        /* HDM; time step at which agents are first killed */
extern int Extinction_period;       /* HDM; period at which extinctions occur */
extern int Num_alive;               /* HDM; number of agents still alive */
extern int Removal_capacity_mode;   /* 0=survivor-normalized service, 1=capacity loss using original Pop_size */
extern double Spontaneous_response_prob; /* NB; variable spontaneous response when response threshold not met;
                                            2020.05.19; */
extern double SRP_gaussian_mu;      /* NB; Spontaneous_response_prob standard distribution; 2020.07.06 */
extern double SRP_gaussian_std; 
extern double RP_gaussian_mu;       /* NB; Response_prob standard distribution; 2020.07.06 */
extern double RP_gaussian_std; 

extern int Prob_dynamic;
extern double Prob_dynamic_init;
extern double Prob_increase;
extern double Prob_decrease;
extern double Prob_idle_increase;
extern double Prob_dynamic_min;
extern double Prob_dynamic_max;
extern double P_gain;
extern double D_gain;
extern double I_gain;
extern int Pid;
extern int Pid_latent_thresholds;
extern double Pid_integral_leak;
extern double Pid_integral_bound;
extern int Agent_pid_gains;
extern double Agent_pid_gain_spread;
extern double Agent_pid_p_spread;
extern double Agent_pid_i_spread;
extern double Agent_pid_d_spread;
extern int Agent_pid_gain_apply_id;
extern int Feedback_noise_enabled;
extern double Feedback_noise_alpha;
extern long Feedback_noise_seed;
extern int Feedback_noise_clip;
extern double Feedback_bias_alpha;
extern long Feedback_bias_seed;
extern int Feedback_bias_mode;

extern int FixedTargetPath; // 0 = random target path each run, 1 = fixed
extern unsigned long TargetPathSeed; // Seed for target path if fixed
extern unsigned long TargetPathState; // State for target path RNG
extern int Demand_segment_len;
extern int Demand_switch_step;
extern int Current_timestep;
