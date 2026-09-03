/* params.c
   08.15.26.AW	Created.
		Adapted from same file in the GA code.
		Routines that have to do with reading in
		parameter values from both the parameter
		and opfiles files.

   Routines:	read_defaults()		06.18.98.AW
		read_params()		06.18.98.AW
		get_next_line()		06.18.98.AW
		set_param()		06.21.98.AW
		read_default_opfiles()	06.21.98.AW
		read_opfiles()		06.21.98.AW
		get_file_pointer()	06.21.98.AW
		setup_output_files()	07.03.98.AW
		file_on()		07.27.98.AW
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "extern.h"
#include "params.h"
#include "output.h"

/********** read_defaults **********/
/* parameters:
   called by:	init_sim(), sim.c
   actions:	Reads in default parameter and opfile values
		from the files: params and opfiles.
*/
int read_defaults()
   {
#ifdef DEBUG
   printf(" ---in read_defaults()---\n");
#endif
  /* read values from params */
   if (read_params("params.default") == ERROR)  return ERROR;
  /* read values from opfiles */
   if (read_default_opfiles("opfiles.default") == ERROR)  return ERROR;
#ifdef DEBUG
   printf(" ---end read_defaults()---\n");
#endif
   return OK;
   }  /* read_defaults */

/********** read_params **********/
/* parameters:  params_file
   called by:   init_sim(), sim.c
                read_defaults, params.c
   actions:     Read from specified params file, one line at
                a time and set the correct parameter value
                in global.h.
                Skip the comment lines which are denoted
                by a "#" or "%" xter in the first column.
		Each line contains the name of a parameter
		followed by its value.
*/
int read_params(char *params_file)
   {
   FILE *fp;
   char *aline;

#ifdef DEBUG
   printf(" ---in read_params()--- %s\n", params_file);
#endif
  /* open input file */
   fp = fopen(params_file, "r");
   if (fp == NULL)
      {
      printf(" Error(read_params): cannot open file: %s\n", params_file);
      return ERROR;
      }  /* if */
   printf(" **** Reading from file: %s\n", params_file);

  /* allocate space for aline */
   aline = (char *)malloc(INPUT_LINE_LEN * sizeof(char));

   while (get_next_line(fp, aline) != ENDOFFILE)
      {
      if (set_param(aline) == ERROR)  return ERROR;
      }  /* while */

   free(aline);
   fclose(fp);
#ifdef DEBUG
   printf(" ---end read_params()---\n");
#endif
   return OK;
   }  /* read_params */

/********** get_next_line **********/
/* parameters:  fp			file to read from
		aline                   temp string for parsing comments
   called by:   read_params(), params.c
   actions:     Read the next non-comment line from fp into aline.
*/
int get_next_line(FILE *fp, char *aline)
   {
#ifdef DEBUG
   printf(" ---in get_next_line()---\n");
#endif
   do
      {
      aline = fgets(aline, INPUT_LINE_LEN, fp);
      if (aline == NULL)  return ENDOFFILE;
      }
   while (aline[0] == '#');

#ifdef DEBUG
   printf(" ---end get_next_line()---\n");
#endif
   return OK;
   }  /* get_next_line */

/********** set_param **********/
/* parameters:  aline			temp string for parsing comments
   called by:   read_params(), params.c
   actions:     set the parameter value specified in aline.
*/
int set_param(char *aline)
   {
   /* Long absolute paths (Run_num_file, Output_path) exceed 100 chars on macOS */
   char name[INPUT_LINE_LEN];
   char temp[INPUT_LINE_LEN];

#ifdef DEBUG
   printf(" ---in set_param()--- \n");
#endif

   if (sscanf(aline, "%511s %511s", name, temp) < 1)
      return ERROR;

   if (!strcmp(name, "Rerun"))  sscanf(temp, "%d", &Rerun);
   else if (!strcmp(name, "Run_num_file"))  sscanf(temp, "%511s", Run_num_file);
   else if (!strcmp(name, "Output_path"))  sscanf(temp, "%511s", Output_path);
   else if (!strcmp(name, "Print_params"))  sscanf(temp, "%d", &Print_params);
   else if (!strcmp(name, "Print_step"))  sscanf(temp, "%d", &Print_step);
   else if (!strcmp(name, "Max_steps"))  sscanf(temp, "%d", &Max_steps);
   else if (!strcmp(name, "Pop_size"))  sscanf(temp, "%d", &Pop_size);
   else if (!strcmp(name, "Num_tasks"))  sscanf(temp, "%d", &Num_tasks);
   else if (!strcmp(name, "Task_demand_pattern"))  sscanf(temp, "%511s", Task_demand_pattern);
   else if (!strcmp(name, "Task_opposition_mode"))  sscanf(temp, "%d", &Task_opposition_mode);
   else if (!strcmp(name, "Thresh_init"))  sscanf(temp, "%lf", &Thresh_init);
   else if (!strcmp(name, "Thresh_dynamic"))  sscanf(temp, "%d", &Thresh_dynamic);
   else if (!strcmp(name, "Thresh_dynamic_init"))  sscanf(temp, "%d", &Thresh_dynamic_init);
   else if (!strcmp(name, "Thresh_increase"))  sscanf(temp, "%lf", &Thresh_increase);
   else if (!strcmp(name, "Thresh_decrease"))  sscanf(temp, "%lf", &Thresh_decrease);
   else if (!strcmp(name, "Animate_thresh"))  sscanf(temp, "%d", &Animate_thresh);
   else if (!strcmp(name, "Animate_stepwise"))  sscanf(temp, "%d", &Animate_stepwise);
   else if (!strcmp(name, "Prob_check"))  sscanf(temp, "%lf", &Prob_check);
   else if (!strcmp(name, "Task_selection"))  sscanf(temp, "%511s", Task_selection);
   else if (!strcmp(name, "Target_path"))  sscanf(temp, "%511s", Target_path);
   else if (!strcmp(name, "Circle_radius"))  sscanf(temp, "%lf", &Circle_radius);
   else if (!strcmp(name, "Edge_length")) sscanf(temp, "%lf", &Edge_length);
   else if (!strcmp(name, "Target_step_len"))  sscanf(temp, "%lf", &Target.step_len);
   else if (!strcmp(name, "Path_amplitude"))  sscanf(temp, "%lf", &Target.amplitude);
   else if (!strcmp(name, "Path_period"))  sscanf(temp, "%lf", &Target.period);
   else if (!strcmp(name, "Range"))  sscanf(temp, "%lf", &Range);
   else if (!strcmp(name, "Step_ratio"))  sscanf(temp, "%lf", &Tracker.step_ratio);
   /* HDM; intensity variation; 2019.09.12 */
   else if (!strcmp(name, "Intensity_variation"))  sscanf(temp, "%d", &Intensity_variation);
   /* HDM; intensity variation; 2019.09.19 */
   else if (!strcmp(name, "Intensity_aging"))  sscanf(temp, "%d", &Intensity_aging);
   else if (!strcmp(name, "Intensity_aging_min"))  sscanf(temp, "%lf", &Intensity_aging_min);
   else if (!strcmp(name, "Intensity_aging_max"))  sscanf(temp, "%lf", &Intensity_aging_max);
   else if (!strcmp(name, "Intensity_aging_up"))  sscanf(temp, "%lf", &Intensity_aging_up);
   else if (!strcmp(name, "Intensity_aging_down"))  sscanf(temp, "%lf", &Intensity_aging_down);
   else if (!strcmp(name, "Intensity_distribution")) sscanf(temp, "%d", &Intensity_distribution);
   /* LR; heterogeneous intensity ranges; 2020.02.08 */
   else if (!strcmp(name, "Hetero_center_distr")) sscanf(temp, "%d", &Hetero_center_distr);
   else if (!strcmp(name, "Hetero_radius_distr")) sscanf(temp, "%d", &Hetero_radius_distr);
   /* LR; heterogeneous intensity ranges; 2020.02.27 */
   else if (!strcmp(name, "Hetero_range_max")) sscanf(temp, "%lf", &Hetero_range_max);
   else if (!strcmp(name, "Hetero_range_min")) sscanf(temp, "%lf", &Hetero_range_min);
   else if (!strcmp(name, "Hetero_radius_max")) sscanf(temp, "%lf", &Hetero_radius_max);
   else if (!strcmp(name, "Hetero_radius_min")) sscanf(temp, "%lf", &Hetero_radius_min);
   /* LR; heterogeneous intensity ranges; 2020.02.15 */
   else if (!strcmp(name, "Hetero_center_mu")) sscanf(temp, "%lf", &Hetero_center_mu);
   else if (!strcmp(name, "Hetero_center_std")) sscanf(temp, "%lf", &Hetero_center_std);
   else if (!strcmp(name, "Hetero_radius_mu")) sscanf(temp, "%lf", &Hetero_radius_mu);
   else if (!strcmp(name, "Hetero_radius_std")) sscanf(temp, "%lf", &Hetero_radius_std);
//rp  220521ASW replace with dynamic response probability
//rp   else if (!strcmp(name, "Response_prob")) sscanf(temp, "%lf", &Response_prob);
   else if (!strcmp(name, "Kill_number")) sscanf(temp, "%d", &Kill_number);
   else if (!strcmp(name, "First_extinction")) sscanf(temp, "%d", &First_extinction);
   else if (!strcmp(name, "Extinction_period")) sscanf(temp, "%d", &Extinction_period);
   else if (!strcmp(name, "Removal_capacity_mode")) sscanf(temp, "%d", &Removal_capacity_mode);
   /* NB; spontaneous response probs; 2020.05.19 */
   else if (!strcmp(name, "Spontaneous_response_prob")) sscanf(temp, "%lf", &Spontaneous_response_prob);
   else if (!strcmp(name, "RP_gaussian_mu")) sscanf(temp, "%lf", &RP_gaussian_mu);
   else if (!strcmp(name, "RP_gaussian_std")) sscanf(temp, "%lf", &RP_gaussian_std);
   else if (!strcmp(name, "SRP_gaussian_mu")) sscanf(temp, "%lf", &SRP_gaussian_mu);
   else if (!strcmp(name, "SRP_gaussian_std")) sscanf(temp, "%lf", &SRP_gaussian_std);
   else if (!strcmp(name, "Gnuplot_plots"))  sscanf(temp, "%d", &Gnuplot_plots);
   else if (!strcmp(name, "Prob_dynamic"))  sscanf(temp, "%d", &Prob_dynamic);
   else if (!strcmp(name, "Prob_dynamic_init")) sscanf(temp, "%lf", &Prob_dynamic_init);
   else if (!strcmp(name, "Prob_increase")) sscanf(temp, "%lf", &Prob_increase);
   else if (!strcmp(name, "Prob_decrease")) sscanf(temp, "%lf", &Prob_decrease);
   else if (!strcmp(name, "Prob_idle_increase")) sscanf(temp, "%lf", &Prob_idle_increase);
   else if (!strcmp(name, "Prob_dynamic_max")) sscanf(temp, "%lf", &Prob_dynamic_max);
   else if (!strcmp(name, "Prob_dynamic_min")) sscanf(temp, "%lf", &Prob_dynamic_min);
   else if (!strcmp(name, "P_gain"))  sscanf(temp, "%lf", &P_gain);
   else if (!strcmp(name, "I_gain"))  sscanf(temp, "%lf", &I_gain);
   else if (!strcmp(name, "D_gain"))  sscanf(temp, "%lf", &D_gain);
   else if (!strcmp(name, "Pid"))  sscanf(temp, "%d", &Pid);
   else if (!strcmp(name, "Pid_latent_thresholds"))  sscanf(temp, "%d", &Pid_latent_thresholds);
   else if (!strcmp(name, "Pid_integral_leak"))  sscanf(temp, "%lf", &Pid_integral_leak);
   else if (!strcmp(name, "Pid_integral_bound"))  sscanf(temp, "%lf", &Pid_integral_bound);
   else if (!strcmp(name, "Agent_pid_gains"))  sscanf(temp, "%d", &Agent_pid_gains);
   else if (!strcmp(name, "Agent_pid_gain_spread"))
      {
      sscanf(temp, "%lf", &Agent_pid_gain_spread);
      Agent_pid_p_spread = Agent_pid_gain_spread;
      Agent_pid_i_spread = Agent_pid_gain_spread;
      Agent_pid_d_spread = Agent_pid_gain_spread;
      }
   else if (!strcmp(name, "Agent_pid_p_spread"))  sscanf(temp, "%lf", &Agent_pid_p_spread);
   else if (!strcmp(name, "Agent_pid_i_spread"))  sscanf(temp, "%lf", &Agent_pid_i_spread);
   else if (!strcmp(name, "Agent_pid_d_spread"))  sscanf(temp, "%lf", &Agent_pid_d_spread);
   else if (!strcmp(name, "Agent_pid_gain_apply_id"))  sscanf(temp, "%d", &Agent_pid_gain_apply_id);
   else if (!strcmp(name, "Feedback_noise_enabled"))  sscanf(temp, "%d", &Feedback_noise_enabled);
   else if (!strcmp(name, "Feedback_noise_alpha"))  sscanf(temp, "%lf", &Feedback_noise_alpha);
   else if (!strcmp(name, "Feedback_noise_seed"))  sscanf(temp, "%ld", &Feedback_noise_seed);
   else if (!strcmp(name, "Feedback_noise_clip"))
      {
      if (!strcmp(temp, "population")) Feedback_noise_clip = 1;
      else sscanf(temp, "%d", &Feedback_noise_clip);
      }
   else if (!strcmp(name, "Feedback_bias_alpha"))  sscanf(temp, "%lf", &Feedback_bias_alpha);
   else if (!strcmp(name, "Feedback_bias_seed"))  sscanf(temp, "%ld", &Feedback_bias_seed);
   else if (!strcmp(name, "Feedback_bias_mode"))
      {
      if (!strcmp(temp, "none")) Feedback_bias_mode = 0;
      else if (!strcmp(temp, "task_fixed_random")) Feedback_bias_mode = 1;
      else if (!strcmp(temp, "positive")) Feedback_bias_mode = 2;
      else sscanf(temp, "%d", &Feedback_bias_mode);
      }
   else if (!strcmp(name, "Feedback_noise_sigma_mode"))
      {
      if (strcmp(temp, "mean_active_task_demand"))
         {
         printf(" Error(set_param): unsupported Feedback_noise_sigma_mode: %s\n", temp);
         return ERROR;
         }
      }
   else if (!strcmp(name, "FixedTargetPath"))  sscanf(temp, "%d", &FixedTargetPath);
   else if (!strcmp(name, "TargetPathSeed"))  sscanf(temp, "%lu", &TargetPathSeed);
   else if (!strcmp(name, "Demand_segment_len"))  sscanf(temp, "%d", &Demand_segment_len);
   else if (!strcmp(name, "Demand_switch_step"))  sscanf(temp, "%d", &Demand_switch_step);
   else
      {
      printf(" Error(set_param): unknown param type: %s\n", name);
      return ERROR;
      }  /* else bad parameter name */

#ifdef DEBUG
   printf(" ---end set_param()---\n");
#endif
   return OK;
   }  /* set_param */

/********** read_default_opfiles **********/
/* parameters:  opfiles_file
   called by:   ga_init(), ga.c
                read_defaults, params.c
   actions:     Read from specified opfiles file, one line at a time.
		Set up the output files as specified in file.
		First number indicates if file is to be printed;
		second number indicates if header is to be printed.
		(1 = yes, 0 = no)  Second number is ignored if first is 0.

                Skip the comment lines which are denoted
                by a "#" or "%" xter in the first column.
		Each line contains the extension of an output file
		and a 1 if it is supposed to be printed during the
		run or 0 if it is not to be printed.
*/
int read_default_opfiles(char *opfiles_file)
   {
   char *aline;
   FILE *fp;
   int i;

#ifdef DEBUG
   printf(" ---in read_default_opfiles()--- %s\n", opfiles_file);
#endif

  /* open file */
   fp = fopen(opfiles_file, "r");
   if (fp == NULL)
      {
      printf(" Error(read_default_opfiles): cannot open file: %s\n",
		opfiles_file);
      return ERROR;
      }  /* if */
   printf(" Reading from file: %s\n", opfiles_file);

  /* allocate space for aline */
   aline = (char *)malloc(INPUT_LINE_LEN * sizeof(char));

  /* first line should give the maximum number of possible output files */
  /* allocate needed array space */
   if (get_next_line(fp, aline) != ENDOFFILE)
      {
      sscanf(aline, "%d", &Max_num_output_files);
      Output_file = (OUTPUT_FILE *)malloc(Max_num_output_files *
			sizeof(OUTPUT_FILE));
      }  /* if */

  /* now read rest of the non-comment lines up to above specified
     number of files -- any additional lines are ignored */
   i = 0;
   while (get_next_line(fp, aline) != ENDOFFILE && i < Max_num_output_files)
      {
      Output_file[i].extension = (char *)malloc(INPUT_LINE_LEN * sizeof(char));
      sscanf(aline, "%s %d", Output_file[i].extension, &Output_file[i].on);
      i++;
      }  /* while */

   free(aline);
   fclose(fp);

#ifdef DEBUG
   printf(" ---end read_default_opfiles()---\n");
#endif
   return OK;
   }  /* read_default_opfiles */

/********** read_opfiles **********/
/* parameters:  opfiles_file
   called by:   ga_init(), ga.c
                read_defaults, params.c
   actions:     Read from specified opfiles file, one line at
                a time.
                Skip the comment lines which are denoted
                by a "#" or "%" xter in the first column.
		Each line contains the extension of an output file
		and a 1 if it is supposed to be printed during the
		run or 0 if it is not to be printed.
*/
int read_opfiles(char *opfiles_file)
   {
   char *aline;
   FILE *fp;
   char *extension;
   int on;
   int i;

#ifdef DEBUG
   printf(" ---in read_opfiles()--- %s\n", opfiles_file);
#endif

  /* open file */
   fp = fopen(opfiles_file, "r");
   if (fp == NULL)
      {
      printf(" Error(read_default_opfiles): cannot open file: %s\n",
		opfiles_file);
      return ERROR;
      }  /* if */
   printf(" Reading from file: %s\n", opfiles_file);

  /* allocate space for aline */
   aline = (char *)malloc(INPUT_LINE_LEN * sizeof(char));
   extension = (char *)malloc(INPUT_LINE_LEN * sizeof(char));

   while (get_next_line(fp, aline) != ENDOFFILE)
      {
      sscanf(aline, "%s %d", extension, &on);
      i = get_file_pointer(extension);
      if (i == -1)
         {
         printf(" Error(read_opfiles): invalid extension: %s\n", extension);
         return ERROR;
         }  /* if */
      sprintf(Output_file[i].extension, "%s", extension);
      Output_file[i].on = on;
      }  /* while */

   free(aline);
   free(extension);
   fclose(fp);
#ifdef DEBUG
   printf(" ---end read_opfiles()---\n");
#endif
   return OK;
   }  /* read_opfiles */

/********** get_file_pointer **********/
/* parameters:	extension	of desired file
   called by:	read_opfiles(), params.c
   actions:	given an extension, goes thru Output_file and returns
		the array pointer to that file's array element.
		Returns -1 of not found.
*/
int get_file_pointer(char *extension)
   {
   int i;

   for (i=Max_num_output_files-1; i>=0; i--)
      {
      if (!strcmp(extension, Output_file[i].extension))  return i;
      }  /* for i */
   return -1;
   }  /* get_file_pointer */

/********** setup_output_files **********/
/* parameters:  aline           line of data on one file
                i               counter to Output_file
   called by:   ga_init(), ga.c
   actions:     Goes thru list of all possible output files
		(read from opfiles.default), and sets up Output_file[i].fp
		for those that are "on" for the current run.
		All Output_file[i].extension and .on entries should
		contain data, just need to decide which ones need .fp
		to be initialized.
*/
int setup_output_files()
   {
   int i;

#ifdef DEBUG
   printf(" ---in setup_output_files()---\n");
#endif

  /* i starts at 1 because array element 0 should always be for
     for the random file (so random should always be the first
     file extension specified in opfiles.default).  This is so
     that the random file can be decided on in get_random_seed()
     in init.c rather than here. */
   for (i=1; i<Max_num_output_files; i++)
      {
      /* if output file is on, set up .fp, otherwise do nothing */
      if (Output_file[i].on)
         {
         Output_file[i].filename = (char *)malloc(INPUT_LINE_LEN*sizeof(char));
         sprintf(Output_file[i].filename, "%s/run.%d/run.%d.%s",
		Output_path, Run_num, Run_num, Output_file[i].extension);
         Output_file[i].fp = fopen(Output_file[i].filename, "w");
         if (Output_file[i].fp == NULL)
            {
            printf(" Error(set_up_file): cannot open file: %s\n",
                           Output_file[i].filename);
            return ERROR;
            }  /* if */
         fclose(Output_file[i].fp);
         }  /* if */
      }  /* for i */

#ifdef DEBUG
   printf(" ---end setup_output_files()---\n");
#endif
   return OK;
   }  /* setup_output_files */

/********** file_on **********/
/* parameters:  extension       of desired file
   called by:  genops.c
   actions:     given an extension, goes thru Output_file and returns
		1 if that file is on (being printed to for this run)
		or 0 if that file is off (not to be printed to).
*/
int file_on(char *extension)
   {
   int i;

   for (i=Max_num_output_files-1; i>=0; i--)
      {
      if (!strcmp(extension, Output_file[i].extension))
         {
         if (Output_file[i].on)
            {
            return 1;
            }
         else
            {
            return 0;
            }
         }
      }  /* for i */
   printf(" Error(file_on): unknown file extension: %s\n", extension);
   return 0;
   }  /* file_on */
