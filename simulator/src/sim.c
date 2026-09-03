/* sim.c
   19.07.12.AW	Created.
*/

#define _POSIX_C_SOURCE 199309L
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "types.h"
#include "global.h"
#include "sim.h"
#include "output.h"
#include "params.h"
#include "random.h"
#include "fxn.h"
#include "gnu.h"
#include "ftarget.h"

/********** run_sim **********/
/* parameters:
   called by:	main(), main.c
   actions:
   returns:	error flag
*/
int run_sim(char *params_file, char *opfiles_file)
   {
   int error;

#ifdef DEBUG
printf("---in run_sim()---\n");
#endif

  /* initializations for setting up the simulation, opfiles, seed, etc. */
   error = init_sim(params_file, opfiles_file);
   if (error == ERROR)  return ERROR;

  /* problem initialization -- in this case setting up the preferences
     in a structure, setting up the population and space. */
      printf(" run init_fxn()\n");
   error = init_fxn();
   if (error == ERROR)  return ERROR;

  /* run the simulation */
   error = run_fxn();
   if (error == ERROR)  return ERROR;

  /* end the simulation */
   error = end_sim();
   if (error == ERROR)  return ERROR;

   printf(" *** End of run %d\n", Run_num);

#ifdef DEBUG
printf("---end run_sim()---\n");
#endif
   return OK;
   }  /* run_sim */

/********** init_sim **********/
/* parameters:  params_file     list of run parameters
                opfiles_file    list of output files
   called by:   run_sim(), sim.c
   actions:     Initialize a run.
*/
int init_sim(char *params_file, char *opfiles_file)
   {
   int ptr;
#ifdef DEBUG
   printf(" ---in init_sim()---\n");
#endif

  /* allocate space */
   Output_path = (char *)malloc(INPUT_LINE_LEN * sizeof(char));
   Run_num_file = (char *)malloc(INPUT_LINE_LEN * sizeof(char));
   Target_path = (char *)malloc(INPUT_LINE_LEN * sizeof(char));
   Task_selection = (char *)malloc(INPUT_LINE_LEN * sizeof(char));
   Task_demand_pattern = (char *)malloc(INPUT_LINE_LEN * sizeof(char));

  /* read default parameter values and desired outputfiles */
   if (read_defaults() == ERROR)  return ERROR;

  /* read overriding parameter values and outputfiles */
   if (read_params(params_file) == ERROR)  return ERROR;
   if (read_opfiles(opfiles_file) == ERROR)  return ERROR;

  /* Defaults for demand paths if omitted from params (BSS zero). */
   if (Demand_segment_len <= 0)
      Demand_segment_len = 200;
   if (Demand_switch_step <= 0)
      Demand_switch_step = 500;

  /* Initialize only after params have supplied FixedTargetPath and
     TargetPathSeed.  This stream is independent of agent randomness. */
   init_target_path_rng();

  /* get new run number -- index to identify run */
   if (get_run_num() == ERROR)  return ERROR;

  /* create output directory (based on run number) */
   if (make_directory() == ERROR)  return ERROR;

  /* set up random number generator */
   if (get_random_seed() == ERROR)  return ERROR;

  /* set up output files */
   if (setup_output_files() == ERROR)  return ERROR;

  /* print parameters to file if flag is on */
   if (file_on("params"))
      {
      ptr = get_file_pointer("params");
      Output_file[ptr].fp = fopen(Output_file[ptr].filename, "a");
      print_params(Output_file[ptr].fp);
      fclose(Output_file[ptr].fp);
      }  /* if */

  /* print parameters to screen if flag is on */
   if (Print_params)
      {
      print_params(stdout);
      printf("\n");
      print_opfiles(stdout);
      printf("\n");
      }

#ifdef DEBUG
   printf(" ---end init_sim()---\n");
#endif
   return OK;
   }  /* init_sim */

/********** get_run_num **********/
/* parameters:
   called by:   init_run(), init.c
   actions:     Get run number for this run.
                Reads index of previous run from Run_num_file,
                increments to get current run number, and writes
                current run number back to Run_num_file.

                The program will always assign a new number to each
                new run, even if the user is rerunning an old run.
                Rerunning an old run simply makes the program read
                in the random seed for the run from a user specified
                file.  The run still gets a new number so that it will
                not conflict with existing files and directories in
                the output directory.
*/
int get_run_num()
   {
   FILE *fp;

  /* read previous run number */
   fp = fopen(Run_num_file, "r");
   if (fp == NULL)
      {
      printf(" Error(get_run_num): cannot open run file: %s\n", Run_num_file);
      return ERROR;
      }
   fscanf(fp, "%d", &Run_num);
   fclose(fp);

  /* assign and write back new number */
   Run_num = Run_num + 1;
   fp = fopen(Run_num_file, "w");
   if (fp == NULL)
      {
      printf(" Error(get_run_num): cannot open run file: %s\n", Run_num_file);
      return ERROR;
      }
   fprintf(fp, "%d\n", Run_num);
   fclose(fp);

   return OK;
   }  /* get_run_num */

/********** make_directory **********/
/* parameters:
   called by:   init_run(), init.c
   actions:     make directory for output files from this run
                within the Output_path directory.
                Directory will be called run.<Run_num>
*/
int make_directory()
   {
   char tempstring[INPUT_LINE_LEN];
   sprintf(tempstring, "%s/run.%d", Output_path, Run_num);
   if (mkdir(tempstring, 0775) != 0)
      {
      if (errno == EEXIST)
         {
         printf("Error(make_directory): directory already exists: %s\n", tempstring);
         }
      else
         {
         printf("Error(make_directory): cannot create directory: %s\n", tempstring);
         perror("mkdir");
         }
      return ERROR;
      }
   return OK;
   }  /* make_directory */

/********** get_random_seed **********/
/* parameters:
   called by:   init_run(), init.c
   actions:     Get random seed for this run.
                If Rerun = -1, the generate a new random seed for this run.
                If Rerun >= 0, then its values specifies the run that we
                want to rerun.  Read the random seed from that run's random
                seed file, i.e. run.<Rerun>.random.

                Need to have already called get_run_num() before calling
                this program because need run number to write random seed
                to file called run.<Run_num>.random.
*/
int get_random_seed()
   {
   FILE *fp;
   char tempstring[INPUT_LINE_LEN];
   long temp;
//   int extra;
   struct timespec ts;

   if (Rerun < 0)
      {
      clock_gettime(CLOCK_MONOTONIC, &ts);
      Seed = seed_random((long)ts.tv_nsec);

printf(" Seed %ld nsec %ld\n", Seed, ts.tv_nsec);

//      // add run number to seed in case run is too short that next random seed
//      // is based on the same time clock   161122.AW
//      fp = fopen("run.num", "r");
//      if (fp == NULL)
//         {
//         printf(" Error(get_random_seed): cannot open file: run.num\n");
//         return ERROR;
//         }
//      fscanf(fp, "%d", &extra);
//      fclose(fp);
//      Seed = seed_random((long)(Rerun+extra*831293) );
      }  /* if generate new seed */
   else
      {
      sprintf(tempstring, "%s/run.%d/run.%d.random",
                Output_path, Rerun, Rerun);
      fp = fopen(tempstring, "r");
      if (fp == NULL)
         {
         printf(" Error(get_random_seed): cannot open file: %s\n", tempstring);
         return ERROR;
         }
      fscanf(fp, "%ld", &temp);
printf(" seed read in from %s is %ld\n", tempstring, temp);
      Seed = seed_random(temp);
      fclose(fp);
      }  /* else read old seed */

  /* print seed to random file for this run if "random" opfiles
     flag is turned on */
   if (Output_file[get_file_pointer("random")].on)
      {
      sprintf(tempstring, "%s/run.%d/run.%d.random",
             Output_path, Run_num, Run_num);
      fp = fopen(tempstring, "w");
      if (fp == NULL)
         {
         printf(" Error(get_random_seed): cannot open file: %s\n", tempstring);
         return ERROR;
         }
      fprintf(fp, "%ld\n", Seed);
      fclose(fp);
      }  /* if print to random file */

//printf(" get_random_seed():  Seed = %ld\n", Seed);

   return OK;
   }  /* get_random_seed */

/********** end_sim **********/
/* parameters:
   called by:   run_sim(), sim.c
   actions:     End a run.
*/
int end_sim()
   {
   int error;

#ifdef DEBUG
   printf(" ---in end_sim()---\n");
#endif

   /* print run data */
   run_output();
   if (Gnuplot_plots)  plot_gnuplot_files();

  /* finish up the function */
   error = end_fxn();
   if (error == ERROR)  return ERROR;

   free(Output_path);
   free(Run_num_file);
   free(Target_path);
   free(Task_demand_pattern);
   free(Task_selection);

#ifdef DEBUG
   printf(" ---end end_sim()---\n");
#endif
   return OK;
   }  /* end_sim */
