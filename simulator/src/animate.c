/* animate.c
   20.12.11.AW  Created.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "extern.h"
#include "animate.h"

/********** fprint_animate_thresh_data ***********/
/* Called by:           step_output(), output.c
   Parameters:		t		current timestep
   Actions:             Print output file each timestep with threshold,
			action count, and switch data for that timestep.
*/
void fprint_animate_thresh_data(int t)
   {
   char filename[INPUT_LINE_LEN];
   FILE *fp;
   int i;

#ifdef DEBUG
printf("---in fprint_animate_thresh_data()---\n");
#endif

   sprintf(filename, "%s/run.%d/run.%d.animatethresh.step%d",
	Output_path, Run_num, Run_num, t);
   fp = fopen(filename, "w");

   for (i=0; i<Pop_size; i++)
      {
      fprintf(fp, " agent %4d count-inesw %4d %4d %4d %4d %4d",
              i,
              Agent[i].count_idle,
              Agent[i].count_north,
              Agent[i].count_east, 
              Agent[i].count_south,
              Agent[i].count_west);
      fprintf(fp, " thresh-nesw %lf %lf %lf %lf",
              // Agent[i].raw_thresh_north,
              // Agent[i].raw_thresh_east,
              // Agent[i].raw_thresh_south,
              // Agent[i].raw_thresh_west);
              Agent[i].thresh_north,
              Agent[i].thresh_east,
              Agent[i].thresh_south,
              Agent[i].thresh_west);
      fprintf(fp, " switch %d\n", Agent[i].count_switch);
      } 
   fclose(fp);

#ifdef DEBUG
printf("---end fprint_animate_thresh_data()---\n");
#endif
   }  /* fprint_animate_thresh_data */

/********** fprint_animate_threshact_gnu ***********/
/* Called by:           step_output(), output.c
   Parameters:          
   Actions:             Print the gnuplot for animating the threshold versus
                        action count for each timestep.
*/
void fprint_animate_threshact_gnu()
   {
   char filename[INPUT_LINE_LEN];
   FILE *fp;
   int t;

#ifdef DEBUG
printf("---in fprint_animate_threshact_gnu()---\n");
#endif

   sprintf(filename, "%s/run.%d/run.%d.animatethreshact.gnu",
        Output_path, Run_num, Run_num);
   fp = fopen(filename, "w");

   fprintf(fp, "# This will only work on an X11 terminal.\n");
   fprintf(fp, "set term x11\n");
   fprintf(fp, "set xlabel \"Threshold value\"\n");
   fprintf(fp, "set ylabel \"Action count\"\n");

   // don't start the animation automatically
   // user needs to press a key to start the simulation
   t = 0;
      fprintf(fp, "\n");
      fprintf(fp, "# Plot timestep 0, then wait for user to start animation.\n");
      fprintf(fp, "# Timestep %d\n", t);

      fprintf(fp, "set title \"Timestep %d: Threshold values versus number times acting on task\"\n", t);
      fprintf(fp, "plot [%lf:%lf][0:%lf]\\\n",
                -0.05*Range, 1.05*Range, Max_steps*1.1);
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 10:5 title \"N\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 1,\\\n");
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 11:6 title \"E\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 5,\\\n");
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 12:7 title \"S\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 7,\\\n");
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 13:8 title \"W\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 2\n");
      fprintf(fp, "pause -1 \"Press any key to start\"\n");

   for (t=0; t<Max_steps; t++)
      {
      fprintf(fp, "\n");
      fprintf(fp, "# Timestep %d\n", t);

      fprintf(fp, "set title \"Timestep %d: Threshold values versus number times acting on task\"\n", t);
      fprintf(fp, "plot [%lf:%lf][0:%lf]\\\n",
                -0.05*Range, 1.05*Range, Max_steps*1.1);
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 10:5 title \"N\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 1,\\\n");
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 11:6 title \"E\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 5,\\\n");
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 12:7 title \"S\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 7,\\\n");
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 13:8 title \"W\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 2\n");

      if (Animate_stepwise == 0)
         {
         fprintf(fp, "pause 0.1 \"Timestep %d\"\n", t);
         }
      else if (Animate_stepwise == 1)
         {
         fprintf(fp, "pause -1 \"Press any key to start\"\n");
         }
      else
         {
         printf(" Error(fprint_animate_threshact_gnu):  Invalid value for Animate_stepwise %d\n", Animate_stepwise);
         }
      }

   t = Max_steps-1;
      fprintf(fp, "\n");
      fprintf(fp, "# Keep last timestep visible until user exits sim\n");
      fprintf(fp, "# Timestep %d\n", t);

      fprintf(fp, "set title \"Timestep %d: Threshold values versus number times acting on task\"\n", t);
      fprintf(fp, "plot [%lf:%lf][0:%lf]\\\n",
                -0.05*Range, 1.05*Range, Max_steps*1.1);
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 10:5 title \"N\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 1,\\\n");
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 11:6 title \"E\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 5,\\\n");
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 12:7 title \"S\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 7,\\\n");
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 13:8 title \"W\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 2\n");

   // for some reason I need this twice to pause the image
   // and keep it from automatically disappearing
   fprintf(fp, "pause -1 \"Press any key to continue\"\n");
   fprintf(fp, "pause -1 \"Press any key to continue\"\n");

   fclose(fp);

#ifdef DEBUG
printf("---end fprint_animate_threshact_gnu()---\n");
#endif
   }  /* fprint_animate_threshact_gnu */

/********** fprint_animate_threshswitch_gnu ***********/
/* Called by:           step_output(), output.c
   Parameters:
   Actions:             Print the gnuplot for animating the threshold versus
                        switch count for each timestep.
*/
void fprint_animate_threshswitch_gnu()
   {
   char filename[INPUT_LINE_LEN];
   FILE *fp;
   int t;

#ifdef DEBUG
printf("---in fprint_animate_threshswitch_gnu()---\n");
#endif

   sprintf(filename, "%s/run.%d/run.%d.animatethreshswitch.gnu",
        Output_path, Run_num, Run_num);
   fp = fopen(filename, "w");

   fprintf(fp, "# This will only work on an X11 terminal.\n");
   fprintf(fp, "set term x11\n");
   fprintf(fp, "set xlabel \"Threshold value\"\n");
   fprintf(fp, "set ylabel \"Switch count\"\n");

   // don't start the animation automatically
   // user needs to press a key to start the simulation
   t = 0;
      fprintf(fp, "\n");
      fprintf(fp, "#Timestep %d\n", t); 
      fprintf(fp, "set title \"Timestep %d: Threshold values versus switch count\"\n", t);
      fprintf(fp, "plot [%lf:%lf][0:%lf]\\\n", 
                -0.05*Range, 1.05*Range, Max_steps*1.1);
      fprintf(fp, "   \"run.%d.animatethresh.step%d\" using 10:15 title \"N\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 1,\\\n"); 
      fprintf(fp, "   \"run.%d.animatethresh.step%d\" using 11:15 title \"E\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 5,\\\n"); 
      fprintf(fp, "   \"run.%d.animatethresh.step%d\" using 12:15 title \"S\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 7,\\\n"); 
      fprintf(fp, "   \"run.%d.animatethresh.step%d\" using 13:15 title \"W\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 2\n");
      fprintf(fp, "pause -1 \"Press any key to continue\"\n");

   for (t=0; t<Max_steps; t++)
      {
      fprintf(fp, "\n");
      fprintf(fp, "#Timestep %d\n", t);
      fprintf(fp, 
        "set title \"Timestep %d: Threshold values versus switch count\"\n", t);
      fprintf(fp, "plot [%lf:%lf][0:%lf]\\\n", 
                -0.05*Range, 1.05*Range, Max_steps*1.1);
      fprintf(fp, "   \"run.%d.animatethresh.step%d\" using 10:15 title \"N\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 1,\\\n"); 
      fprintf(fp, "   \"run.%d.animatethresh.step%d\" using 11:15 title \"E\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 5,\\\n"); 
      fprintf(fp, "   \"run.%d.animatethresh.step%d\" using 12:15 title \"S\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 7,\\\n"); 
      fprintf(fp, "   \"run.%d.animatethresh.step%d\" using 13:15 title \"W\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 2\n");

      if (Animate_stepwise == 0)
         {
         fprintf(fp, "pause 0.1 \"Timestep %d\"\n", t);
         }
      else if (Animate_stepwise == 1)
         {
         fprintf(fp, "pause -1 \"Press any key to start\"\n");
         }
      else
         {
         printf(" Error(fprint_animate_threshswitch_gnu):  Invalid value for Animate_stepwise %d\n", Animate_stepwise);
         }
      }

   t = Max_steps-1;
      fprintf(fp, "\n");
      fprintf(fp, "#Timestep %d\n", t); 
      fprintf(fp, "set title \"Timestep %d: Threshold values versus switch count\"\n", t);
      fprintf(fp, "plot [%lf:%lf][0:%lf]\\\n", 
                -0.05*Range, 1.05*Range, Max_steps*1.1);
      fprintf(fp, "   \"run.%d.animatethresh.step%d\" using 10:15 title \"N\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 1,\\\n"); 
      fprintf(fp, "   \"run.%d.animatethresh.step%d\" using 11:15 title \"E\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 5,\\\n"); 
      fprintf(fp, "   \"run.%d.animatethresh.step%d\" using 12:15 title \"S\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 7,\\\n"); 
      fprintf(fp, "   \"run.%d.animatethresh.step%d\" using 13:15 title \"W\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 2\n");

   // for some reason I need this twice to pause the image
   // and keep it from automatically disappearing
   fprintf(fp, "pause -1 \"Press any key to continue\"\n");
   fprintf(fp, "pause -1 \"Press any key to continue\"\n");

   fclose(fp);

#ifdef DEBUG
printf("---end fprint_animate_threshswitch_gnu()---\n");
#endif
   }  /* fprint_animate_threshswitch_gnu */

/********** fprint_animate_path_data ***********/
/* Called by:           step_output(), output.c
   Parameters:          t               current timestep
   Actions:             Print output file each timestep with the path so far.
*/
void fprint_animate_path_data(int t)
   {
   char oldfile[INPUT_LINE_LEN];
   char newfile[INPUT_LINE_LEN];
   char cmd[INPUT_LINE_LEN];
   FILE *fp;
   int i;

#ifdef DEBUG
printf("---in fprint_animate_path_data()---\n");
#endif

   if (t == 0)
      {
      sprintf(newfile, "%s/run.%d/run.%d.animatepath.step0",
           Output_path, Run_num, Run_num);
      fp = fopen(newfile, "w");
      fprintf(fp, " t %4d target %lf %lf tracker %lf %lf\n", t,
		Target.x, Target.y, Tracker.x, Tracker.y);
      fclose(fp);
      }
   else
      {
      sprintf(oldfile, "%s/run.%d/run.%d.animatepath.step%d",
           Output_path, Run_num, Run_num, t-1);
      sprintf(newfile, "%s/run.%d/run.%d.animatepath.step%d",
           Output_path, Run_num, Run_num, t);
      sprintf(cmd, "cp %s %s", oldfile, newfile);
      printf(" Executing:  %s\n", cmd);
      system(cmd);

      fp = fopen(newfile, "a");
      fprintf(fp, " t %4d target %lf %lf tracker %lf %lf\n", t,
		Target.x, Target.y, Tracker.x, Tracker.y);
      fclose(fp);
      }

#ifdef DEBUG
printf("---end fprint_animate_path_data()---\n");
#endif
   }  /* fprint_animate_path_data */

/********** fprint_animate_path_gnu ***********/
/* Called by:           step_output(), output.c
   Parameters:
   Actions:             Print the gnuplot for animating the threshold versus
                        action count for each timestep.
*/
void fprint_animate_path_gnu()
   {
   char filename[INPUT_LINE_LEN];
   FILE *fp;
   int t;

#ifdef DEBUG
printf("---in fprint_animate_path_gnu()---\n");
#endif

   sprintf(filename, "%s/run.%d/run.%d.animatepath.gnu",
        Output_path, Run_num, Run_num);
   fp = fopen(filename, "w");

   fprintf(fp, "# This will only work on an X11 terminal.\n");
   fprintf(fp, "set term x11\n");
   fprintf(fp, "set xlabel \"Threshold value\"\n");
   fprintf(fp, "set ylabel \"Action count\"\n");

   t = 0;
      fprintf(fp, "\n");
      fprintf(fp, "# Timestep %d\n", t);

      fprintf(fp, "set title \"Timestep %d: Target and tracker path\"\n", t);
      fprintf(fp, "plot [0:%lf][%lf:%lf]\\\n",
		(1.8 * Target.amplitude/Target.period) * Max_steps, 
		-1.5 * Target.amplitude, 1.5*Target.amplitude);
      fprintf(fp, "    \"run.%d.animatepath.step%d\" using 4:5 title ",
                Run_num, t);
      fprintf(fp,       "\"Target\" w linesp,\\\n");
      fprintf(fp, "    \"run.%d.animatepath.step%d\" using 7:8 title ",
                Run_num, t);
      fprintf(fp,       "\"Tracker\" w linesp\n");
   fprintf(fp, "pause -1 \"Press any key to start\"\n");

   for (t=0; t<Max_steps; t++)
      {
      fprintf(fp, "\n");
      fprintf(fp, "# Timestep %d\n", t);

      fprintf(fp, "set title \"Timestep %d: Target and tracker path\"\n", t);
      fprintf(fp, "plot [0:%lf][%lf:%lf]\\\n",
		(1.8 * Target.amplitude/Target.period) * Max_steps, 
		-1.5 * Target.amplitude, 1.5*Target.amplitude);
      fprintf(fp, "    \"run.%d.animatepath.step%d\" using 4:5 title ",
                Run_num, t);
      fprintf(fp,       "\"Target\" w linesp,\\\n");
      fprintf(fp, "    \"run.%d.animatepath.step%d\" using 7:8 title ",
                Run_num, t);
      fprintf(fp,       "\"Tracker\" w linesp\n");

      if (Animate_stepwise == 0)
         {
         fprintf(fp, "pause 0.1 \"Timestep %d\"\n", t);
         }
      else if (Animate_stepwise == 1)
         {
         fprintf(fp, "pause -1 \"Press any key to start\"\n");
         }
      else
         {
         printf(" Error(fprint_animate_path_gnu):  Invalid value for Animate_stepwise %d\n", Animate_stepwise);
         }
      }

   t = Max_steps - 1;
      fprintf(fp, "\n");
      fprintf(fp, "# Timestep %d\n", t);

      fprintf(fp, "set title \"Timestep %d: Target and tracker path\"\n", t);
      fprintf(fp, "plot [0:%lf][%lf:%lf]\\\n",
		(1.8 * Target.amplitude/Target.period) * Max_steps, 
		-1.5 * Target.amplitude, 1.5*Target.amplitude);
      fprintf(fp, "    \"run.%d.animatepath.step%d\" using 4:5 title ",
                Run_num, t);
      fprintf(fp,       "\"Target\" w linesp,\\\n");
      fprintf(fp, "    \"run.%d.animatepath.step%d\" using 7:8 title ",
                Run_num, t);
      fprintf(fp,       "\"Tracker\" w linesp\n");

   fprintf(fp, "pause -1 \"Press any key to continue\"\n");
   fprintf(fp, "pause -1 \"Press any key to continue\"\n");

   fclose(fp);

#ifdef DEBUG
printf("---end fprint_animate_path_gnu()---\n");
#endif
   }  /* fprint_animate_path_gnu */

/********** fprint_animate_combo_gnu ***********/
/* Called by:           step_output(), output.c
   Parameters:
   Actions:             Print the gnuplot for animating the threshold versus
                        action count for each timestep.
*/
void fprint_animate_combo_gnu()
   {
   char filename[INPUT_LINE_LEN];
   FILE *fp;
   int t;

#ifdef DEBUG
printf("---in fprint_animate_combo_gnu()---\n");
#endif

   sprintf(filename, "%s/run.%d/run.%d.animatecombo.gnu",
        Output_path, Run_num, Run_num);
   fp = fopen(filename, "w");

   fprintf(fp, "# This will only work on an X11 terminal.\n");
   fprintf(fp, "set term x11\n");

   t = 0;
      fprintf(fp, "set multiplot layout 2,2\n");
      fprintf(fp, "\n");
      fprintf(fp, "# Plot timestep 0, then wait for user to start animation.\n");
      fprintf(fp, "# Timestep %d\n", t);

      fprintf(fp, "set title \"Timestep %d: Threshold versus activation\"\n", t);
      fprintf(fp, "set size 0.5,0.5\n");
      fprintf(fp, "set xlabel \"Threshold value\"\n");
      fprintf(fp, "set ylabel \"Action count\"\n");
      fprintf(fp, "plot [%lf:%lf][0:%lf]\\\n",
                -0.05*Range, 1.05*Range, Max_steps*1.1);
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 10:5 title \"N\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 1,\\\n");
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 11:6 title \"E\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 5,\\\n");
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 12:7 title \"S\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 7,\\\n");
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 13:8 title \"W\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 2\n");
      fprintf(fp, "unset title\n");
      fprintf(fp, "unset xlabel\n");
      fprintf(fp, "unset ylabel\n");

      fprintf(fp, "set title \"Timestep %d: Threshold versus switch count\"\n", t);
      fprintf(fp, "set size 0.5,0.5\n");
      fprintf(fp, "set xlabel \"Threshold value\"\n");
      fprintf(fp, "set ylabel \"Switch count\"\n");
      fprintf(fp, "plot [%lf:%lf][0:]\\\n",
                -0.05*Range, 1.05*Range);
//      fprintf(fp, "plot [%lf:%lf][0:%lf]\\\n",
//                -0.05*Range, 1.05*Range, Max_steps*1.1);
      fprintf(fp, "   \"run.%d.animatethresh.step%d\" using 10:15 title \"N\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 1,\\\n");
      fprintf(fp, "   \"run.%d.animatethresh.step%d\" using 11:15 title \"E\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 5,\\\n");
      fprintf(fp, "   \"run.%d.animatethresh.step%d\" using 12:15 title \"S\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 7,\\\n");
      fprintf(fp, "   \"run.%d.animatethresh.step%d\" using 13:15 title \"W\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 2\n");
      fprintf(fp, "unset title\n");
      fprintf(fp, "unset xlabel\n");
      fprintf(fp, "unset ylabel\n");

      fprintf(fp, "\n");
      fprintf(fp, "# Timestep %d\n", t);
      fprintf(fp, "set title \"Timestep %d: Target and tracker path\"\n", t);
      fprintf(fp, "set size 1,0.5\n");
// axes scaled for zigzag and scurve
//      fprintf(fp, "plot [0:%lf][%lf:%lf]\\\n",
//		(1.8 * Target.amplitude/Target.period) * Max_steps, 
//		-1.5 * Target.amplitude, 1.5*Target.amplitude);
      fprintf(fp, "plot\\\n");
      fprintf(fp, "    \"run.%d.animatepath.step%d\" using 4:5 title ",
                Run_num, t);
      fprintf(fp,       "\"Target\" w linesp,\\\n");
      fprintf(fp, "    \"run.%d.animatepath.step%d\" using 7:8 title ",
                Run_num, t);
      fprintf(fp,       "\"Tracker\" w linesp\n");
      fprintf(fp, "unset title\n");
      fprintf(fp, "unset multiplot\n");

   fprintf(fp, "pause -1 \"Press any key to start\"\n");

   for (t=0; t<Max_steps; t++)
      {
      fprintf(fp, "\n");
      fprintf(fp, "# Timestep %d\n", t);
      fprintf(fp, "set multiplot layout 2,2\n");

      fprintf(fp, "set title \"Timestep %d: Threshold values versus activation\"\n", t);
      fprintf(fp, "set size 0.5,0.5\n");
      fprintf(fp, "set xlabel \"Threshold value\"\n");
      fprintf(fp, "set ylabel \"Action count\"\n");
      fprintf(fp, "plot [%lf:%lf][0:%lf]\\\n",
                -0.05*Range, 1.05*Range, Max_steps*1.1);
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 10:5 title \"N\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 1,\\\n");
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 11:6 title \"E\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 5,\\\n");
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 12:7 title \"S\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 7,\\\n");
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 13:8 title \"W\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 2\n");
      fprintf(fp, "unset title\n");
      fprintf(fp, "unset xlabel\n");
      fprintf(fp, "unset ylabel\n");

      fprintf(fp, "set title \"Timestep %d: Threshold versus switch count\"\n", t);
      fprintf(fp, "set size 0.5,0.5\n");
      fprintf(fp, "set xlabel \"Threshold value\"\n");
      fprintf(fp, "set ylabel \"Switch count\"\n");
      fprintf(fp, "plot [%lf:%lf][0:]\\\n",
                -0.05*Range, 1.05*Range);
//      fprintf(fp, "plot [%lf:%lf][0:%lf]\\\n",
//                -0.05*Range, 1.05*Range, Max_steps*1.1);
      fprintf(fp, "   \"run.%d.animatethresh.step%d\" using 10:15 title \"N\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 1,\\\n");
      fprintf(fp, "   \"run.%d.animatethresh.step%d\" using 11:15 title \"E\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 5,\\\n");
      fprintf(fp, "   \"run.%d.animatethresh.step%d\" using 12:15 title \"S\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 7,\\\n");
      fprintf(fp, "   \"run.%d.animatethresh.step%d\" using 13:15 title \"W\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 2\n");
      fprintf(fp, "unset title\n");
      fprintf(fp, "unset xlabel\n");
      fprintf(fp, "unset ylabel\n");

      fprintf(fp, "set title \"Timestep %d: Target and tracker path\"\n", t);
      fprintf(fp, "set size 1,0.5\n");
// axes scaled for zigzag and scurve
//      fprintf(fp, "plot [0:%lf][%lf:%lf]\\\n",
//		(1.8 * Target.amplitude/Target.period) * Max_steps, 
//		-1.5 * Target.amplitude, 1.5*Target.amplitude);
      fprintf(fp, "plot\\\n");
      fprintf(fp, "    \"run.%d.animatepath.step%d\" using 4:5 title ",
                Run_num, t);
      fprintf(fp,       "\"Target\" w linesp,\\\n");
      fprintf(fp, "    \"run.%d.animatepath.step%d\" using 7:8 title ",
                Run_num, t);
      fprintf(fp,       "\"Tracker\" w linesp\n");
      fprintf(fp, "unset title\n");
      fprintf(fp, "unset multiplot\n");

      if (Animate_stepwise == 0)
         {
         fprintf(fp, "pause 0.1 \"Timestep %d\"\n", t);
         }
      else if (Animate_stepwise == 1)
         {
         fprintf(fp, "pause -1 \"Press any key to start\"\n");
         }
      else
         {
         printf(" Error(fprint_animate_combo_gnu):  Invalid value for Animate_stepwise %d\n", Animate_stepwise);
         }
      }

   t = Max_steps - 1;
      fprintf(fp, "\n");
      fprintf(fp, "# Timestep %d\n", t);
      fprintf(fp, "set multiplot layout 2,2\n");

      fprintf(fp, "set title \"Timestep %d: Threshold values versus activation\"\n", t);
      fprintf(fp, "set size 0.5,0.5\n");
      fprintf(fp, "set xlabel \"Threshold value\"\n");
      fprintf(fp, "set ylabel \"Action count\"\n");
      fprintf(fp, "plot [%lf:%lf][0:%lf]\\\n",
                -0.05*Range, 1.05*Range, Max_steps*1.1);
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 10:5 title \"N\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 1,\\\n");
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 11:6 title \"E\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 5,\\\n");
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 12:7 title \"S\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 7,\\\n");
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 13:8 title \"W\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 2\n");
      fprintf(fp, "unset title\n");
      fprintf(fp, "unset xlabel\n");
      fprintf(fp, "unset ylabel\n");

      fprintf(fp, "set title \"Timestep %d: Threshold versus switch count\"\n", t);
      fprintf(fp, "set size 0.5,0.5\n");
      fprintf(fp, "set xlabel \"Threshold value\"\n");
      fprintf(fp, "set ylabel \"Switch count\"\n");
      fprintf(fp, "plot [%lf:%lf][0:]\\\n",
                -0.05*Range, 1.05*Range);
//      fprintf(fp, "plot [%lf:%lf][0:%lf]\\\n",
//                -0.05*Range, 1.05*Range, Max_steps*1.1);
      fprintf(fp, "   \"run.%d.animatethresh.step%d\" using 10:15 title \"N\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 1,\\\n");
      fprintf(fp, "   \"run.%d.animatethresh.step%d\" using 11:15 title \"E\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 5,\\\n");
      fprintf(fp, "   \"run.%d.animatethresh.step%d\" using 12:15 title \"S\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 7,\\\n");
      fprintf(fp, "   \"run.%d.animatethresh.step%d\" using 13:15 title \"W\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 2\n");
      fprintf(fp, "unset title\n");
      fprintf(fp, "unset xlabel\n");
      fprintf(fp, "unset ylabel\n");

      fprintf(fp, "set title \"Timestep %d: Target and tracker path\"\n", t);
      fprintf(fp, "set size 1,0.5\n");
// axes scaled for zigzag and scurve
//      fprintf(fp, "plot [0:%lf][%lf:%lf]\\\n",
//		(1.8 * Target.amplitude/Target.period) * Max_steps, 
//		-1.5 * Target.amplitude, 1.5*Target.amplitude);
      fprintf(fp, "plot\\\n");
      fprintf(fp, "    \"run.%d.animatepath.step%d\" using 4:5 title ",
                Run_num, t);
      fprintf(fp,       "\"Target\" w linesp,\\\n");
      fprintf(fp, "    \"run.%d.animatepath.step%d\" using 7:8 title ",
                Run_num, t);
      fprintf(fp,       "\"Tracker\" w linesp\n");
      fprintf(fp, "unset title\n");
      fprintf(fp, "unset multiplot\n");

   fprintf(fp, "pause -1 \"Press any key to continue\"\n");
   fprintf(fp, "pause -1 \"Press any key to continue\"\n");

   fclose(fp);

#ifdef DEBUG
printf("---end fprint_animate_combo_gnu()---\n");
#endif
   }  /* fprint_animate_combo_gnu */

/********** fprint_animate_threshactseparate_gnu ***********/
/* Called by:           step_output(), output.c
   Parameters:          
   Actions:             Print the gnuplot for animating the threshold versus
                        action count for each timestep.
			Same data as fprint_animate_threshact_gnu but with
			each direction on a separate plot.
*/
void fprint_animate_threshactseparate_gnu()
   {
   char filename[INPUT_LINE_LEN];
   FILE *fp;
   int t;

#ifdef DEBUG
printf("---in fprint_animate_threshactseparate_gnu()---\n");
#endif

   sprintf(filename, "%s/run.%d/run.%d.animatethreshactseparate.gnu",
        Output_path, Run_num, Run_num);
   fp = fopen(filename, "w");

   fprintf(fp, "# This will only work on an X11 terminal.\n");
   fprintf(fp, "set term x11\n");
   fprintf(fp, "set xlabel \"Threshold value\"\n");
   fprintf(fp, "set ylabel \"Action count\"\n");

   // don't start the animation automatically
   // user needs to press a key to start the simulation
   t = 0;
      fprintf(fp, "set multiplot layout 3,2\n");
      fprintf(fp, "\n");
      fprintf(fp, "# Plot timestep 0, then wait for user to start animation.\n");
      fprintf(fp, "# Timestep %d\n", t);

      // north
      fprintf(fp, "set title \"Timestep %d, North: Threshold versus activation\"\n", t);
      fprintf(fp, "set size 0.5,0.39\n");
      fprintf(fp, "plot [%lf:%lf][0:%lf]\\\n",
                -0.05*Range, 1.05*Range, Max_steps*1.1);
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 10:5 title \"N\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 1\n");
      fprintf(fp, "unset title\n");

      // east
      fprintf(fp, "set title \"Timestep %d, East: Threshold versus activation\"\n", t);
      fprintf(fp, "set size 0.5,0.39\n");
      fprintf(fp, "plot [%lf:%lf][0:%lf]\\\n",
                -0.05*Range, 1.05*Range, Max_steps*1.1);
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 11:6 title \"E\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 5\n");
      fprintf(fp, "unset title\n");

      // south
      fprintf(fp, "set title \"Timestep %d, South: Threshold versus activation\"\n", t);
      fprintf(fp, "set size 0.5,0.39\n");
      fprintf(fp, "plot [%lf:%lf][0:%lf]\\\n",
                -0.05*Range, 1.05*Range, Max_steps*1.1);
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 12:7 title \"S\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 7\n");
      fprintf(fp, "unset title\n");

      // west
      fprintf(fp, "set title \"Timestep %d, West: Threshold versus activation\"\n", t);
      fprintf(fp, "set size 0.5,0.39\n");
      fprintf(fp, "plot [%lf:%lf][0:%lf]\\\n",
                -0.05*Range, 1.05*Range, Max_steps*1.1);
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 13:8 title \"W\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 2\n");
      fprintf(fp, "unset title\n");

      fprintf(fp, "\n");
      fprintf(fp, "# Timestep %d\n", t);
      fprintf(fp, "set title \"Timestep %d: Target and tracker path\"\n", t);
      fprintf(fp, "set size 1,0.39\n");
      fprintf(fp, "plot\\\n");
      fprintf(fp, "    \"run.%d.animatepath.step%d\" using 4:5 title ",
                Run_num, t);
      fprintf(fp,       "\"Target\" w linesp,\\\n");
      fprintf(fp, "    \"run.%d.animatepath.step%d\" using 7:8 title ",
                Run_num, t);
      fprintf(fp,       "\"Tracker\" w linesp\n");
      fprintf(fp, "unset title\n");

      fprintf(fp, "unset multiplot\n");

   fprintf(fp, "pause -1 \"Press any key to start\"\n");

   for (t=0; t<Max_steps; t++)
      {
      fprintf(fp, "\n");
      fprintf(fp, "# Timestep %d\n", t);
      fprintf(fp, "set multiplot layout 3,2\n");

      fprintf(fp, "set title \"Timestep %d, North: Threshold values versus activation\"\n", t);
      fprintf(fp, "set size 0.5,0.39\n");
      fprintf(fp, "plot [%lf:%lf][0:%lf]\\\n",
                -0.05*Range, 1.05*Range, Max_steps*1.1);
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 10:5 title \"N\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 1\n");
      fprintf(fp, "unset title\n");

      fprintf(fp, "set title \"Timestep %d, East: Threshold values versus activation\"\n", t);
      fprintf(fp, "set size 0.5,0.39\n");
      fprintf(fp, "plot [%lf:%lf][0:%lf]\\\n",
                -0.05*Range, 1.05*Range, Max_steps*1.1);
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 11:6 title \"E\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 5\n");
      fprintf(fp, "unset title\n");

      fprintf(fp, "set title \"Timestep %d, South: Threshold values versus activation\"\n", t);
      fprintf(fp, "set size 0.5,0.39\n");
      fprintf(fp, "plot [%lf:%lf][0:%lf]\\\n",
                -0.05*Range, 1.05*Range, Max_steps*1.1);
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 12:7 title \"S\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 7\n");
      fprintf(fp, "unset title\n");

      fprintf(fp, "set title \"Timestep %d, West: Threshold values versus activation\"\n", t);
      fprintf(fp, "set size 0.5,0.39\n");
      fprintf(fp, "plot [%lf:%lf][0:%lf]\\\n",
                -0.05*Range, 1.05*Range, Max_steps*1.1);
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 13:8 title \"W\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 2\n");
      fprintf(fp, "unset title\n");

      fprintf(fp, "set title \"Timestep %d: Target and tracker path\"\n", t);
      fprintf(fp, "set size 1,0.39\n");
      fprintf(fp, "plot\\\n");
      fprintf(fp, "    \"run.%d.animatepath.step%d\" using 4:5 title ",
                Run_num, t);
      fprintf(fp,       "\"Target\" w linesp,\\\n");
      fprintf(fp, "    \"run.%d.animatepath.step%d\" using 7:8 title ",
                Run_num, t);
      fprintf(fp,       "\"Tracker\" w linesp\n");
      fprintf(fp, "unset title\n");

      fprintf(fp, "unset multiplot\n");

      if (Animate_stepwise == 0)
         {
         fprintf(fp, "pause 0.1 \"Timestep %d\"\n", t);
         }
      else if (Animate_stepwise == 1)
         {
         fprintf(fp, "pause -1 \"Press any key to start\"\n");
         }
      else
         {
         printf(" Error(fprint_animate_combo_gnu):  Invalid value for Animate_stepwise %d\n", Animate_stepwise);
         }
      }

   t = Max_steps-1;
      fprintf(fp, "\n");
      fprintf(fp, "# Timestep %d\n", t);
      fprintf(fp, "set multiplot layout 3,2\n");

      fprintf(fp, "set title \"Timestep %d, North: Threshold values versus activation\"\n", t);
      fprintf(fp, "set size 0.5,0.39\n");
      fprintf(fp, "plot [%lf:%lf][0:%lf]\\\n",
                -0.05*Range, 1.05*Range, Max_steps*1.1);
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 10:5 title \"N\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 1\n");
      fprintf(fp, "unset title\n");

      fprintf(fp, "set title \"Timestep %d, East: Threshold values versus activation\"\n", t);
      fprintf(fp, "set size 0.5,0.39\n");
      fprintf(fp, "plot [%lf:%lf][0:%lf]\\\n",
                -0.05*Range, 1.05*Range, Max_steps*1.1);
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 11:6 title \"E\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 5\n");
      fprintf(fp, "unset title\n");

      fprintf(fp, "set title \"Timestep %d, South: Threshold values versus activation\"\n", t);
      fprintf(fp, "set size 0.5,0.39\n");
      fprintf(fp, "plot [%lf:%lf][0:%lf]\\\n",
                -0.05*Range, 1.05*Range, Max_steps*1.1);
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 12:7 title \"S\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 7\n");
      fprintf(fp, "unset title\n");

      fprintf(fp, "set title \"Timestep %d, West: Threshold values versus activation\"\n", t);
      fprintf(fp, "set size 0.5,0.39\n");
      fprintf(fp, "plot [%lf:%lf][0:%lf]\\\n",
                -0.05*Range, 1.05*Range, Max_steps*1.1);
      fprintf(fp, "    \"run.%d.animatethresh.step%d\" using 13:8 title \"W\" ",
                Run_num, t);
      fprintf(fp,       "w points pt 7 ps 0.9 lc 2\n");
      fprintf(fp, "unset title\n");

      fprintf(fp, "set title \"Timestep %d: Target and tracker path\"\n", t);
      fprintf(fp, "set size 1,0.39\n");
      fprintf(fp, "plot\\\n");
      fprintf(fp, "    \"run.%d.animatepath.step%d\" using 4:5 title ",
                Run_num, t);
      fprintf(fp,       "\"Target\" w linesp,\\\n");
      fprintf(fp, "    \"run.%d.animatepath.step%d\" using 7:8 title ",
                Run_num, t);
      fprintf(fp,       "\"Tracker\" w linesp\n");
      fprintf(fp, "unset title\n");
      fprintf(fp, "unset multiplot\n");

   // for some reason I need this twice to pause the image
   // and keep it from automatically disappearing
   fprintf(fp, "pause -1 \"Press any key to continue\"\n");
   fprintf(fp, "pause -1 \"Press any key to continue\"\n");

   fclose(fp);

#ifdef DEBUG
printf("---end fprint_animate_threshactseparate_gnu()---\n");
#endif
   }  /* fprint_animate_threshactseparate_gnu */

