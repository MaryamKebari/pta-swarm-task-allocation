/* output.c
   08.05.26.AW	Created.
		Contains routines that output to screen or file.
		General routines, not problem specific.

   Routines:	print_params()		06.21.98.AW
		print_opfiles()		07.08.98.AW
		gen_output()		07.29.98.AW
		run_output()		07.29.98.AW
*/

//#define DEBUG 1

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "types.h"
#include "extern.h"
#include "output.h"
#include "params.h"
#include "gnu.h"
#include "fxn.h"
#include "animate.h"

// #define HISTOGRAM_SIZE ((Intensity_aging_max - Intensity_aging_min) * 10)
#define HISTOGRAM_SIZE 10

static void fprint_task_csv_header(FILE *fp, const char *prefix)
   {
   int task;
   for (task=1; task<=Num_tasks; task++)
      fprintf(fp, ",%s_task%d", prefix, task);
   }

static void fprint_agent_csv_header(FILE *fp)
   {
   int agent;
   fprintf(fp, "timestep");
   for (agent=0; agent<Pop_size; agent++)
      fprintf(fp, ",agent%d", agent);
   fprintf(fp, "\n");
   }

static void strip_newline(char *line)
   {
   int i;
   for (i=0; line[i] != '\0'; i++)
      {
      if (line[i] == '\n' || line[i] == '\r')
         {
         line[i] = '\0';
         return;
         }
      }
   }

static char *csv_after_first_field(char *line)
   {
   char *ptr;
   for (ptr=line; *ptr != '\0'; ptr++)
      if (*ptr == ',') return ptr + 1;
   return ptr;
   }

static void create_stepsummary_cleaned_file(char *output_filename)
   {
   char stepsummary_filename[INPUT_LINE_LEN];
   char taskdemand_filename[INPUT_LINE_LEN];
   char taskcounts_filename[INPUT_LINE_LEN];
   char taskthresh_filename[INPUT_LINE_LEN];
   char line[65536];
   char task_line[65536];
   FILE *stepsummary_fp;
   FILE *output_fp;
   FILE *taskdemand_fp;
   FILE *taskcounts_fp;
   FILE *taskthresh_fp;

   int timestep;
   int actors;
   int idle;
   int num_switch;
   int num_switch_noidle;
   double target_vector_norm;
   double tracker_vector_norm;
   double pre_service_residual_norm;
   double residual_norm;
   double step_arrival_sum;
   double step_service_sum;
   double demand_sum;
   double pct_actors_switch;
   double pct_all_switch;
   double actor_fraction;
   double service_fraction;

   sprintf(stepsummary_filename, "%s/run.%d/run.%d.stepsummary",
           Output_path, Run_num, Run_num);
   sprintf(taskdemand_filename, "%s/run.%d/run.%d.steptaskdemand",
           Output_path, Run_num, Run_num);
   sprintf(taskcounts_filename, "%s/run.%d/run.%d.steptaskcounts",
           Output_path, Run_num, Run_num);
   sprintf(taskthresh_filename, "%s/run.%d/run.%d.steptaskthresh",
           Output_path, Run_num, Run_num);

   stepsummary_fp = fopen(stepsummary_filename, "r");
   if (stepsummary_fp == NULL)
      {
      printf("WARNING: could not open %s to create cleaned stepsummary\n",
             stepsummary_filename);
      return;
      }

   output_fp = fopen(output_filename, "w");
   if (output_fp == NULL)
      {
      printf("WARNING: could not open %s for cleaned stepsummary\n",
             output_filename);
      fclose(stepsummary_fp);
      return;
      }

   taskdemand_fp = fopen(taskdemand_filename, "r");
   taskcounts_fp = fopen(taskcounts_filename, "r");
   taskthresh_fp = fopen(taskthresh_filename, "r");

   fprintf(output_fp,
           "timestep,target_vector_norm,tracker_vector_norm,pre_service_residual_norm,residual_norm,"
           "step_arrival_sum,step_service_sum,demand_sum,actors,idle,"
           "actor_fraction,service_fraction,num_switch,pct_actors_switch,"
           "pct_all_switch,num_switch_noidle");

   if (fgets(line, sizeof(line), stepsummary_fp) == NULL)
      {
      fclose(stepsummary_fp);
      fclose(output_fp);
      if (taskdemand_fp != NULL) fclose(taskdemand_fp);
      if (taskcounts_fp != NULL) fclose(taskcounts_fp);
      if (taskthresh_fp != NULL) fclose(taskthresh_fp);
      return;
      }

   if (taskdemand_fp != NULL && fgets(task_line, sizeof(task_line), taskdemand_fp) != NULL)
      {
      strip_newline(task_line);
      fprintf(output_fp, ",%s", csv_after_first_field(task_line));
      }
   else if (taskdemand_fp != NULL)
      {
      fclose(taskdemand_fp);
      taskdemand_fp = NULL;
      }

   if (taskcounts_fp != NULL && fgets(task_line, sizeof(task_line), taskcounts_fp) != NULL)
      {
      strip_newline(task_line);
      fprintf(output_fp, ",%s", csv_after_first_field(task_line));
      }
   else if (taskcounts_fp != NULL)
      {
      fclose(taskcounts_fp);
      taskcounts_fp = NULL;
      }

   if (taskthresh_fp != NULL && fgets(task_line, sizeof(task_line), taskthresh_fp) != NULL)
      {
      strip_newline(task_line);
      fprintf(output_fp, ",%s", csv_after_first_field(task_line));
      }
   else if (taskthresh_fp != NULL)
      {
      fclose(taskthresh_fp);
      taskthresh_fp = NULL;
      }

   fprintf(output_fp, "\n");

   while (fgets(line, sizeof(line), stepsummary_fp) != NULL)
      {
      if (sscanf(line, "%d,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%d,%d,%d,%lf,%lf,%d",
                 &timestep, &target_vector_norm, &tracker_vector_norm,
                 &pre_service_residual_norm, &residual_norm,
                 &step_arrival_sum, &step_service_sum,
                 &demand_sum, &actors, &idle, &num_switch,
                 &pct_actors_switch, &pct_all_switch,
                 &num_switch_noidle) != 14)
         continue;

      actor_fraction = (actors + idle > 0)
                         ? (double)actors / (double)(actors + idle)
                         : 0.0;
      service_fraction = (step_arrival_sum > 0.0)
                           ? step_service_sum / step_arrival_sum
                           : 0.0;

      fprintf(output_fp,
              "%d,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%d,%d,%lf,%lf,%d,%lf,%lf,%d",
              timestep, target_vector_norm, tracker_vector_norm,
              pre_service_residual_norm, residual_norm,
              step_arrival_sum, step_service_sum,
              demand_sum, actors, idle, actor_fraction, service_fraction,
              num_switch, pct_actors_switch, pct_all_switch,
              num_switch_noidle);

      if (taskdemand_fp != NULL && fgets(task_line, sizeof(task_line), taskdemand_fp) != NULL)
         {
         strip_newline(task_line);
         fprintf(output_fp, ",%s", csv_after_first_field(task_line));
         }
      if (taskcounts_fp != NULL && fgets(task_line, sizeof(task_line), taskcounts_fp) != NULL)
         {
         strip_newline(task_line);
         fprintf(output_fp, ",%s", csv_after_first_field(task_line));
         }
      if (taskthresh_fp != NULL && fgets(task_line, sizeof(task_line), taskthresh_fp) != NULL)
         {
         strip_newline(task_line);
         fprintf(output_fp, ",%s", csv_after_first_field(task_line));
         }

      fprintf(output_fp, "\n");
      }

   fclose(stepsummary_fp);
   fclose(output_fp);
   if (taskdemand_fp != NULL) fclose(taskdemand_fp);
   if (taskcounts_fp != NULL) fclose(taskcounts_fp);
   if (taskthresh_fp != NULL) fclose(taskthresh_fp);
   }

static void create_stepsummary_cleaned()
   {
   char output_filename[INPUT_LINE_LEN];

   sprintf(output_filename, "%s/run.%d/run.%d.stepsummary_cleaned.csv",
           Output_path, Run_num, Run_num);
   create_stepsummary_cleaned_file(output_filename);

   sprintf(output_filename, "%s/run.%d/run.%d_stepsummary_cleaned.csv",
           Output_path, Run_num, Run_num);
   create_stepsummary_cleaned_file(output_filename);
   }



/********** print_params **********/
/* parameters:	fp	where to print, includes stdout (screen)
			Assume that fp has already been fopened.
   called by:	read_params(), params.c
		ga_init(), ga.c
   actions:	prints out the values of the parameters for a particular
		run.  Values to be printed (currently) must be defined in
		global.h.
*/
void print_params(FILE *fp)
   {
  /* things that were read in */
   fprintf(fp, " Run_num = %d\n", Run_num);
   fprintf(fp, " Seed = %ld\n", Seed);
   fprintf(fp, " Run_num_file = %s\n", Run_num_file);
   fprintf(fp, " Rerun = %d\n", Rerun);
   fprintf(fp, " Output_path = %s\n", Output_path);
   fprintf(fp, " Max_steps = %d\n", Max_steps);
   fprintf(fp, " Print_params = %d\n", Print_params);
   fprintf(fp, " Print_step = %d\n", Print_step);
   fprintf(fp, " Pop_size = %d\n", Pop_size);
   fprintf(fp, " Num_tasks = %d\n", Num_tasks);
   fprintf(fp, " Task_demand_pattern = %s\n", Task_demand_pattern);
   fprintf(fp, " Task_opposition_mode = %d\n", Task_opposition_mode);
   fprintf(fp, " Thresh_init = %lf\n", Thresh_init);
   fprintf(fp, " Thresh_dynamic = %d\n", Thresh_dynamic);
   fprintf(fp, " Thresh_dynamic_init = %d\n", Thresh_dynamic_init);
   fprintf(fp, " Thresh_increase = %lf\n", Thresh_increase);
   fprintf(fp, " Thresh_decrease = %lf\n", Thresh_decrease);
   fprintf(fp, " Animate_thresh = %d\n", Animate_thresh);
   fprintf(fp, " Animate_stepwise = %d\n", Animate_stepwise);
   fprintf(fp, " Prob_check = %lf\n", Prob_check);
//rp  220521ASW replace with dynamic response probability
//rp   fprintf(fp, " Response_prob = %lf\n", Response_prob);
   fprintf(fp, " RP_gaussian_mu = %lf\n", RP_gaussian_mu);
   fprintf(fp, " RP gaussian_std = %lf\n", RP_gaussian_std);
   fprintf(fp, " Task_selection = %s\n", Task_selection);
   fprintf(fp, " Target_path = %s\n", Target_path);
   fprintf(fp, " Circle_radius = %lf\n", Circle_radius);
   fprintf(fp, "Edge_length = %lf\n", Edge_length);
   fprintf(fp, " Target.amplitude = %lf\n", Target.amplitude);
   fprintf(fp, " Target.period = %lf\n", Target.period);
   fprintf(fp, " Target.step_len = %lf\n", Target.step_len);
   fprintf(fp, " Range = %lf\n", Range);
   fprintf(fp, " Step_ratio = %lf\n", Tracker.step_ratio);
   /* HDM; intensity variation; 2019.09.12 */
   fprintf(fp, " Intensity_variation = %d\n", Intensity_variation);
   /* HDM; intensity variation; 2019.09.19 */
   fprintf(fp, " Intensity_aging = %d\n", Intensity_aging);
   fprintf(fp, " Intensity_aging_min = %lf\n", Intensity_aging_min);
   fprintf(fp, " Intensity_aging_max = %lf\n", Intensity_aging_max);
   fprintf(fp, " Intensity_aging_up = %lf\n", Intensity_aging_up);
   fprintf(fp, " Intensity_aging_down = %lf\n", Intensity_aging_down);
   fprintf(fp, " Intensity_distribution = %d\n", Intensity_distribution);
   /* LR; heterogeneous intensity ranges; 2020.02.08 */
   fprintf(fp, " Hetero_center_distr = %d\n", Hetero_center_distr);
   fprintf(fp, " Hetero_radius_distr = %d\n", Hetero_radius_distr);
   /* LR; heterogeneous intensity ranges; 2020.02.27 */
   fprintf(fp, " Hetero_range_max = %lf\n", Hetero_range_max);
   fprintf(fp, " Hetero_range_min = %lf\n", Hetero_range_min);
   fprintf(fp, " Hetero_radius_max = %lf\n", Hetero_radius_max);
   fprintf(fp, " Hetero_radius_min = %lf\n", Hetero_radius_min);
   /* LR; heterogeneous intensity ranges; 2020.02.15 */
   fprintf(fp, " Hetero_center_mu = %lf\n", Hetero_center_mu);
   fprintf(fp, " Hetero_center_std = %lf\n", Hetero_center_std);
   fprintf(fp, " Hetero_radius_mu = %lf\n", Hetero_radius_mu);
   fprintf(fp, " Hetero_radius_std = %lf\n", Hetero_radius_std);
   /* HDM; response variation probability; 2019.10.24 */
   fprintf(fp, " Kill_number = %d\n", Kill_number);
   fprintf(fp, " First_extinction = %d\n", First_extinction);
   fprintf(fp, " Extinction_period = %d\n", Extinction_period);
   /* NB; spontaneous response probability; 2020.05.19 */
   fprintf(fp, " Spontaneous_response_prob = %lf\n", Spontaneous_response_prob);
   fprintf(fp, " SRP_gaussian_mu = %lf\n", SRP_gaussian_mu);
   fprintf(fp, " SRP gaussian_std = %lf\n", SRP_gaussian_std);
   fprintf(fp, " Gnuplot_plots = %d\n", Gnuplot_plots);
   fprintf(fp, " Prob_dynamic = %d\n", Prob_dynamic);
   fprintf(fp, " Prob_dynamic_init %lf\n", Prob_dynamic_init);
   fprintf(fp, " Prob_increase %lf\n", Prob_increase);
   fprintf(fp, " Prob_decrease %lf\n", Prob_decrease);
   fprintf(fp, " Prob_dynamic_max %lf\n", Prob_dynamic_max);
   fprintf(fp, " Prob_dynamic_min %lf\n", Prob_dynamic_min);
   fprintf(fp, " P_gain = %lf\n", P_gain);
   fprintf(fp, " D_gain = %lf\n", D_gain);
   fprintf(fp, " I_gain = %lf\n", I_gain);
   fprintf(fp, " Pid = %d\n", Pid);
   fprintf(fp, " Pid_latent_thresholds = %d\n", Pid_latent_thresholds);
   fprintf(fp, " Pid_integral_leak = %lf\n", Pid_integral_leak);
   fprintf(fp, " Pid_integral_bound = %lf\n", Pid_integral_bound);
   fprintf(fp, " Agent_pid_gains = %d\n", Agent_pid_gains);
   fprintf(fp, " Agent_pid_gain_spread = %lf\n", Agent_pid_gain_spread);
   fprintf(fp, " Agent_pid_p_spread = %lf\n", Agent_pid_p_spread);
   fprintf(fp, " Agent_pid_i_spread = %lf\n", Agent_pid_i_spread);
   fprintf(fp, " Agent_pid_d_spread = %lf\n", Agent_pid_d_spread);
   fprintf(fp, " Agent_pid_gain_apply_id = %d\n", Agent_pid_gain_apply_id);
   fprintf(fp, " Feedback_noise_enabled = %d\n", Feedback_noise_enabled);
   fprintf(fp, " Feedback_noise_alpha = %lf\n", Feedback_noise_alpha);
   fprintf(fp, " Feedback_noise_seed = %ld\n", Feedback_noise_seed);
   fprintf(fp, " Feedback_noise_sigma_mode = mean_active_task_demand\n");
   fprintf(fp, " Feedback_noise_clip = %d\n", Feedback_noise_clip);
   fprintf(fp, " Feedback_bias_alpha = %lf\n", Feedback_bias_alpha);
   fprintf(fp, " Feedback_bias_seed = %ld\n", Feedback_bias_seed);
   if (Feedback_bias_mode == 1)
      fprintf(fp, " Feedback_bias_mode = task_fixed_random\n");
   else if (Feedback_bias_mode == 2)
      fprintf(fp, " Feedback_bias_mode = positive\n");
   else
      fprintf(fp, " Feedback_bias_mode = none\n");
   fprintf(fp, " Demand_segment_len = %d\n", Demand_segment_len);
   fprintf(fp, " Demand_switch_step = %d\n", Demand_switch_step);
   }  /* print_params */

/********** print_opfiles **********/
/* parameters:	fp	where to print, includes stdout (screen)
			Assume that fp has already been fopened.
   called by:	read_default_opfiles, read_opfiles, params.c
   actions:	print out status of output files -- which ones are
		to be printed and which ones are not.
*/

/********** fprint_stepagentmintask ***********/
/* Called by:           step_output(), output.c
   Parameters:          t       current timestep
   Actions:             Called once per step.  Print what task each agent was
                        performing in that time step.
*/

/* Called by:           run_output(), output.c
   Parameters:
   Actions:             Print how many timesteps each agent spent on each task
			and their thresholds.
			Prints a single line of data for each agent.
			For each agent, prints:
			a<agent#> <idle> <north> <east> <south> <west>
*/

void print_opfiles(FILE *fp)
   {
   int i;

   fprintf(fp, " Max_num_output_files = %d\n", Max_num_output_files);

   for (i=0; i<Max_num_output_files; i++)
      {
      fprintf(fp, "     %d: %s %d", i, Output_file[i].extension,
			Output_file[i].on);

/*
      if (Output_file[i].on)  fprintf(fp, " %s\n", Output_file[i].filename);
      else  fprintf(fp, "\n");
*/
      fprintf(fp, "\n");
      }  /* for i */
   }  /* print_opfiles */

/********** start_output **********/
/* parameters:
   called by:   init_fxn(), fxn.c
   actions:     prints everything that needs to be printed at the start
                of a run.
*/
void start_output()
   {
   int array_ptr;

#ifdef DEBUG
   printf(" ---in start_output---\n");
#endif

  /* output to files */
   if (file_on("initpop"))
      {
      array_ptr = get_file_pointer("initpop");
      Output_file[array_ptr].fp = fopen(Output_file[array_ptr].filename, "a");
      fprint_pop(Output_file[array_ptr].fp);
      fclose(Output_file[array_ptr].fp);
      }  /* if initpop */
   if (file_on("params"))
      {
      array_ptr = get_file_pointer("params");
      Output_file[array_ptr].fp = fopen(Output_file[array_ptr].filename, "a");
      print_fxn_params(Output_file[array_ptr].fp);
      fclose(Output_file[array_ptr].fp);
      }  /* if */
   if (file_on("threshrange"))
      {
      array_ptr = get_file_pointer("threshrange");
      Output_file[array_ptr].fp = fopen(Output_file[array_ptr].filename, "a");
      fprint_threshrange(Output_file[array_ptr].fp);
      fclose(Output_file[array_ptr].fp);
      }  /* if */

  /* print initial position of target and tracker */
   if (file_on("stepsummary"))
      {
      array_ptr = get_file_pointer("stepsummary");
      Output_file[array_ptr].fp = fopen(Output_file[array_ptr].filename, "a");
      fprint_stepsummary(Output_file[array_ptr].fp, -1);
      fclose(Output_file[array_ptr].fp);
      }  /* if */
   if (file_on("steptargetpath"))
      {
      array_ptr = get_file_pointer("steptargetpath");
      Output_file[array_ptr].fp = fopen(Output_file[array_ptr].filename, "a");
      fprint_steptargetpath(Output_file[array_ptr].fp, -1);
      fclose(Output_file[array_ptr].fp);
      }  /* if */
   if (file_on("steptrackerpath"))
      {
      array_ptr = get_file_pointer("steptrackerpath");
      Output_file[array_ptr].fp = fopen(Output_file[array_ptr].filename, "a");
      fprint_steptrackerpath(Output_file[array_ptr].fp, -1);
      fclose(Output_file[array_ptr].fp);
      }  /* if */
   if (file_on("stephistnorth") && Intensity_aging)
      {
      array_ptr = get_file_pointer("stephistnorth");
      Output_file[array_ptr].fp = fopen(Output_file[array_ptr].filename, "a");
      fprint_stephistnorth(Output_file[array_ptr].fp, -1);
      fclose(Output_file[array_ptr].fp);
      }  /* if */
   if (file_on("stephistsouth") && Intensity_aging)
      {
      array_ptr = get_file_pointer("stephistsouth");
      Output_file[array_ptr].fp = fopen(Output_file[array_ptr].filename, "a");
      fprint_stephistsouth(Output_file[array_ptr].fp, -1);
      fclose(Output_file[array_ptr].fp);
      }  /* if */
   if (file_on("stephisteast") && Intensity_aging)
      {
      array_ptr = get_file_pointer("stephisteast");
      Output_file[array_ptr].fp = fopen(Output_file[array_ptr].filename, "a");
      fprint_stephisteast(Output_file[array_ptr].fp, -1);
      fclose(Output_file[array_ptr].fp);
      }  /* if */
   if (file_on("stephistwest") && Intensity_aging)
      {
      array_ptr = get_file_pointer("stephistwest");
      Output_file[array_ptr].fp = fopen(Output_file[array_ptr].filename, "a");
      fprint_stephistwest(Output_file[array_ptr].fp, -1);
      fclose(Output_file[array_ptr].fp);
      }  /* if */
   // 22.04.04.ASW
   if (file_on("initprob"))
      {
      array_ptr = get_file_pointer("initprob");
      Output_file[array_ptr].fp = fopen(Output_file[array_ptr].filename, "a");
      fprint_responseprob(Output_file[array_ptr].fp);
      fclose(Output_file[array_ptr].fp);
      }  /* if initprob */

   /* print the initialized aging ranges for each agent */
   if (file_on("intensityrange"))
      {
      array_ptr = get_file_pointer("intensityrange");
      Output_file[array_ptr].fp = fopen(Output_file[array_ptr].filename, "a");
      fprint_intensityrange(Output_file[array_ptr].fp);
      fclose(Output_file[array_ptr].fp);
      }  /* if */

#ifdef DEBUG
   printf(" ---end start_output---\n");
#endif
   }  /* start_output */

/********** step_output **********/
/* parameters:
   called by:
   actions:     prints everything that needs to be printed at the end
                of an step.
   parameters:  t = current timestep
*/
void step_output(int t)
   {
   int ptr;

#ifdef DEBUG
   printf(" ---in step_output---\n");
#endif

  /* output to files */
  /* if */

   if (file_on("stepthresh"))
      {
      ptr = get_file_pointer("stepthresh");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_stepthresh(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }  /* if */ 
   if (file_on("stepsummary"))
      {
      ptr = get_file_pointer("stepsummary");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_stepsummary(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }  /* if */
   if (file_on("stepdemand"))
      {
      ptr = get_file_pointer("stepdemand");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_stepdemand(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }  /* if */
   if (file_on("steptaskdemand"))
      {
      ptr = get_file_pointer("steptaskdemand");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_steptaskdemand(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }
   if (file_on("steptaskcounts"))
      {
      ptr = get_file_pointer("steptaskcounts");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_steptaskcounts(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }
   if (file_on("steptaskthresh"))
      {
      ptr = get_file_pointer("steptaskthresh");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_steptaskthresh(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }
   if (file_on("steptargetpath"))
      {
      ptr = get_file_pointer("steptargetpath");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_steptargetpath(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }  /* if */
   if (file_on("steptrackerpath"))
      {
      ptr = get_file_pointer("steptrackerpath");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_steptrackerpath(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }  /* if */
   if (file_on("stepnorthsouth"))
      {
      ptr = get_file_pointer("stepnorthsouth");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_stepnorthsouth(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }  /* if */
   if (file_on("stepeastwest"))
      {
      ptr = get_file_pointer("stepeastwest");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_stepeastwest(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }  /* if */
   if (file_on("stepagentaction"))
      {
      ptr = get_file_pointer("stepagentaction");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_stepagentaction(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }  /* if */
   if (file_on("stepagentactionwtime"))
      {
      ptr = get_file_pointer("stepagentactionwtime");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_stepagentactionwtime(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }  /* if */
   if (file_on("stepagentactionxyz"))
      {
      ptr = get_file_pointer("stepagentactionxyz");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_stepagentactionxyz(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }  /* if */
   if (file_on("stephistnorth") && Intensity_aging)
      {
      ptr = get_file_pointer("stephistnorth");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_stephistnorth(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }  /* if */
   if (file_on("stephistsouth") && Intensity_aging)
      {
      ptr = get_file_pointer("stephistsouth");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_stephistsouth(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }  /* if */
   if (file_on("stephisteast") && Intensity_aging)
      {
      ptr = get_file_pointer("stephisteast");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_stephisteast(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }  /* if */
   if (file_on("stephistwest") && Intensity_aging)
      {
      ptr = get_file_pointer("stephistwest");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_stephistwest(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }  /* if */
   if (file_on("stepthreshnorth"))
      {
      ptr = get_file_pointer("stepthreshnorth");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_stepthreshnorth(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }  /* if */
   if (file_on("stepthreshsouth"))
      {
      ptr = get_file_pointer("stepthreshsouth");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_stepthreshsouth(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }  /* if */
   if (file_on("stepthresheast"))
      {
      ptr = get_file_pointer("stepthresheast");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_stepthresheast(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }  /* if */
   if (file_on("stepthreshwest"))
      {
      ptr = get_file_pointer("stepthreshwest");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_stepthreshwest(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }  /* if */
   if (file_on("stepintensity"))
      {
      ptr = get_file_pointer("stepintensity");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_stepintensity(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }  /* if */
   if (file_on("stepintensitynorth"))
      {
      ptr = get_file_pointer("stepintensitynorth");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_stepintensitynorth(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }  /* if */
   if (file_on("stepintensitysouth"))
      {
      ptr = get_file_pointer("stepintensitysouth");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_stepintensitysouth(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }  /* if */
   if (file_on("stepintensityeast"))
      {
      ptr = get_file_pointer("stepintensityeast");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_stepintensityeast(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }  /* if */
   if (file_on("stepintensitywest"))
      {
      ptr = get_file_pointer("stepintensitywest");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_stepintensitywest(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }  /* if */
   if (file_on("stepprobnorth"))
      {
      ptr = get_file_pointer("stepprobnorth");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_stepprobnorth(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }  /* if */
   if (file_on("stepprobsouth"))
      {
      ptr = get_file_pointer("stepprobsouth");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_stepprobsouth(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }  /* if */
   if (file_on("stepprobeast"))
      {
      ptr = get_file_pointer("stepprobeast");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_stepprobeast(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }  /* if */
   if (file_on("stepprobwest"))
      {
      ptr = get_file_pointer("stepprobwest");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_stepprobwest(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }  /* if */
    if (file_on("stepagentmintask"))
      {
      ptr = get_file_pointer("stepagentmintask");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_stepagentmintask(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }  /* if */
   if (file_on("stepagentmintaskaction"))
      {
      ptr = get_file_pointer("stepagentmintaskaction");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      fprint_stepagentmintaskaction(Output_file[ptr].fp, t);
      fclose(Output_file[ptr].fp);
      }  /* if */

   /* create files for animating threshold versus action/switches */
   if (Animate_thresh == 1)
      {
      fprint_animate_thresh_data(t);
      fprint_animate_path_data(t);
      }

#ifdef DEBUG
   printf(" ---end step_output---\n");
#endif
   }  /* step_output */

/********** run_output **********/
/* parameters:
   called by:   end_sim(), sim.c
   actions:     prints everything that needs to be printed at the end
                of a run.
*/
void run_output()
   {
   int array_ptr;
   int i;
   double sum;

#ifdef DEBUG
   printf(" ---in run_output---\n");
#endif

  /* output to files */
   if (file_on("finalstats"))
      {
      array_ptr = get_file_pointer("finalstats");
      Output_file[array_ptr].fp = fopen(Output_file[array_ptr].filename, "a");
      fprint_finalstats(Output_file[array_ptr].fp);
      fclose(Output_file[array_ptr].fp);
      }
   if (file_on("finalagent"))
      {
      array_ptr = get_file_pointer("finalagent");
      Output_file[array_ptr].fp = fopen(Output_file[array_ptr].filename, "a");
      fprint_finalagent(Output_file[array_ptr].fp);
      fclose(Output_file[array_ptr].fp);
      }
   if (file_on("finaltask"))
      {
      array_ptr = get_file_pointer("finaltask");
      Output_file[array_ptr].fp = fopen(Output_file[array_ptr].filename, "a");
      fprint_finaltask(Output_file[array_ptr].fp);
      fclose(Output_file[array_ptr].fp);
      }
   if (file_on("gnu"))
      {
      array_ptr = get_file_pointer("gnu");
      Output_file[array_ptr].fp = fopen(Output_file[array_ptr].filename, "a");
      fprint_gnu(Output_file[array_ptr].fp);
      fclose(Output_file[array_ptr].fp);
      }
    // HDM; 2019.09.19
    if (file_on("finalintensity"))
        {
        array_ptr = get_file_pointer("finalintensity");
        Output_file[array_ptr].fp = fopen(Output_file[array_ptr].filename, "a");
        fprint_intensities(Output_file[array_ptr].fp);
        fclose(Output_file[array_ptr].fp);
        }
   // 20.04.18.ASW
   if (file_on("finalpop"))
      {
      array_ptr = get_file_pointer("finalpop");
      Output_file[array_ptr].fp = fopen(Output_file[array_ptr].filename, "a");
      fprint_pop(Output_file[array_ptr].fp);
      fclose(Output_file[array_ptr].fp);
      }  /* if initpop */
   // 20.12.08.ASW
   if (file_on("finalthreshswitch"))
      {
      array_ptr = get_file_pointer("finalthreshswitch");
      Output_file[array_ptr].fp = fopen(Output_file[array_ptr].filename, "a");
      fprint_finalthreshswitch(Output_file[array_ptr].fp);
      fclose(Output_file[array_ptr].fp);
      }  /* if initpop */
   if (file_on("finalthreshact"))
      {
      array_ptr = get_file_pointer("finalthreshact");
      Output_file[array_ptr].fp = fopen(Output_file[array_ptr].filename, "a");
      fprint_finalthreshact(Output_file[array_ptr].fp);
      fclose(Output_file[array_ptr].fp);
      }  /* if initpop */
   // 22.04.04.ASW
   if (file_on("finalprob"))
      {
      array_ptr = get_file_pointer("finalprob");
      Output_file[array_ptr].fp = fopen(Output_file[array_ptr].filename, "a");
      fprint_responseprob(Output_file[array_ptr].fp);
      fclose(Output_file[array_ptr].fp);
      }  /* if initprob */

   if (file_on("stepsummary"))
      create_stepsummary_cleaned();

   /* print gnuplot file for animating threshold versus action/switches */
   if (Animate_thresh == 1)
      {
      fprint_animate_threshact_gnu();
      fprint_animate_threshswitch_gnu();
      fprint_animate_path_gnu();
      fprint_animate_combo_gnu();
      fprint_animate_threshactseparate_gnu();
      }

#ifdef DEBUG
   printf(" ---end run_output---\n");
#endif
   }  /* run_output */

/********** fprint_pop **********/
void fprint_pop(FILE *fp)
   {
   int i;
   int j;

#ifdef DEBUG
printf("---in fprint_pop()---\n");
#endif

   for (i=0; i<Pop_size; i++)
      {
      // print info for one agent on each line
      fprintf(fp, " agent %d raw_thresh t-n %lf t-s %lf t-e %lf t-w %lf",
         Agent[i].index,
         Agent[i].raw_thresh_north,
         Agent[i].raw_thresh_south,
         Agent[i].raw_thresh_east,
         Agent[i].raw_thresh_west);
      fprintf(fp, " prob_check %lf", Agent[i].prob_check);
      fprintf(fp, " thresh t-n %lf t-s %lf t-e %lf t-w %lf",
         Agent[i].thresh_north,
         Agent[i].thresh_south,
         Agent[i].thresh_east,
         Agent[i].thresh_west);
      // for plotting the thresholds
      fprintf(fp, " 1 2 3 4");
      // print max and min thresh
      fprintf(fp, " raw_thresh_max %lf raw_thresh_min %lf",
         Agent[i].raw_thresh_max,
         Agent[i].raw_thresh_min);
      if (Thresh_dynamic == 1 || Thresh_dynamic == 2)
         {
         // max and min of each direction
         fprintf(fp, " north_range max %lf min %lf",
            Agent[i].thresh_max_north,
            Agent[i].thresh_min_north);
         fprintf(fp, " south_range max %lf min %lf",
            Agent[i].thresh_max_south,
            Agent[i].thresh_min_south);
         fprintf(fp, " east_range max %lf min %lf",
            Agent[i].thresh_max_east,
            Agent[i].thresh_min_east);
         fprintf(fp, " west_range max %lf min %lf",
            Agent[i].thresh_max_west,
            Agent[i].thresh_min_west);
         }
      // end of line
      fprintf(fp, "\n");
      }  /* for r */

#ifdef DEBUG
printf("---end fprint_pop()---\n");
#endif
   }  /* fprint_pop */

/********** fprint_threshrange ***********/
/* Called by:           step_output(), output.c
   Parameters:          t       current timestep
   Actions:             Called at start of run.  Prints initial threshold and
			threshold range (max and min) for each agent.  
			One agent per line.
*/
void fprint_threshrange(FILE *fp)
   {
   int i, j;

#ifdef DEBUG
printf("---in fprint_threshrange()---\n");
#endif

   for (i=0; i<Pop_size; i++)
      {
      fprintf(fp, " Agent %4d ", i);
      // for each direction print init, max, min threshold value
      fprintf(fp, " N %lf %lf %lf S %lf %lf %lf E %lf %lf %lf W %lf %lf %lf ",
      Agent[i].thresh_north,Agent[i].thresh_min_north,Agent[i].thresh_max_north,
      Agent[i].thresh_south,Agent[i].thresh_min_south,Agent[i].thresh_max_south,
      Agent[i].thresh_east,Agent[i].thresh_min_east,Agent[i].thresh_max_east,
      Agent[i].thresh_west,Agent[i].thresh_min_west,Agent[i].thresh_max_west);
      fprintf(fp, "\n");
      }

#ifdef DEBUG
printf("---end fprint_threshrange()---\n");
#endif

   }  /* fprint_threshrange */
/********** fprint_stepthresh ***********/
/* Called by:           step_output(), output.c
   Parameters:          t       current timestep
   Actions:             Called once per step to print thresh parameters.
*/



/********** fprint_stepthresh ***********/
/* Called by:           step_output(), output.c
   Parameters:          t       current timestep
   Actions:             Called once per step to print thresh parameters.
*/
void fprint_stepthresh(FILE *fp, int t)
   {
   int i, j;

#ifdef DEBUG
printf("---in fprint_stepthresh()---\n");
#endif

   fprintf(fp, " T %4d ", t);
   for (i=0; i<Pop_size; i++)
      {
      fprintf(fp, " A %d ", i);
      fprintf(fp, " N %lf S %lf E %lf W %lf ",
		Agent[i].thresh_north, Agent[i].thresh_south,
		Agent[i].thresh_east, Agent[i].thresh_west);
      }
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end fprint_stepthresh()---\n");
#endif

   }  /* fprint_stepthresh */

/********** fprint_stepsummary ***********/
/* Called by:           step_output(), output.c
   Parameters:          t       current timestep
   Actions:             Print CSV task-vector summary values for each step.
*/
void fprint_stepsummary(FILE *fp, int t)
   {
   int task;
   int actors = 0;
   double arrival_sum = 0.0;
   double service_sum = 0.0;
   double demand_sum = 0.0;

#ifdef DEBUG
printf("---in fprint_stepsummary()---\n");
#endif

   if (t < 0)
      {
      fprintf(fp, "timestep,target_vector_norm,tracker_vector_norm,pre_service_residual_norm,residual_norm,step_arrival_sum,step_service_sum,demand_sum,actors,idle,num_switch,pct_actors_switch,pct_all_switch,num_switch_noidle\n");
      return;
      }

   for (task=1; task<=Num_tasks; task++)
      {
      actors += Task_actor_count[task];
      arrival_sum += Task_arrival[task];
      service_sum += Task_service[task];
      demand_sum += Task_demand[task];
      }

   fprintf(fp,
           "%d,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%d,%d,%d,%lf,%lf,%d\n",
           t, Target.length, Tracker.length,
           Tracker.pre_service_difference, Tracker.difference,
           arrival_sum, service_sum, demand_sum, actors, Num_alive - actors,
           Tracker.num_switch, Tracker.pct_actors_switch,
           Tracker.pct_all_switch, Tracker.num_switch_noidle);

#ifdef DEBUG
printf("---end fprint_stepsummary()---\n");
#endif
   }  /* fprint_stepsummary */

/********** fprint_stepdemand ***********/
/* Called by:           step_output(), output.c
   Parameters:          t       current timestep
   Actions:             Print CSV task-vector summary norms for each timestep.
*/
void fprint_stepdemand(FILE *fp, int t)
   {
   double total_arrival = 0.0;
   double total_service = 0.0;
   double total_demand = 0.0;
   int task;

#ifdef DEBUG
printf("---in fprint_stepdemand()---\n");
#endif

   if (t == 0)
      fprintf(fp, "timestep,target_vector_norm,tracker_vector_norm,pre_service_residual_norm,residual_norm,step_arrival_sum,step_service_sum,demand_sum\n");

   for (task=1; task<=Num_tasks; task++)
      {
      total_arrival += Task_arrival[task];
      total_service += Task_service[task];
      total_demand += Task_demand[task];
      }

   fprintf(fp, "%d,%lf,%lf,%lf,%lf,%lf,%lf,%lf\n",
           t, Target.length, Tracker.length,
           Tracker.pre_service_difference, Tracker.difference,
           total_arrival, total_service, total_demand);

#ifdef DEBUG
printf("---end fprint_stepdemand()---\n");
#endif
   }  /* fprint_stepdemand */

void fprint_steptaskdemand(FILE *fp, int t)
   {
   int task;

   if (t == 0)
      {
      fprintf(fp, "timestep");
      fprint_task_csv_header(fp, "demand");
      fprint_task_csv_header(fp, "arrival");
      fprint_task_csv_header(fp, "service");
      fprint_task_csv_header(fp, "total_arrival");
      fprint_task_csv_header(fp, "total_service");
      fprintf(fp, "\n");
      }

   fprintf(fp, "%d", t);
   for (task=1; task<=Num_tasks; task++) fprintf(fp, ",%lf", Task_demand[task]);
   for (task=1; task<=Num_tasks; task++) fprintf(fp, ",%lf", Task_arrival[task]);
   for (task=1; task<=Num_tasks; task++) fprintf(fp, ",%lf", Task_service[task]);
   for (task=1; task<=Num_tasks; task++) fprintf(fp, ",%lf", Task_total_arrival[task]);
   for (task=1; task<=Num_tasks; task++) fprintf(fp, ",%lf", Task_total_service[task]);
   fprintf(fp, "\n");
   }  /* fprint_steptaskdemand */

void fprint_steptaskcounts(FILE *fp, int t)
   {
   int task;

   if (t == 0)
      {
      fprintf(fp, "timestep");
      fprint_task_csv_header(fp, "actors");
      fprintf(fp, "\n");
      }

   fprintf(fp, "%d", t);
   for (task=1; task<=Num_tasks; task++) fprintf(fp, ",%d", Task_actor_count[task]);
   fprintf(fp, "\n");
   }  /* fprint_steptaskcounts */

void fprint_steptaskthresh(FILE *fp, int t)
   {
   int task, agent;
   double avg_thresh, avg_prob;

   if (t == 0)
      {
      fprintf(fp, "timestep");
      fprint_task_csv_header(fp, "avg_thresh");
      fprint_task_csv_header(fp, "avg_resprob");
      fprintf(fp, "\n");
      }

   fprintf(fp, "%d", t);
   for (task=1; task<=Num_tasks; task++)
      {
      avg_thresh = 0.0;
      for (agent=0; agent<Pop_size; agent++)
         avg_thresh += Agent[agent].thresh_task[task];
      fprintf(fp, ",%lf", avg_thresh / (double)Pop_size);
      }

   for (task=1; task<=Num_tasks; task++)
      {
      avg_prob = 0.0;
      for (agent=0; agent<Pop_size; agent++)
         avg_prob += Agent[agent].resprob[task];
      fprintf(fp, ",%lf", avg_prob / (double)Pop_size);
      }
   fprintf(fp, "\n");
   }  /* fprint_steptaskthresh */

/********** fprint_steptargetpath ***********/
/* Called by:           step_output(), output.c
   Parameters:          t       current timestep
   Actions:             Called once per step to print target coordinates.
*/
void fprint_steptargetpath(FILE *fp, int t)
   {
   int i, j;

#ifdef DEBUG
printf("---in fprint_steptargetpath()---\n");
#endif

   fprintf(fp, " T %4d x %lf y %lf %s\n", t, Target.x, Target.y, Target.side);

#ifdef DEBUG
printf("---end fprint_steptargetpath()---\n");
#endif

   }  /* fprint_steptargetpath */

/********** fprint_steptrackerpath ***********/
/* Called by:           step_output(), output.c
   Parameters:          t       current timestep
   Actions:             Called once per step to print tracker coordinates.
*/
void fprint_steptrackerpath(FILE *fp, int t)
   {
   int i, j;

#ifdef DEBUG
printf("---in fprint_steptrackerpath()---\n");
#endif

   fprintf(fp, " T %4d x %lf y %lf\n", t, Tracker.x, Tracker.y);

#ifdef DEBUG
printf("---end fprint_steptrackerpath()---\n");
#endif

   }  /* fprint_steptrackerpath */

/********** fprint_stepnorthsouth ***********/
/* Called by:           step_output(), output.c
   Parameters:          t       current timestep
   Actions:             Called once per step to print tracker coordinates
			on y-axis.
*/
void fprint_stepnorthsouth(FILE *fp, int t)
   {
   int i, j;

#ifdef DEBUG
printf("---in fprint_stepnorthsouth()---\n");
#endif

   fprintf(fp, " T %4d target %lf tracker %lf y-diff %lf\n", t, Target.y, Tracker.y, Target.y-Tracker.y);

#ifdef DEBUG
printf("---end fprint_stepnorthsouth()---\n");
#endif

   }  /* fprint_stepnorthsouth */

/********** fprint_stepeastwest ***********/
/* Called by:           step_output(), output.c
   Parameters:          t       current timestep
   Actions:             Called once per step to print tracker coordinates
			on x-axis.
*/
void fprint_stepeastwest(FILE *fp, int t)
   {
   int i, j;

#ifdef DEBUG
printf("---in fprint_stepeastwest()---\n");
#endif

   fprintf(fp, " T %4d target %lf tracker %lf x-diff %lf\n", t, Target.x, Tracker.x, Target.x-Tracker.x);

#ifdef DEBUG
printf("---end fprint_stepeastwest()---\n");
#endif

   }  /* fprint_stepeastwest */

/********** fprint_finalstats ***********/
/* Called by:           run_output(), output.c
   Parameters:          fp      where to print
   Actions:             Print final task-vector summary statistics as CSV.
*/
void fprint_finalstats(FILE *fp)
   {
   int i;
   double sum;
   int max_switches, min_switches;
   double avg_switch, avg_spontaneous_switch, avg_switch_noidle;
   int max_spontaneous_switches, min_spontaneous_switches;
   int max_switch_noidle, min_switch_noidle;
   double R, R_abs, R2, R2_norm, R2_max_norm;
   double post_removal_R, post_removal_R_abs;
   double post_removal_R2_norm, post_removal_R2_max_norm;

#ifdef DEBUG
printf("---in fprint_finalstats()---\n");
#endif

   sum = 0.0;
   max_switches = -1;
   min_switches = 2 * Max_steps;
   for (i=0; i<Pop_size; i++)
      {
      sum += Agent[i].count_switch;
      if (Agent[i].count_switch > max_switches)
         max_switches = Agent[i].count_switch;
      if (Agent[i].count_switch < min_switches)
         min_switches = Agent[i].count_switch;
      }
   avg_switch = sum/(double)Pop_size;

   sum = 0.0;
   max_spontaneous_switches = -1;
   min_spontaneous_switches = 2 * Max_steps;
   for (i=0; i<Pop_size; i++)
      {
      sum += Agent[i].count_switch_spontaneous;
      if (Agent[i].count_switch_spontaneous > max_spontaneous_switches)
         max_spontaneous_switches = Agent[i].count_switch_spontaneous;
      if (Agent[i].count_switch_spontaneous < min_spontaneous_switches)
         min_spontaneous_switches = Agent[i].count_switch_spontaneous;
      }
   avg_spontaneous_switch = sum/(double)Pop_size;

   sum = 0.0;
   max_switch_noidle = -1;
   min_switch_noidle = 2 * Max_steps;
   for (i=0; i<Pop_size; i++)
      {
      sum += Agent[i].count_switch_noidle;
      if (Agent[i].count_switch_noidle > max_switch_noidle)
         max_switch_noidle = Agent[i].count_switch_noidle;
      if (Agent[i].count_switch_noidle < min_switch_noidle)
         min_switch_noidle = Agent[i].count_switch_noidle;
      }
   avg_switch_noidle = sum/(double)Pop_size;

   R = (Max_steps > 0) ? Tracker.metric_sum_R/(double)Max_steps : 0.0;
   R_abs = (Max_steps > 0) ? Tracker.metric_sum_R_abs/(double)Max_steps : 0.0;
   R2 = (Max_steps > 0) ? Tracker.metric_sum_R2/(double)Max_steps : 0.0;
   R2_norm = (Max_steps > 0) ? Tracker.metric_sum_R2_norm/(double)Max_steps : 0.0;
   R2_max_norm = Tracker.metric_max_R2_norm;
   post_removal_R = (Tracker.post_removal_steps > 0) ?
      Tracker.post_removal_sum_R/(double)Tracker.post_removal_steps : 0.0;
   post_removal_R_abs = (Tracker.post_removal_steps > 0) ?
      Tracker.post_removal_sum_R_abs/(double)Tracker.post_removal_steps : 0.0;
   post_removal_R2_norm = (Tracker.post_removal_steps > 0) ?
      Tracker.post_removal_sum_R2_norm/(double)Tracker.post_removal_steps : 0.0;
   post_removal_R2_max_norm = Tracker.post_removal_max_R2_norm;

   fprintf(fp,
           "run,target_vector_norm,tracker_vector_norm,avg_pre_service_residual_norm,max_pre_service_residual_norm,min_pre_service_residual_norm,avg_residual_norm,max_residual_norm,min_residual_norm,avg_switch,max_switch,min_switch,avg_spontaneous_switch,max_spontaneous_switch,min_spontaneous_switch,avg_switch_noidle,max_switch_noidle,min_switch_noidle,R,R_abs,R2,R2_norm,R2_max_norm,post_removal_R,post_removal_R_abs,post_removal_R2_norm,post_removal_R2_max_norm,post_removal_steps\n");
   fprintf(fp,
           "%d,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%d,%d,%lf,%d,%d,%lf,%d,%d,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%d\n",
           Run_num, Target.length, Tracker.length,
           Tracker.total_pre_service_difference/(double)Max_steps,
           Tracker.max_pre_service_difference,
           Tracker.min_pre_service_difference,
           Tracker.total_difference/(double)Max_steps,
           Tracker.max_difference, Tracker.min_difference,
           avg_switch, max_switches, min_switches,
           avg_spontaneous_switch, max_spontaneous_switches, min_spontaneous_switches,
           avg_switch_noidle, max_switch_noidle, min_switch_noidle,
           R, R_abs, R2, R2_norm, R2_max_norm,
           post_removal_R, post_removal_R_abs,
           post_removal_R2_norm, post_removal_R2_max_norm,
           Tracker.post_removal_steps);

#ifdef DEBUG
printf("---end fprint_finalstats()---\n");
#endif
   }  /* fprint_finalstats */

/********** fprint_stepagentaction ***********/
/* Called by:           step_output(), output.c
   Parameters:          t       current timestep
   Actions:             Print CSV task id selected by each agent each step.
*/
void fprint_stepagentaction(FILE *fp, int t)
   {
   int i;

#ifdef DEBUG
printf("---in fprint_stepagentaction()---\n");
#endif

   if (t == 0) fprint_agent_csv_header(fp);

   fprintf(fp, "%d", t);
   for (i=0; i<Pop_size; i++)
      {
      if(!Agent[i].dead) fprintf(fp, ",%d", Agent[i].current_task);
      else fprintf(fp, ",-1");
      }
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end fprint_stepagentaction()---\n");
#endif
   }  /* fprint_stepagentaction */

/********** fprint_stepagentactionwtime ***********/
/* Compatibility alias for CSV stepagentaction. */
void fprint_stepagentactionwtime(FILE *fp, int t)
   {
   fprint_stepagentaction(fp, t);
   }  /* fprint_stepagentactionwtime */

/********** fprint_stepagentactionxyz ***********/
/* Called by:           step_output(), output.c
   Parameters:          t       current timestep
   Actions:             Called once per step.  Print what task each agent was
                        performing in that time step.
*/
void fprint_stepagentactionxyz(FILE *fp, int t)
   {
   int i;

#ifdef DEBUG
printf("---in fprint_stepagentactionxyz()---\n");
#endif

   for (i=0; i<Pop_size; i++)
      {
      if(!Agent[i].dead)
         fprintf(fp, " T %4d agent %4d current_task %d\n",
                 t, i, Agent[i].current_task);
      else
         fprintf(fp, " T %4d agent %4d current_task %d\n",
                t, i, -1);
      }
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end fprint_stepagentactionxyz()---\n");
#endif

   }  /* fprint_stepagentactionxyz */
/********** fprint_stepagentmintask ***********/
/* Called by:           step_output(), output.c
   Parameters:          t       current timestep
   Actions:             Print the minimum-threshold task channel for each agent.
*/
void fprint_stepagentmintask(FILE *fp, int t)
   {
   int i, task;
   double min_threshold;
   int min_task;

#ifdef DEBUG
printf("---in fprint_stepagentmintask()---\n");
#endif

   for (i=0; i<Pop_size; i++)
      {
      min_threshold = Range * Range;
      min_task = -1;
      for (task=1; task<=Num_tasks; task++)
         {
         if (Agent[i].thresh_task[task] < min_threshold)
            {
            min_threshold = Agent[i].thresh_task[task];
            min_task = task;
            }
         }
      fprintf(fp, " %d", min_task);
      }
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end fprint_stepagentmintask()---\n");
#endif
   }  /* fprint_stepagentmintask */
/********** fprint_stepagentmintaskaction ***********/
/* Called by:           step_output(), output.c
   Parameters:          t       current timestep
   Actions:             Print whether each active agent is on its minimum-threshold task.
*/
void fprint_stepagentmintaskaction(FILE *fp, int t)
   {
   int i, task;
   double min_threshold;
   int min_task;

#ifdef DEBUG
printf("---in fprint_stepagentmintaskaction()---\n");
#endif

   for (i=0; i<Pop_size; i++)
      {
      min_threshold = Range * Range;
      min_task = -1;

      if (Agent[i].current_task == 0)
         fprintf(fp, " 2");
      else
         {
         for (task=1; task<=Num_tasks; task++)
            {
            if (Agent[i].thresh_task[task] < min_threshold)
               {
               min_threshold = Agent[i].thresh_task[task];
               min_task = task;
               }
            }
         fprintf(fp, " %d", (min_task == Agent[i].current_task) ? 1 : 0);
         }
      }
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end fprint_stepagentmintaskaction()---\n");
#endif
   }  /* fprint_stepagentmintaskaction */
/********** fprint_finalagent ***********/
/* Called by:           run_output(), output.c
   Actions:             Print CSV per-agent, per-task final counts and state.
*/
void fprint_finalagent(FILE *fp)
   {
   int i, task;

#ifdef DEBUG
printf("---in fprint_finalagent()---\n");
#endif

   fprintf(fp, "agent,task,count,threshold,response_prob,intensity,idle_count,total_switches,switch_noidle,spontaneous_switches,killed_at\n");
   for (i=0; i<Pop_size; i++)
      {
      for (task=1; task<=Num_tasks; task++)
         {
         fprintf(fp, "%d,%d,%d,%lf,%lf,%lf,%d,%d,%d,%d,%d\n",
                 i, task, Agent[i].count_task[task], Agent[i].thresh_task[task],
                 Agent[i].resprob[task], Agent[i].intensity_task[task],
                 Agent[i].count_idle, Agent[i].count_switch,
                 Agent[i].count_switch_noidle, Agent[i].count_switch_spontaneous,
                 Agent[i].time_killed);
         }
      }

#ifdef DEBUG
printf("---end fprint_finalagent()---\n");
#endif
   }  /* fprint_finalagent */

void fprint_finaltask(FILE *fp)
   {
   int task, agent;
   double avg_count, avg_thresh, avg_prob, avg_intensity;

   fprintf(fp, "task,final_demand,total_arrival,total_service,actors_last,avg_count,avg_thresh,avg_resprob,avg_intensity\n");
   for (task=1; task<=Num_tasks; task++)
      {
      avg_count = avg_thresh = avg_prob = avg_intensity = 0.0;
      for (agent=0; agent<Pop_size; agent++)
         {
         avg_count += Agent[agent].count_task[task];
         avg_thresh += Agent[agent].thresh_task[task];
         avg_prob += Agent[agent].resprob[task];
         avg_intensity += Agent[agent].intensity_task[task];
         }
      fprintf(fp, "%d,%lf,%lf,%lf,%d,%lf,%lf,%lf,%lf\n",
              task, Task_demand[task], Task_total_arrival[task],
              Task_total_service[task], Task_actor_count[task],
              avg_count / (double)Pop_size, avg_thresh / (double)Pop_size,
              avg_prob / (double)Pop_size, avg_intensity / (double)Pop_size);
      }
   }  /* fprint_finaltask */
/********** fprint_finalthreshswitch ***********/
/* Called by:
   Actions:             Print threshold vs switch count for all task channels.
*/
void fprint_finalthreshswitch(FILE *fp)
   {
   int i, task;

#ifdef DEBUG
printf("---in fprint_finalthreshswitch()---\n");
#endif

   for (i=0; i<Pop_size; i++)
      {
      fprintf(fp, " agent %d switch %d thresholds", i, Agent[i].count_switch);
      for (task=1; task<=Num_tasks; task++) fprintf(fp, " task%d %lf", task, Agent[i].thresh_task[task]);
      fprintf(fp, "\n");
      }

#ifdef DEBUG
printf("---end fprint_finalthreshswitch()---\n");
#endif
   }  /* fprint_finalthreshswitch */
/********** fprint_finalthreshact ***********/
/* Called by:           run_output()
   Actions:             Print thresholds and action counts for every task.
*/
void fprint_finalthreshact(FILE *fp)
   {
   int i, j, task;
   int largest_value, largest_index;
   int *selected, *order;

#ifdef DEBUG
printf("---in fprint_finalthreshact()---\n");
#endif

   selected = (int *)malloc(Pop_size * sizeof(int) );
   order = (int *)malloc(Pop_size * sizeof(int) );
   for (i=0; i<Pop_size; i++) { selected[i] = 0; order[i] = -1; }

   for (i=0; i<Pop_size; i++)
      {
      largest_value = -1;
      largest_index = -1;
      for (j=0; j<Pop_size; j++)
         if (selected[j] == 0 && Agent[j].count_idle > largest_value)
            { largest_value = Agent[j].count_idle; largest_index = j; }
      order[i] = largest_index;
      selected[largest_index] = 1;
      }

   for (task=1; task<=Num_tasks; task++)
      {
      fprintf(fp, " Task %d thresholds", task);
      for (i=0; i<Pop_size; i++) fprintf(fp, " %lf", Agent[order[i]].thresh_task[task]);
      fprintf(fp, " actioncount");
      for (i=0; i<Pop_size; i++) fprintf(fp, " %3d", Agent[order[i]].count_task[task]);
      fprintf(fp, "\n");
      }

   free(selected);
   free(order);

#ifdef DEBUG
printf("---end fprint_finalthreshact()---\n");
#endif
   }  /* fprint_finalthreshact */
/********** fprint_agentthresh ***********/
/* Called by:
   Parameters:		n	which agent
   Actions:             Print agent number and it's thresholds.
*/
void fprint_agentthresh(FILE *fp, int n)
   {
   int j;

#ifdef DEBUG
printf("---in fprint_agentthresh()---\n");
#endif

//  19.09.12.AW:  This function is not currently active right now.
//                Did not delete because we may want to print this out.

#ifdef OUT
   fprintf(fp, " Agent %d ", n);
   for (j=0; j<Nest.num_tasks; j++)
      {
      fprintf(fp, " %s %lf ", Nest.task[j].name, Agent[n].thresh[j]);
      }
      fprintf(fp, "\n");
#endif

#ifdef DEBUG
printf("---end fprint_agentthresh()---\n");
#endif
   }  /* fprint_agentthresh */

// HDM; 2019.09.19
/********** fprint_intensities **********/
/* Called by:       run_output
   Actions:         print average/min/max intensity by task for each agent
*/
void fprint_intensities(FILE *fp)
    {
    int i, task;
    double avg;

    for(i=0; i<Pop_size; i++)
        {
        fprintf(fp, " agent %5d", i);
        for(task=1; task<=Num_tasks; task++)
            {
            if(Agent[i].count_task[task] > 0)
               avg = Intensity_variation == 0 ? Agent[i].intensity_task[task]
                  : Agent[i].int_tot_task[task] / Agent[i].count_task[task];
            else avg = 0.0;
            fprintf(fp, " task%d_avg %lf task%d_min %lf task%d_max %lf",
                    task, avg, task, Agent[i].int_min_task[task], task, Agent[i].int_max_task[task]);
            }
        fprintf(fp, "\n");
        }
    }  /* fprint_intensities */
/********** fprint_stephistnorth **********/
/* Parameters:      fp  pointer to file
                    timestep  int representing current step of simulation
   Called by:       step_output
   Actions:         print intensity distribution data for current timestep
*/
void fprint_stephistnorth(FILE *fp, int timestep)
{
    for(int i = 0; i < HISTOGRAM_SIZE; i++)
    {
        fprintf(fp, " T %5d Bin %4d Count %4d \n", timestep, i, Tracker.intensity_north_dist[i]);
    }
}

// HDM; 2019.10.10
/********** fprint_stephistsouth **********/
/* Parameters:      fp  pointer to file
                    timestep  int representing current step of simulation
   Called by:       step_output
   Actions:         print intensity distribution data for current timestep
*/
void fprint_stephistsouth(FILE *fp, int timestep)
{
    for(int i = 0; i < HISTOGRAM_SIZE; i++)
    {
        fprintf(fp, " T %5d Bin %4d Count %4d \n", timestep, i, Tracker.intensity_south_dist[i]);
    }
} /* fprint_stephistsouth */

// HDM; 2019.10.10
/********** fprint_stephisteast **********/
/* Parameters:      fp  pointer to file
                    timestep  int representing current step of simulation
   Called by:       step_output
   Actions:         print intensity distribution data for current timestep
*/
void fprint_stephisteast(FILE *fp, int timestep)
{
    for(int i = 0; i < HISTOGRAM_SIZE; i++)
    {
        fprintf(fp, " T %5d Bin %4d Count %4d \n", timestep, i, Tracker.intensity_east_dist[i]);
    }
} /* fprint_stephisteast */

// HDM; 2019.10.10
/********** fprint_stephistwest **********/
/* Parameters:      fp  pointer to file
                    timestep  int representing current step of simulation
   Called by:       step_output
   Actions:         print intensity distribution data for current timestep
*/
void fprint_stephistwest(FILE *fp, int timestep)
{
    for(int i = 0; i < HISTOGRAM_SIZE; i++)
    {
        fprintf(fp, " T %5d Bin %4d Count %4d \n", timestep, i, Tracker.intensity_west_dist[i]);
    }
} /* fprint_stephistwest */

// LR; 2020.02.14
/********** fprint_intensityrange **********/
/* Parameters:      fp  pointer to file
   Actions:         Print the intensity aging ranges for each agent
			For each agent, prints:
			agent: <agent#>   intensity aging ranges: n=[min, max], s=[min, max], e=[min, max], w=[min, max]
*/
void fprint_intensityrange(FILE *fp)
   {
   int i;

#ifdef DEBUG
printf("---in fprint_intensityrange()---\n");
#endif

   for (i=0; i<Pop_size; i++)
      {
      fprintf(fp, " Agent %4d ", i);
      // for each direction print init, max, min threshold value
      fprintf(fp, " N %lf %lf %lf E %lf %lf %lf S %lf %lf %lf W %lf %lf %lf ",
      Agent[i].intensity_north,Agent[i].int_aging_min_n,Agent[i].int_aging_max_n,
      Agent[i].intensity_east,Agent[i].int_aging_min_e, Agent[i].int_aging_max_e,
      Agent[i].intensity_south,Agent[i].int_aging_min_s, Agent[i].int_aging_max_s,
      Agent[i].intensity_west,Agent[i].int_aging_min_w, Agent[i].int_aging_max_w);
      fprintf(fp, "\n");
      }

#ifdef DEBUG
printf("---end fprint_intensityrange()---\n");
#endif
}  /* fprint_intensityrange */

/********** fprint_stepthreshnorth ***********/
/* Created:             20.04.17.ASW
   Called by:           step_output(), output.c
   Parameters:          t       current timestep
   Actions:             Called once per step.  Print threshold of each agent 
                        in that time step. 
                        Only called for variable thresholds.
*/
void fprint_stepthreshnorth(FILE *fp, int t)
   {
   int i;

#ifdef DEBUG
printf("---in fprint_stepthreshnorth()---\n");
#endif

// timestep removed to allow plotting using "plot matrix" in gnuplot
   for (i=0; i<Pop_size; i++)
      {
      if(!Agent[i].dead)
         fprintf(fp, " %lf", Agent[i].thresh_north);
      else
         fprintf(fp, " -1");
      }
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end fprint_stepthreshnorth()---\n");
#endif
   }  /* fprint_stepthreshnorth */

/********** fprint_stepthreshsouth ***********/
/* Created:             20.04.17.ASW
   Called by:           step_output(), output.c
   Parameters:          t       current timestep
   Actions:             Called once per step.  Print threshold of each agent
                        in that time step.
                        Only called for variable thresholds.
*/
void fprint_stepthreshsouth(FILE *fp, int t)
   {
   int i;

#ifdef DEBUG
printf("---in fprint_stepthreshsouth()---\n");
#endif

// timestep removed to allow plotting using "plot matrix" in gnuplot
   for (i=0; i<Pop_size; i++)
      {
      if(!Agent[i].dead)
         fprintf(fp, " %lf", Agent[i].thresh_south);
      else
         fprintf(fp, " -1");
      }
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end fprint_stepthreshsouth()---\n");
#endif
   }  /* fprint_stepthreshsouth */

/********** fprint_stepthresheast ***********/
/* Created:             20.04.17.ASW
   Called by:           step_output(), output.c
   Parameters:          t       current timestep
   Actions:             Called once per step.  Print threshold of each agent
                        in that time step.
                        Only called for variable thresholds.
*/
void fprint_stepthresheast(FILE *fp, int t)
   {
   int i;

#ifdef DEBUG
printf("---in fprint_stepthresheast()---\n");
#endif

// timestep removed to allow plotting using "plot matrix" in gnuplot
   for (i=0; i<Pop_size; i++)
      {
      if(!Agent[i].dead)
         fprintf(fp, " %lf", Agent[i].thresh_east);
      else
         fprintf(fp, " -1");
      }
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end fprint_stepthresheast()---\n");
#endif
   }  /* fprint_stepthresheast */

/********** fprint_stepthreshwest ***********/
/* Created:             20.04.17.ASW
   Called by:           step_output(), output.c
   Parameters:          t       current timestep
   Actions:             Called once per step.  Print threshold of each agent
                        in that time step.
                        Only called for variable thresholds.
*/
void fprint_stepthreshwest(FILE *fp, int t)
   {
   int i;

#ifdef DEBUG
printf("---in fprint_stepthreshwest()---\n");
#endif

// timestep removed to allow plotting using "plot matrix" in gnuplot
   for (i=0; i<Pop_size; i++)
      {
      if(!Agent[i].dead)
         fprintf(fp, " %lf", Agent[i].thresh_west);
      else
         fprintf(fp, " -1");
      }
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end fprint_stepthreshwest()---\n");
#endif
   }  /* fprint_stepthreshwest */

/********** fprint_stepintensity ***********/
/* Created:             20.11.04.HDM
   Called by:           step_output(), output.c
   Parameters:          t       current timestep
   Actions:             Called once per step to print intensity parameters.
*/
void fprint_stepintensity(FILE *fp, int t)
   {
   int i, j;

#ifdef DEBUG
printf("---in fprint_stepintensity()---\n");
#endif

   fprintf(fp, " T %4d ", t);
   for (i=0; i<Pop_size; i++)
      {
      fprintf(fp, " A %d ", i);
      fprintf(fp, " N %lf E %lf S %lf W %lf ",
		Agent[i].intensity_north, Agent[i].intensity_east,
		Agent[i].intensity_south, Agent[i].intensity_west);
      }
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end fprint_stepintensity()---\n");
#endif

   }  /* fprint_stepintensity */

/********** fprint_stepintensitynorth ***********/
/* Created:             20.11.06.HDM
   Called by:           step_output(), output.c
   Parameters:          t       current timestep
   Actions:             Called once per step.  Print intensity of each agent 
                        in that time step. 
                        Only called for variable intensities.
*/
void fprint_stepintensitynorth(FILE *fp, int t)
   {
   int i;

#ifdef DEBUG
printf("---in fprint_stepintensitynorth()---\n");
#endif

// timestep removed to allow plotting using "plot matrix" in gnuplot
   for (i=0; i<Pop_size; i++)
      {
      if(!Agent[i].dead)
         fprintf(fp, " %lf", Agent[i].intensity_north);
      else
         fprintf(fp, " -1");
      }
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end fprint_stepintensitynorth()---\n");
#endif
   }  /* fprint_stepintensitynorth */

/********** fprint_stepintensitysouth ***********/
/* Created:             20.11.06.HDM
   Called by:           step_output(), output.c
   Parameters:          t       current timestep
   Actions:             Called once per step.  Print intensity of each agent
                        in that time step.
                        Only called for variable intensities.
*/
void fprint_stepintensitysouth(FILE *fp, int t)
   {
   int i;

#ifdef DEBUG
printf("---in fprint_stepintensitysouth()---\n");
#endif

// timestep removed to allow plotting using "plot matrix" in gnuplot
   for (i=0; i<Pop_size; i++)
      {
      if(!Agent[i].dead)
         fprintf(fp, " %lf", Agent[i].intensity_south);
      else
         fprintf(fp, " -1");
      }
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end fprint_stepintensitysouth()---\n");
#endif
   }  /* fprint_stepintensitysouth */

/********** fprint_stepintensityeast ***********/
/* Created:             20.11.06.HDM
   Called by:           step_output(), output.c
   Parameters:          t       current timestep
   Actions:             Called once per step.  Print intensity of each agent
                        in that time step.
                        Only called for variable intensities.
*/
void fprint_stepintensityeast(FILE *fp, int t)
   {
   int i;

#ifdef DEBUG
printf("---in fprint_stepintensityeast()---\n");
#endif

// timestep removed to allow plotting using "plot matrix" in gnuplot
   for (i=0; i<Pop_size; i++)
      {
      if(!Agent[i].dead)
         fprintf(fp, " %lf", Agent[i].intensity_east);
      else
         fprintf(fp, " -1");
      }
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end fprint_stepintensityeast()---\n");
#endif
   }  /* fprint_stepintensityeast */

/********** fprint_stepintensitywest ***********/
/* Created:             20.11.06.HDM
   Called by:           step_output(), output.c
   Parameters:          t       current timestep
   Actions:             Called once per step.  Print intensity of each agent
                        in that time step.
                        Only called for variable intensities.
*/
void fprint_stepintensitywest(FILE *fp, int t)
   {
   int i;

#ifdef DEBUG
printf("---in fprint_stepintensitywest()---\n");
#endif

// timestep removed to allow plotting using "plot matrix" in gnuplot
   for (i=0; i<Pop_size; i++)
      {
      if(!Agent[i].dead)
         fprintf(fp, " %lf", Agent[i].intensity_west);
      else
         fprintf(fp, " -1");
      }
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end fprint_stepintensitywest()---\n");
#endif
   }  /* fprint_stepintensitywest */

/********** fprint_responseprob **********/
/* Called by:       run_output
   Actions:         Print response probability values for all task channels.
*/
void fprint_responseprob(FILE *fp)
   {
   int i, task;

#ifdef DEBUG
printf("---in fprint_responseprob()---\n");
#endif

   for (i=0; i<Pop_size; i++)
      {
      fprintf(fp, " agent %d response_prob", Agent[i].index);
      for (task=1; task<=Num_tasks; task++) fprintf(fp, " task%d %lf", task, Agent[i].resprob[task]);
      fprintf(fp, " min");
      for (task=1; task<=Num_tasks; task++) fprintf(fp, " task%d %lf", task, Agent[i].resprob_min[task]);
      fprintf(fp, " max");
      for (task=1; task<=Num_tasks; task++) fprintf(fp, " task%d %lf", task, Agent[i].resprob_max[task]);
      fprintf(fp, "\n");
      }

#ifdef DEBUG
printf("---end fprint_responseprob()---\n");
#endif
   }  /* fprint_responseprob */
/********** fprint_stepprobnorth ***********/
/* Created:             22.04.22.ASW
   Called by:           step_output(), output.c
   Parameters:          t       current timestep
   Actions:             Called once per step.  Print response probability 
			for legacy task 1 response probability in that time step. 
                        Only called for variable probabilities.
*/
void fprint_stepprobnorth(FILE *fp, int t)
   {
   int i;

#ifdef DEBUG
printf("---in fprint_stepprobnorth()---\n");
#endif

// timestep removed to allow plotting using "plot matrix" in gnuplot
   for (i=0; i<Pop_size; i++)
      {
      if(!Agent[i].dead)
         fprintf(fp, " %lf", Agent[i].resprob[1]);
      else
         fprintf(fp, " -1");
      }
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end fprint_stepprobnorth()---\n");
#endif
   }  /* fprint_stepprobnorth */

/********** fprint_stepprobeast ***********/
/* Created:             22.04.22.ASW
   Called by:           step_output(), output.c
   Parameters:          t       current timestep
   Actions:             Called once per step.  Print response probability 
                        for legacy task 2 response probability in that time step.
                        Only called for variable probabilities.
*/
void fprint_stepprobeast(FILE *fp, int t)
   {
   int i;

#ifdef DEBUG
printf("---in fprint_stepprobeast()---\n");
#endif

// timestep removed to allow plotting using "plot matrix" in gnuplot
   for (i=0; i<Pop_size; i++)
      {
      if(!Agent[i].dead)
         fprintf(fp, " %lf", Agent[i].resprob[2]);
      else
         fprintf(fp, " -1");
      }
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end fprint_stepprobeast()---\n");
#endif
   }  /* fprint_stepprobeast */

/********** fprint_stepprobsouth ***********/
/* Created:             22.04.22.ASW
   Called by:           step_output(), output.c
   Parameters:          t       current timestep
   Actions:             Called once per step.  Print response probability 
                        for legacy task 3 response probability in that time step.
                        Only called for variable probabilities.
*/
void fprint_stepprobsouth(FILE *fp, int t)
   {
   int i;

#ifdef DEBUG
printf("---in fprint_stepprobsouth()---\n");
#endif

// timestep removed to allow plotting using "plot matrix" in gnuplot
   for (i=0; i<Pop_size; i++)
      {
      if(!Agent[i].dead)
         fprintf(fp, " %lf", Agent[i].resprob[3]);
      else
         fprintf(fp, " -1");
      }
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end fprint_stepprobsouth()---\n");
#endif
   }  /* fprint_stepprobsouth */

/********** fprint_stepprobwest ***********/
/* Created:             22.04.22.ASW
   Called by:           step_output(), output.c
   Parameters:          t       current timestep
   Actions:             Called once per step.  Print response probability 
                        for legacy task 4 response probability in that time step.
                        Only called for variable probabilities.
*/
void fprint_stepprobwest(FILE *fp, int t)
   {
   int i;

#ifdef DEBUG
printf("---in fprint_stepprobwest()---\n");
#endif

// timestep removed to allow plotting using "plot matrix" in gnuplot
   for (i=0; i<Pop_size; i++)
      {
      if(!Agent[i].dead)
         fprintf(fp, " %lf", Agent[i].resprob[4]);
      else
         fprintf(fp, " -1");
      }
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end fprint_stepprobwest()---\n");
#endif
   }  /* fprint_stepprobwest */
