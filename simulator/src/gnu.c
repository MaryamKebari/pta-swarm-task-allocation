/* gnu.c
   19.07.30.AW  Created.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "extern.h"
#include "gnu.h"
#include "params.h"

/********** plot_gnuplot_files ***********/
/* Called by:           end_sim(), sim.c
   Parameters:
   Actions:             Execute the gnuplot file for the current run.
*/
void plot_gnuplot_files()
   {
   char cmd[INPUT_LINE_LEN];
#ifdef DEBUG
printf("---in plot_gnuplot_files()---\n");
#endif

   sprintf(cmd, "cd %s/run.%d && gnuplot run.%d.gnu", 
           Output_path, Run_num, Run_num);
   printf(" Executing: %s\n", cmd);
   system(cmd);

#ifdef DEBUG
printf("---end plot_gnuplot_files()---\n");
#endif
   }  /* plot_gnuplot_files */

/********** fprint_gnu ***********/
/* Called by:           run_output(), output.c
   Parameters:
   Actions:             Print gnuplot file.
*/
void fprint_gnu(FILE *fp)
   {
#ifdef DEBUG
printf("---in fprint_gnu()---\n");
#endif

   fprintf(fp, "set datafile separator ','\n");
   fprintf(fp, "set key outside\n");
   fprintf(fp, "set grid\n\n");

   if (file_on("stepdemand"))       gnu_stepdemand(fp);
   if (file_on("stepsummary"))      gnu_stepsummary(fp);
   if (file_on("steptaskdemand"))   gnu_steptaskdemand(fp);
   if (file_on("steptaskcounts"))   gnu_steptaskcounts(fp);
   if (file_on("steptaskthresh"))   gnu_steptaskthresh(fp);
   if (file_on("stepagentaction"))  gnu_stepagentaction(fp);
   if (file_on("finaltask"))        gnu_finaltask(fp);

#ifdef DEBUG
printf("---end fprint_gnu()---\n");
#endif
   }  /* fprint_gnu */

/********** gnu_stephistnorth ***********/
/* Called by:           fprint_gnu(), gnu.c
   Parameters:
   Actions:             Plots a surface that shows the histogram of
			intensity values per timestep.
*/
void gnu_stephistnorth(FILE *fp)
   {
#ifdef DEBUG
printf("---in gnu_stephistnorth()---\n");
#endif

   fprintf(fp, "set term post eps\n");
   fprintf(fp, "set xlabel \"Timestep\"\n");
   fprintf(fp, "set ylabel \"Bins\"\n");
   fprintf(fp, "set zlabel \"Count\"\n");
   fprintf(fp, "set output \"run.%d.stephistnorth.eps\"\n", Run_num);
   fprintf(fp, "set title \"Histogram of intensity values, north\"\n");
   fprintf(fp, "set dgrid3d 50,50 qnorm 2\n");
   fprintf(fp, "splot \"run.%d.stephistnorth\" using 2:4:6 w lines\n", Run_num);
   fprintf(fp, "unset size\n");
   fprintf(fp, "unset title\n");
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end gnu_stephistnorth()---\n");
#endif
   }  /* gnu_stephistnorth */

/********** gnu_stephistsouth ***********/
/* Called by:           fprint_gnu(), gnu.c
   Parameters:
   Actions:             Plots a surface that shows the histogram of
			intensity values per timestep.
*/
void gnu_stephistsouth(FILE *fp)
   {
#ifdef DEBUG
printf("---in gnu_stephistsouth()---\n");
#endif

   fprintf(fp, "set term post eps\n");
   fprintf(fp, "set xlabel \"Timestep\"\n");
   fprintf(fp, "set ylabel \"Bins\"\n");
   fprintf(fp, "set zlabel \"Count\"\n");
   fprintf(fp, "set output \"run.%d.stephistsouth.eps\"\n", Run_num);
   fprintf(fp, "set title \"Histogram of intensity values, south\"\n");
   fprintf(fp, "set dgrid3d 50,50 qnorm 2\n");
   fprintf(fp, "splot \"run.%d.stephistsouth\" using 2:4:6 w lines\n", Run_num);
   fprintf(fp, "unset size\n");
   fprintf(fp, "unset title\n");
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end gnu_stephistsouth()---\n");
#endif
   }  /* gnu_stephistsouth */

/********** gnu_stephisteast ***********/
/* Called by:           fprint_gnu(), gnu.c
   Parameters:
   Actions:             Plots a surface that shows the histogram of
			intensity values per timestep.
*/
void gnu_stephisteast(FILE *fp)
   {
#ifdef DEBUG
printf("---in gnu_stephisteast()---\n");
#endif

   fprintf(fp, "set term post eps\n");
   fprintf(fp, "set xlabel \"Timestep\"\n");
   fprintf(fp, "set ylabel \"Bins\"\n");
   fprintf(fp, "set zlabel \"Count\"\n");
   fprintf(fp, "set output \"run.%d.stephisteast.eps\"\n", Run_num);
   fprintf(fp, "set title \"Histogram of intensity values, east\"\n");
   fprintf(fp, "set dgrid3d 50,50 qnorm 2\n");
   fprintf(fp, "splot \"run.%d.stephisteast\" using 2:4:6 w lines\n", Run_num);
   fprintf(fp, "unset size\n");
   fprintf(fp, "unset title\n");
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end gnu_stephisteast()---\n");
#endif
   }  /* gnu_stephisteast */

/********** gnu_stephistwest ***********/
/* Called by:           fprint_gnu(), gnu.c
   Parameters:
   Actions:             Plots a surface that shows the histogram of
			intensity values per timestep.
*/
void gnu_stephistwest(FILE *fp)
   {
#ifdef DEBUG
printf("---in gnu_stephistwest()---\n");
#endif

   fprintf(fp, "set term post eps\n");
   fprintf(fp, "set xlabel \"Timestep\"\n");
   fprintf(fp, "set ylabel \"Bins\"\n");
   fprintf(fp, "set zlabel \"Count\"\n");
   fprintf(fp, "set output \"run.%d.stephistwest.eps\"\n", Run_num);
   fprintf(fp, "set title \"Histogram of intensity values, west\"\n");
   fprintf(fp, "set dgrid3d 50,50 qnorm 2\n");
   fprintf(fp, "splot \"run.%d.stephistwest\" using 2:4:6 w lines\n", Run_num);
   fprintf(fp, "unset size\n");
   fprintf(fp, "unset title\n");
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end gnu_stephistwest()---\n");
#endif
   }  /* gnu_stephistwest */

/********** gnu_stepdemand ***********/
/* CSV task-vector demand/service summary plots. */
void gnu_stepdemand(FILE *fp)
   {
#ifdef DEBUG
printf("---in gnu_stepdemand()---\n");
#endif

   fprintf(fp, "set term post eps color\n");
   fprintf(fp, "set xlabel 'Timestep'\n");
   fprintf(fp, "set output 'run.%d.stepdemand.norms.eps'\n", Run_num);
   fprintf(fp, "set title 'Task-vector norms'\n");
   fprintf(fp, "set ylabel 'Vector norm'\n");
   fprintf(fp, "plot 'run.%d.stepdemand' using 1:2 title 'target arrival norm' w lines,\\\n", Run_num);
   fprintf(fp, "     'run.%d.stepdemand' using 1:3 title 'tracker service norm' w lines,\\\n", Run_num);
   fprintf(fp, "     'run.%d.stepdemand' using 1:4 title 'pre-service demand norm' w lines,\\\n", Run_num);
   fprintf(fp, "     'run.%d.stepdemand' using 1:5 title 'residual demand norm' w lines\n", Run_num);
   fprintf(fp, "set output 'run.%d.stepdemand.sums.eps'\n", Run_num);
   fprintf(fp, "set title 'Task demand and service sums'\n");
   fprintf(fp, "set ylabel 'Sum'\n");
   fprintf(fp, "plot 'run.%d.stepdemand' using 1:6 title 'step arrival sum' w lines,\\\n", Run_num);
   fprintf(fp, "     'run.%d.stepdemand' using 1:7 title 'step service sum' w lines,\\\n", Run_num);
   fprintf(fp, "     'run.%d.stepdemand' using 1:8 title 'remaining demand sum' w lines\n", Run_num);
   fprintf(fp, "unset title\nunset xlabel\nunset ylabel\n\n");

#ifdef DEBUG
printf("---end gnu_stepdemand()---\n");
#endif
   }  /* gnu_stepdemand */

/********** gnu_stepsummary ***********/
/* CSV task-vector summary plots. */
void gnu_stepsummary(FILE *fp)
   {
#ifdef DEBUG
printf("---in gnu_stepsummary()---\n");
#endif

   fprintf(fp, "set term post eps color\n");
   fprintf(fp, "set xlabel 'Timestep'\n");
   fprintf(fp, "set output 'run.%d.stepsummary.actors.eps'\n", Run_num);
   fprintf(fp, "set title 'Agents allocated vs idle'\n");
   fprintf(fp, "set ylabel 'Agents'\n");
   fprintf(fp, "plot 'run.%d.stepsummary' using 1:9 title 'allocated' w lines,\\\n", Run_num);
   fprintf(fp, "     'run.%d.stepsummary' using 1:10 title 'idle' w lines\n", Run_num);
   fprintf(fp, "set output 'run.%d.stepsummary.switches.eps'\n", Run_num);
   fprintf(fp, "set title 'Task switching'\n");
   fprintf(fp, "set ylabel 'Switch metric'\n");
   fprintf(fp, "plot 'run.%d.stepsummary' using 1:11 title 'switch count' w lines,\\\n", Run_num);
   fprintf(fp, "     'run.%d.stepsummary' using 1:12 title 'pct prior actors' w lines,\\\n", Run_num);
   fprintf(fp, "     'run.%d.stepsummary' using 1:13 title 'pct population' w lines,\\\n", Run_num);
   fprintf(fp, "     'run.%d.stepsummary' using 1:14 title 'switch count no idle' w lines\n", Run_num);
   fprintf(fp, "unset title\nunset xlabel\nunset ylabel\n\n");

#ifdef DEBUG
printf("---end gnu_stepsummary()---\n");
#endif
   }  /* gnu_stepsummary */

void gnu_steptaskdemand(FILE *fp)
   {
   int task;
#ifdef DEBUG
printf("---in gnu_steptaskdemand()---\n");
#endif

   fprintf(fp, "set term post eps color\n");
   fprintf(fp, "set xlabel 'Timestep'\n");
   fprintf(fp, "set ylabel 'Task value'\n");
   fprintf(fp, "set output 'run.%d.steptaskdemand.demand.eps'\n", Run_num);
   fprintf(fp, "set title 'Remaining demand by task'\n");
   fprintf(fp, "plot ");
   for (task=1; task<=Num_tasks; task++)
      fprintf(fp, "%s'run.%d.steptaskdemand' using 1:%d title 'task %d' w lines%s",
              (task==1 ? "" : "     "), Run_num, task+1, task,
              (task==Num_tasks ? "\n" : ",\\\n"));
   fprintf(fp, "set output 'run.%d.steptaskdemand.service.eps'\n", Run_num);
   fprintf(fp, "set title 'Step service by task'\n");
   fprintf(fp, "plot ");
   for (task=1; task<=Num_tasks; task++)
      fprintf(fp, "%s'run.%d.steptaskdemand' using 1:%d title 'task %d' w lines%s",
              (task==1 ? "" : "     "), Run_num, (2*Num_tasks)+task+1, task,
              (task==Num_tasks ? "\n" : ",\\\n"));
   fprintf(fp, "unset title\nunset xlabel\nunset ylabel\n\n");

#ifdef DEBUG
printf("---end gnu_steptaskdemand()---\n");
#endif
   }  /* gnu_steptaskdemand */

void gnu_steptaskcounts(FILE *fp)
   {
   int task;
#ifdef DEBUG
printf("---in gnu_steptaskcounts()---\n");
#endif

   fprintf(fp, "set term post eps color\n");
   fprintf(fp, "set xlabel 'Timestep'\n");
   fprintf(fp, "set ylabel 'Agents'\n");
   fprintf(fp, "set output 'run.%d.steptaskcounts.actors.eps'\n", Run_num);
   fprintf(fp, "set title 'Agents allocated by task'\n");
   fprintf(fp, "plot ");
   for (task=1; task<=Num_tasks; task++)
      fprintf(fp, "%s'run.%d.steptaskcounts' using 1:%d title 'task %d' w lines%s",
              (task==1 ? "" : "     "), Run_num, task+1, task,
              (task==Num_tasks ? "\n" : ",\\\n"));
   fprintf(fp, "unset title\nunset xlabel\nunset ylabel\n\n");

#ifdef DEBUG
printf("---end gnu_steptaskcounts()---\n");
#endif
   }  /* gnu_steptaskcounts */

void gnu_steptaskthresh(FILE *fp)
   {
   int task;
#ifdef DEBUG
printf("---in gnu_steptaskthresh()---\n");
#endif

   fprintf(fp, "set term post eps color\n");
   fprintf(fp, "set xlabel 'Timestep'\n");
   fprintf(fp, "set ylabel 'Average threshold'\n");
   fprintf(fp, "set output 'run.%d.steptaskthresh.avg_thresh.eps'\n", Run_num);
   fprintf(fp, "set title 'Average threshold by task'\n");
   fprintf(fp, "plot ");
   for (task=1; task<=Num_tasks; task++)
      fprintf(fp, "%s'run.%d.steptaskthresh' using 1:%d title 'task %d' w lines%s",
              (task==1 ? "" : "     "), Run_num, task+1, task,
              (task==Num_tasks ? "\n" : ",\\\n"));
   fprintf(fp, "set ylabel 'Average response probability'\n");
   fprintf(fp, "set output 'run.%d.steptaskthresh.avg_resprob.eps'\n", Run_num);
   fprintf(fp, "set title 'Average response probability by task'\n");
   fprintf(fp, "plot ");
   for (task=1; task<=Num_tasks; task++)
      fprintf(fp, "%s'run.%d.steptaskthresh' using 1:%d title 'task %d' w lines%s",
              (task==1 ? "" : "     "), Run_num, Num_tasks+task+1, task,
              (task==Num_tasks ? "\n" : ",\\\n"));
   fprintf(fp, "unset title\nunset xlabel\nunset ylabel\n\n");

#ifdef DEBUG
printf("---end gnu_steptaskthresh()---\n");
#endif
   }  /* gnu_steptaskthresh */

void gnu_finaltask(FILE *fp)
   {
#ifdef DEBUG
printf("---in gnu_finaltask()---\n");
#endif

   fprintf(fp, "set term post eps color\n");
   fprintf(fp, "set style data histograms\nset style fill solid border -1\nset boxwidth 0.9\n");
   fprintf(fp, "set xlabel 'Task'\n");
   fprintf(fp, "set output 'run.%d.finaltask.service.eps'\n", Run_num);
   fprintf(fp, "set title 'Total arrival and service by task'\n");
   fprintf(fp, "set ylabel 'Total'\n");
   fprintf(fp, "plot 'run.%d.finaltask' using 3:xtic(1) title 'arrival', '' using 4 title 'service'\n", Run_num);
   fprintf(fp, "set output 'run.%d.finaltask.avg_count.eps'\n", Run_num);
   fprintf(fp, "set title 'Average agent count by task'\n");
   fprintf(fp, "set ylabel 'Average count'\n");
   fprintf(fp, "plot 'run.%d.finaltask' using 6:xtic(1) title 'avg count'\n", Run_num);
   fprintf(fp, "unset style data\nunset title\nunset xlabel\nunset ylabel\n\n");

#ifdef DEBUG
printf("---end gnu_finaltask()---\n");
#endif
   }  /* gnu_finaltask */

/********** gnu_steptargetpath ***********/
/* Called by:           fprint_gnu(), gnu.c
   Parameters:
   Actions:             Plot target path.
*/
void gnu_steptargetpath(FILE *fp)
   {
#ifdef DEBUG
printf("---in gnu_steptargetpath()---\n");
#endif

   fprintf(fp, "set term post eps color\n");
   fprintf(fp, "set size square\n");
   fprintf(fp, "set output \"run.%d.steptargetpath.eps\"\n", Run_num);
   fprintf(fp, "set title \"Target path\"\n");
   fprintf(fp, "plot \"run.%d.steptargetpath\" using 4:6 w line\n", Run_num);
   fprintf(fp, "unset size\n");
   fprintf(fp, "unset title\n");
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end gnu_steptargetpath()---\n");
#endif
   }  /* gnu_steptargetpath */

/********** gnu_steptrackerpath ***********/
/* Called by:           fprint_gnu(), gnu.c
   Parameters:
   Actions:             Plot tracker path.
*/
void gnu_steptrackerpath(FILE *fp)
   {
#ifdef DEBUG
printf("---in gnu_steptrackerpath()---\n");
#endif

   fprintf(fp, "set term post eps color\n");
   fprintf(fp, "set size square\n");
   fprintf(fp, "set output \"run.%d.steptrackerpath.eps\"\n", Run_num);
   fprintf(fp, "set title \"Target path\"\n");
   fprintf(fp, "plot \"run.%d.steptrackerpath\" using 4:6 w line\n", Run_num);
   fprintf(fp, "unset size\n");
   fprintf(fp, "unset title\n");
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end gnu_steptrackerpath()---\n");
#endif
   }  /* gnu_steptrackerpath */

/********** gnu_stepbothpaths ***********/
/* Called by:           fprint_gnu(), gnu.c
   Parameters:
   Actions:             Plot target and tracker paths.
*/
void gnu_stepbothpaths(FILE *fp)
   {
#ifdef DEBUG
printf("---in gnu_stepbothpaths()---\n");
#endif

   fprintf(fp, "set term post eps color\n");
   fprintf(fp, "set size square\n");
   fprintf(fp, "set output \"run.%d.stepbothpaths.eps\"\n", Run_num);
   fprintf(fp, "set title \"Target and Tracker paths\"\n");
   fprintf(fp, "plot\\\n");
//   fprintf(fp, "plot[-35:35][-35:35]\\\n");
   fprintf(fp, "     \"run.%d.steptargetpath\" using 4:6 w linesp,\\\n",
		Run_num);
   fprintf(fp, "     \"run.%d.steptrackerpath\" using 4:6 w linesp\n",
		Run_num);
   fprintf(fp, "unset size\n");
   fprintf(fp, "unset title\n");
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end gnu_stepbothpaths()---\n");
#endif
   }  /* gnu_stepbothpaths */

/********** gnu_initpop ***********/
/* Called by:           fprint_gnu(), gnu.c
   Parameters:
   Actions:             Print gnuplot file.
*/
void gnu_initpop(FILE *fp)
   {
#ifdef DEBUG
printf("---in gnu_initpop()---\n");
#endif

   // plot raw threshold values -- value between 0.0 and 1.0
   fprintf(fp, "set term post eps color\n");
   fprintf(fp, "set size square\n");
   fprintf(fp, "set output \"run.%d.initpop.rawthresh.eps\"\n", Run_num);
   fprintf(fp, "set xtics (\"N\" 1, \"E\" 2, \"S\" 3, \"W\" 4)\n");
   fprintf(fp, "set ylabel \"Raw thresholds\"\n");
   fprintf(fp, "set title \"Initial population thresholds - raw\"\n");
   fprintf(fp, "plot [0:5][0:1.2]\\\n");
   fprintf(fp, "     \"run.%d.initpop\" using 23:5 w points,\\\n", Run_num);
   fprintf(fp, "     \"run.%d.initpop\" using 24:7 w points,\\\n", Run_num);
   fprintf(fp, "     \"run.%d.initpop\" using 25:9 w points,\\\n", Run_num);
   fprintf(fp, "     \"run.%d.initpop\" using 26:11 w points,\\\n", Run_num);
   fprintf(fp, "     1.0 title \"\" lt 1 lc 7\n");
   fprintf(fp, "unset size\n");
   fprintf(fp, "unset ylabel\n");
   fprintf(fp, "unset xtics\n");
   fprintf(fp, "set xtics\n");
   fprintf(fp, "\n");

   // plot scaled threshold values -- value between 0.0 and Range
   fprintf(fp, "set term post eps color\n");
   fprintf(fp, "set size square\n");
   fprintf(fp, "set output \"run.%d.initpop.scaledthresh.eps\"\n", Run_num);
   fprintf(fp, "set xtics (\"N\" 1, \"E\" 2, \"S\" 3, \"W\" 4)\n");
   fprintf(fp, "set ylabel \"Scaled thresholds\"\n");
   fprintf(fp, "set title \"Initial population thresholds - scaled\"\n");
   fprintf(fp, "plot [0:5][0:%lf]\\\n", Range*1.2);
   fprintf(fp, "     \"run.%d.initpop\" using 23:16 w points,\\\n", Run_num);
   fprintf(fp, "     \"run.%d.initpop\" using 24:18 w points,\\\n", Run_num);
   fprintf(fp, "     \"run.%d.initpop\" using 25:20 w points,\\\n", Run_num);
   fprintf(fp, "     \"run.%d.initpop\" using 26:22 w points,\\\n", Run_num);
   fprintf(fp, "     %lf title \"\" lt 1 lc 7\n", Range);
   fprintf(fp, "unset size\n");
   fprintf(fp, "unset ylabel\n");
   fprintf(fp, "unset xtics\n");
   fprintf(fp, "unset title\n");
   fprintf(fp, "set xtics\n");
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end gnu_initpop()---\n");
#endif
   }  /* gnu_initpop */

/********** gnu_finalpop ***********/
/* Called by:           fprint_gnu(), gnu.c
   Parameters:
   Actions:             Print gnuplot file.
*/
void gnu_finalpop(FILE *fp)
   {
#ifdef DEBUG
printf("---in gnu_finalpop()---\n");
#endif

   // plot scaled threshold values -- value between 0.0 and Range
   fprintf(fp, "set term post eps color\n");
   fprintf(fp, "set size square\n");
   fprintf(fp, "set output \"run.%d.finalpop.scaledthresh.eps\"\n", Run_num);
   fprintf(fp, "set xtics (\"N\" 1, \"E\" 2, \"S\" 3, \"W\" 4)\n");
   fprintf(fp, "set ylabel \"Scaled thresholds\"\n");
   fprintf(fp, "set title \"Initial population thresholds - scaled\"\n");
   fprintf(fp, "plot [0:5][0:%lf]\\\n", Range*1.2);
   fprintf(fp, "     \"run.%d.finalpop\" using 23:16 w points,\\\n", Run_num);
   fprintf(fp, "     \"run.%d.finalpop\" using 24:18 w points,\\\n", Run_num);
   fprintf(fp, "     \"run.%d.finalpop\" using 25:20 w points,\\\n", Run_num);
   fprintf(fp, "     \"run.%d.finalpop\" using 26:22 w points,\\\n", Run_num);
   fprintf(fp, "     %lf title \"\" lt 1 lc 7\n", Range);
   fprintf(fp, "unset size\n");
   fprintf(fp, "unset ylabel\n");
   fprintf(fp, "unset xtics\n");
   fprintf(fp, "unset title\n");
   fprintf(fp, "set xtics\n");
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end gnu_finalpop()---\n");
#endif
   }  /* gnu_finalpop */

/********** gnu_stepnorthsouth ***********/
/* Called by:           fprint_gnu(), gnu.c
   Parameters:
   Actions:             Prints two plots -- the target and tracker N/S
			locations and the differences between those
			locations.
*/
void gnu_stepnorthsouth(FILE *fp)
   {
#ifdef DEBUG
printf("---in gnu_stepnorthsouth()---\n");
#endif

   fprintf(fp, "set term post eps color\n");
   fprintf(fp, "set size ratio 0.5\n");
   fprintf(fp, "set output \"run.%d.stepnorthsouth.loc.eps\"\n", Run_num);
   fprintf(fp, "set title \"North/south locations\"\n");
   fprintf(fp, "set xlabel \"Timestep\"\n");
   fprintf(fp, "set ylabel \"y-coordinate\"\n");
   fprintf(fp, "plot \"run.%d.stepnorthsouth\" using 2:4 title \"Target\" w line,\\\n", Run_num);
   fprintf(fp, "     \"run.%d.stepnorthsouth\" using 2:6 title \"Tracker\" w line lc 3\n", Run_num);
   fprintf(fp, "unset size\n");
   fprintf(fp, "\n");

   fprintf(fp, "set term post eps color\n");
   fprintf(fp, "set size ratio 0.5\n");
   fprintf(fp, "set output \"run.%d.stepnorthsouth.diff.eps\"\n", Run_num);
   fprintf(fp, "set title \"North/south difference, Target loc - Tracker loc\"\n");
   fprintf(fp, "set xlabel \"Timestep\"\n");
   fprintf(fp, "set ylabel \"Difference on y-axis\"\n");
   fprintf(fp, "plot \"run.%d.stepnorthsouth\" using 2:8 title \"Target\" w line\n", Run_num);
   fprintf(fp, "unset size\n");
   fprintf(fp, "unset xlabel\n");
   fprintf(fp, "unset ylabel\n");
   fprintf(fp, "unset title\n");
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end gnu_stepnorthsouth()---\n");
#endif
   }  /* gnu_stepnorthsouth */

/********** gnu_stepeastwest ***********/
/* Called by:           fprint_gnu(), gnu.c
   Parameters:
   Actions:             Prints two plots -- the target and tracker N/S
                        locations and the differences between those
                        locations.
*/
void gnu_stepeastwest(FILE *fp)
   {
#ifdef DEBUG
printf("---in gnu_stepeastwest()---\n");
#endif

   fprintf(fp, "set term post eps color\n");
   fprintf(fp, "set size ratio 0.5\n");
   fprintf(fp, "set output \"run.%d.stepeastwest.loc.eps\"\n", Run_num);
   fprintf(fp, "set title \"East/west locations\"\n");
   fprintf(fp, "set xlabel \"Timestep\"\n");
   fprintf(fp, "set ylabel \"x-coordinate\"\n");
   fprintf(fp, "plot \"run.%d.stepeastwest\" using 2:4 title \"Target\" w line,\\\n", Run_num);
   fprintf(fp, "     \"run.%d.stepeastwest\" using 2:6 title \"Tracker\" w line lc 3\n", Run_num);
   fprintf(fp, "unset size\n");
   fprintf(fp, "\n");

   fprintf(fp, "set term post eps color\n");
   fprintf(fp, "set size ratio 0.5\n");
   fprintf(fp, "set output \"run.%d.stepeastwest.diff.eps\"\n", Run_num);
   fprintf(fp, "set title \"East/west difference, Target loc - Tracker loc\"\n");
   fprintf(fp, "set xlabel \"Timestep\"\n");
   fprintf(fp, "set ylabel \"Difference on x-axis\"\n");
   fprintf(fp, "plot \"run.%d.stepeastwest\" using 2:8 title \"Target\" w line\n", Run_num);
   fprintf(fp, "unset size\n");
   fprintf(fp, "unset xlabel\n");
   fprintf(fp, "unset ylabel\n");
   fprintf(fp, "unset title\n");
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end gnu_stepeastwest()---\n");
#endif
   }  /* gnu_stepeastwest */

/********** gnu_stepagentaction ***********/
/* CSV agent action heatmap. */
void gnu_stepagentaction(FILE *fp)
   {
#ifdef DEBUG
printf("---in gnu_stepagentaction()---\n");
#endif

   fprintf(fp, "set term post eps color\n");
   fprintf(fp, "set output 'run.%d.stepagentaction.eps'\n", Run_num);
   fprintf(fp, "set title 'Agent task allocation heatmap'\n");
   fprintf(fp, "set xlabel 'Timestep'\n");
   fprintf(fp, "set ylabel 'Agent'\n");
   fprintf(fp, "set palette maxcolors %d\n", Num_tasks + 2);
   fprintf(fp, "set cbrange[-1:%d]\n", Num_tasks);
   fprintf(fp, "plot for [i=2:%d] 'run.%d.stepagentaction' using 1:(i-2):(column(i)) title '' with points pt 5 ps 0.35 palette\n",
           Pop_size + 1, Run_num);
   fprintf(fp, "unset title\nunset xlabel\nunset ylabel\n\n");

#ifdef DEBUG
printf("---end gnu_stepagentaction()---\n");
#endif
   }  /* gnu_stepagentaction */

void gnu_stepagentactionwtime(FILE *fp)
   {
   gnu_stepagentaction(fp);
   }  /* gnu_stepagentactionwtime */

void gnu_stepagentmintask(FILE *fp)
   {
   gnu_stepagentaction(fp);
   }  /* gnu_stepagentmintask */

void gnu_stepagentmintaskaction(FILE *fp)
   {
   gnu_stepagentaction(fp);
   }  /* gnu_stepagentmintaskaction */

/********** gnu_stepthreshnorth ***********/
/* Called by:           fprint_gnu(), gnu.c
   Parameters:
   Actions:             Plot target path.
*/
void gnu_stepthreshnorth(FILE *fp)
   {
#ifdef DEBUG
printf("---in gnu_stepthreshnorth()---\n");
#endif

   fprintf(fp, "set term post eps color\n");
//   fprintf(fp, "set size ratio %lf\n", (double)Max_steps/(double)Pop_size);
   fprintf(fp, "set size ratio 2\n");
   fprintf(fp, "set output \"run.%d.stepthreshnorth.eps\"\n", Run_num);
   fprintf(fp, "set title \"Agent thresholds: North\"\n");
   fprintf(fp, "set xlabel \"Agent\"\n");
   fprintf(fp, "set ylabel \"Timestep\"\n");
   fprintf(fp, "set palette maxcolors 100\n");
   fprintf(fp, "set palette defined (0 \"green\", %lf \"yellow\", %lf \"red\")\n", Range/2.0, Range);
   fprintf(fp, "set cbrange[0:%lf]\n", Range);
   fprintf(fp, "set view map\n");
   fprintf(fp, "plot [%lf:%lf][%d:-2] \"run.%d.stepthreshnorth\" matrix title \"\" w image\n",
           Pop_size*0.1*(-1), Pop_size*1.1, Max_steps+1, Run_num);
   fprintf(fp, "unset size\n");
   fprintf(fp, "unset title\n");
   fprintf(fp, "unset xlabel\n");
   fprintf(fp, "unset ylabel\n");
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end gnu_stepthreshnorth()---\n");
#endif
   }  /* gnu_stepthreshnorth */

/********** gnu_stepthreshsouth ***********/
/* Called by:           fprint_gnu(), gnu.c
   Parameters:
   Actions:             Plot target path.
*/
void gnu_stepthreshsouth(FILE *fp)
   {
#ifdef DEBUG
printf("---in gnu_stepthreshsouth()---\n");
#endif

   fprintf(fp, "set term post eps color\n");
//   fprintf(fp, "set size ratio %lf\n", (double)Max_steps/(double)Pop_size);
   fprintf(fp, "set size ratio 2\n");
   fprintf(fp, "set output \"run.%d.stepthreshsouth.eps\"\n", Run_num);
   fprintf(fp, "set title \"Agent thresholds: South\"\n");
   fprintf(fp, "set xlabel \"Agent\"\n");
   fprintf(fp, "set ylabel \"Timestep\"\n");
   fprintf(fp, "set palette maxcolors 100\n");
   fprintf(fp, "set palette defined (0 \"green\", %lf \"yellow\", %lf \"red\")\n", Range/2.0, Range);
   fprintf(fp, "set cbrange[0:%lf]\n", Range);
   fprintf(fp, "set view map\n");
   fprintf(fp, "plot [%lf:%lf][%d:-2] \"run.%d.stepthreshsouth\" matrix title \"\" w image\n",
           Pop_size*0.1*(-1), Pop_size*1.1, Max_steps+1, Run_num);
   fprintf(fp, "unset size\n");
   fprintf(fp, "unset title\n");
   fprintf(fp, "unset xlabel\n");
   fprintf(fp, "unset ylabel\n");
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end gnu_stepthreshsouth()---\n");
#endif
   }  /* gnu_stepthreshsouth */

/********** gnu_stepthresheast ***********/
/* Called by:           fprint_gnu(), gnu.c
   Parameters:
   Actions:             Plot target path.
*/
void gnu_stepthresheast(FILE *fp)
   {
#ifdef DEBUG
printf("---in gnu_stepthresheast()---\n");
#endif

   fprintf(fp, "set term post eps color\n");
//   fprintf(fp, "set size ratio %lf\n", (double)Max_steps/(double)Pop_size);
   fprintf(fp, "set size ratio 2\n");
   fprintf(fp, "set output \"run.%d.stepthresheast.eps\"\n", Run_num);
   fprintf(fp, "set title \"Agent thresholds: East\"\n");
   fprintf(fp, "set xlabel \"Agent\"\n");
   fprintf(fp, "set ylabel \"Timestep\"\n");
   fprintf(fp, "set palette maxcolors 100\n");
   fprintf(fp, "set palette defined (0 \"green\", %lf \"yellow\", %lf \"red\")\n", Range/2.0, Range);
   fprintf(fp, "set cbrange[0:%lf]\n", Range);
   fprintf(fp, "set view map\n");
   fprintf(fp, "plot [%lf:%lf][%d:-2] \"run.%d.stepthresheast\" matrix title \"\" w image\n",
           Pop_size*0.1*(-1), Pop_size*1.1, Max_steps+1, Run_num);
   fprintf(fp, "unset size\n");
   fprintf(fp, "unset title\n");
   fprintf(fp, "unset xlabel\n");
   fprintf(fp, "unset ylabel\n");
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end gnu_stepthresheast()---\n");
#endif
   }  /* gnu_stepthresheast */

/********** gnu_stepthreshwest ***********/
/* Called by:           fprint_gnu(), gnu.c
   Parameters:
   Actions:             Plot target path.
*/
void gnu_stepthreshwest(FILE *fp)
   {
#ifdef DEBUG
printf("---in gnu_stepthreshwest()---\n");
#endif

   fprintf(fp, "set term post eps color\n");
//   fprintf(fp, "set size ratio %lf\n", (double)Max_steps/(double)Pop_size);
   fprintf(fp, "set size ratio 2\n");
   fprintf(fp, "set output \"run.%d.stepthreshwest.eps\"\n", Run_num);
   fprintf(fp, "set title \"Agent thresholds: West\"\n");
   fprintf(fp, "set xlabel \"Agent\"\n");
   fprintf(fp, "set ylabel \"Timestep\"\n");
   fprintf(fp, "set palette maxcolors 100\n");
   fprintf(fp, "set palette defined (0 \"green\", %lf \"yellow\", %lf \"red\")\n", Range/2.0, Range);
   fprintf(fp, "set cbrange[0:%lf]\n", Range);
   fprintf(fp, "set view map\n");
   fprintf(fp, "plot [%lf:%lf][%d:-2] \"run.%d.stepthreshwest\" matrix title \"\" w image\n",
           Pop_size*0.1*(-1), Pop_size*1.1, Max_steps+1, Run_num);
   fprintf(fp, "unset size\n");
   fprintf(fp, "unset title\n");
   fprintf(fp, "unset xlabel\n");
   fprintf(fp, "unset ylabel\n");
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end gnu_stepthreshwest()---\n");
#endif
   }  /* gnu_stepthreshwest */

/********** gnu_threshrange ***********/
/* Called by:           fprint_gnu(), gnu.c
   Parameters:
   Actions:             Plots initial threshold and threshold range for
			every agent.
*/
void gnu_threshrange(FILE *fp)
   {
#ifdef DEBUG
printf("---in gnu_threshrange()---\n");
#endif
   
   fprintf(fp, "set term post eps\n");
   fprintf(fp, "set size ratio 0.5\n"); 
   fprintf(fp, "set xlabel \"Agent\"\n");
   fprintf(fp, "set ylabel \"Threshold\"\n");

   fprintf(fp, "set output \"run.%d.threshrangenorth.eps\"\n", Run_num);
   fprintf(fp, "set title \"Agent threshold ranges: North\"\n");
   fprintf(fp, 
	"plot \"run.%d.threshrange\" using 2:4:5:6 title \"\" w errorbars\n",
	Run_num);

   fprintf(fp, "set output \"run.%d.threshrangesouth.eps\"\n", Run_num);
   fprintf(fp, "set title \"Agent threshold ranges: South\"\n");
   fprintf(fp, 
	"plot \"run.%d.threshrange\" using 2:8:9:10 title \"\" w errorbars\n",
	Run_num);

   fprintf(fp, "set output \"run.%d.threshrangeeast.eps\"\n", Run_num);
   fprintf(fp, "set title \"Agent threshold ranges: East\"\n");
   fprintf(fp, 
	"plot \"run.%d.threshrange\" using 2:12:13:14 title \"\" w errorbars\n",
	Run_num);

   fprintf(fp, "set output \"run.%d.threshrangewest.eps\"\n", Run_num);
   fprintf(fp, "set title \"Agent threshold ranges: West\"\n");
   fprintf(fp, 
	"plot \"run.%d.threshrange\" using 2:16:17:18 title \"\" w errorbars\n",
	Run_num);
   fprintf(fp, "unset size\n");

   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end gnu_threshrange()---\n");
#endif
   }  /* gnu_threshrange */

/********** gnu_intensityrange ***********/
/* Called by:           fprint_gnu(), gnu.c
   Parameters:
   Actions:             Plots initial intensity and intensity range for
			every agent.
*/
void gnu_intensityrange(FILE *fp)
   {
#ifdef DEBUG
printf("---in gnu_intensityrange()---\n");
#endif
   
   fprintf(fp, "set term post eps\n");
   fprintf(fp, "set size ratio 0.5\n"); 
   fprintf(fp, "set xlabel \"Agent\"\n");
   fprintf(fp, "set ylabel \"Intensity\"\n");

   fprintf(fp, "set output \"run.%d.intensityrangenorth.eps\"\n", Run_num);
   fprintf(fp, "set title \"Agent intensity ranges: North\"\n");
   fprintf(fp, 
	"plot \"run.%d.intensityrange\" using 2:4:5:6 title \"\" w errorbars\n",
	Run_num);

   fprintf(fp, "set output \"run.%d.intensityrangeeast.eps\"\n", Run_num);
   fprintf(fp, "set title \"Agent intensity ranges: East\"\n");
   fprintf(fp, 
	"plot \"run.%d.intensityrange\" using 2:8:9:10 title \"\" w errorbars\n",
	Run_num);

   fprintf(fp, "set output \"run.%d.intensityrangesouth.eps\"\n", Run_num);
   fprintf(fp, "set title \"Agent intensity ranges: South\"\n");
   fprintf(fp, 
	"plot \"run.%d.intensityrange\" using 2:12:13:14 title \"\" w errorbars\n",
	Run_num);

   fprintf(fp, "set output \"run.%d.intensityrangewest.eps\"\n", Run_num);
   fprintf(fp, "set title \"Agent intensity ranges: West\"\n");
   fprintf(fp, 
	"plot \"run.%d.intensityrange\" using 2:16:17:18 title \"\" w errorbars\n",
	Run_num);
   fprintf(fp, "unset size\n");

   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end gnu_intensityrange()---\n");
#endif
   }  /* gnu_intensityrange */

/********** gnu_stepintensitynorth ***********/
/* Called by:           fprint_gnu(), gnu.c
   Parameters:
   Actions:             Plot target path.
*/
void gnu_stepintensitynorth(FILE *fp)
   {
   double range, min, max;
#ifdef DEBUG
printf("---in gnu_stepintensitynorth()---\n");
#endif

   if(Intensity_variation == 1)
       {
       min = Intensity_aging_min;
       max = Intensity_aging_max;
       }
   else
       {
       min = Hetero_range_min;
       max = Hetero_range_max;
       }
   range = max - min;
   fprintf(fp, "set term post eps color\n");
//   fprintf(fp, "set size ratio %lf\n", (double)Max_steps/(double)Pop_size);
   fprintf(fp, "set size ratio 2\n");
   fprintf(fp, "set output \"run.%d.stepintensitynorth.eps\"\n", Run_num);
   fprintf(fp, "set title \"Agent intensities: North\"\n");
   fprintf(fp, "set xlabel \"Agent\"\n");
   fprintf(fp, "set ylabel \"Timestep\"\n");
   fprintf(fp, "set palette maxcolors 100\n");
   fprintf(fp, "set palette defined (0 \"green\", %lf \"yellow\", %lf \"red\")\n", range/2.0, range);
   fprintf(fp, "set cbrange[%lf:%lf]\n", min, max);
   fprintf(fp, "set view map\n");
   fprintf(fp, "plot [%lf:%lf][%d:-2] \"run.%d.stepintensitynorth\" matrix title \"\" w image\n",
           Pop_size*0.1*(-1), Pop_size*1.1, Max_steps+1, Run_num);
   fprintf(fp, "unset size\n");
   fprintf(fp, "unset title\n");
   fprintf(fp, "unset xlabel\n");
   fprintf(fp, "unset ylabel\n");
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end gnu_stepintensitynorth()---\n");
#endif
   }  /* gnu_stepintensitynorth */

/********** gnu_stepintensitysouth ***********/
/* Called by:           fprint_gnu(), gnu.c
   Parameters:
   Actions:             Plot target path.
*/
void gnu_stepintensitysouth(FILE *fp)
   {
   double range, min, max;
#ifdef DEBUG
printf("---in gnu_stepintensitysouth()---\n");
#endif

   if(Intensity_variation == 1)
       {
       min = Intensity_aging_min;
       max = Intensity_aging_max;
       }
   else
       {
       min = Hetero_range_min;
       max = Hetero_range_max;
       }
   range = max - min;
   fprintf(fp, "set term post eps color\n");
//   fprintf(fp, "set size ratio %lf\n", (double)Max_steps/(double)Pop_size);
   fprintf(fp, "set size ratio 2\n");
   fprintf(fp, "set output \"run.%d.stepintensitysouth.eps\"\n", Run_num);
   fprintf(fp, "set title \"Agent intensities: South\"\n");
   fprintf(fp, "set xlabel \"Agent\"\n");
   fprintf(fp, "set ylabel \"Timestep\"\n");
   fprintf(fp, "set palette maxcolors 100\n");
   fprintf(fp, "set palette defined (0 \"green\", %lf \"yellow\", %lf \"red\")\n", range/2.0, range);
   fprintf(fp, "set cbrange[%lf:%lf]\n", min, max);
   fprintf(fp, "set view map\n");
   fprintf(fp, "plot [%lf:%lf][%d:-2] \"run.%d.stepintensitysouth\" matrix title \"\" w image\n",
           Pop_size*0.1*(-1), Pop_size*1.1, Max_steps+1, Run_num);
   fprintf(fp, "unset size\n");
   fprintf(fp, "unset title\n");
   fprintf(fp, "unset xlabel\n");
   fprintf(fp, "unset ylabel\n");
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end gnu_stepintensitysouth()---\n");
#endif
   }  /* gnu_stepintensitysouth */

/********** gnu_stepintensityeast ***********/
/* Called by:           fprint_gnu(), gnu.c
   Parameters:
   Actions:             Plot target path.
*/
void gnu_stepintensityeast(FILE *fp)
   {
   double range, min, max;
#ifdef DEBUG
printf("---in gnu_stepintensityeast()---\n");
#endif

   if(Intensity_variation == 1)
       {
       min = Intensity_aging_min;
       max = Intensity_aging_max;
       }
   else
       {
       min = Hetero_range_min;
       max = Hetero_range_max;
       }
   range = max - min;
   fprintf(fp, "set term post eps color\n");
//   fprintf(fp, "set size ratio %lf\n", (double)Max_steps/(double)Pop_size);
   fprintf(fp, "set size ratio 2\n");
   fprintf(fp, "set output \"run.%d.stepintensityeast.eps\"\n", Run_num);
   fprintf(fp, "set title \"Agent intensities: East\"\n");
   fprintf(fp, "set xlabel \"Agent\"\n");
   fprintf(fp, "set ylabel \"Timestep\"\n");
   fprintf(fp, "set palette maxcolors 100\n");
   fprintf(fp, "set palette defined (0 \"green\", %lf \"yellow\", %lf \"red\")\n", range/2.0, range);
   fprintf(fp, "set cbrange[%lf:%lf]\n", min, max);
   fprintf(fp, "set view map\n");
   fprintf(fp, "plot [%lf:%lf][%d:-2] \"run.%d.stepintensityeast\" matrix title \"\" w image\n",
           Pop_size*0.1*(-1), Pop_size*1.1, Max_steps+1, Run_num);
   fprintf(fp, "unset size\n");
   fprintf(fp, "unset title\n");
   fprintf(fp, "unset xlabel\n");
   fprintf(fp, "unset ylabel\n");
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end gnu_stepintensityeast()---\n");
#endif
   }  /* gnu_stepintensityeast */

/********** gnu_stepintensitywest ***********/
/* Called by:           fprint_gnu(), gnu.c
   Parameters:
   Actions:             Plot target path.
*/
void gnu_stepintensitywest(FILE *fp)
   {
   double range, min, max;
#ifdef DEBUG
printf("---in gnu_stepintensitywest()---\n");
#endif

   if(Intensity_variation == 1)
       {
       min = Intensity_aging_min;
       max = Intensity_aging_max;
       }
   else
       {
       min = Hetero_range_min;
       max = Hetero_range_max;
       }
   range = max - min;
   fprintf(fp, "set term post eps color\n");
//   fprintf(fp, "set size ratio %lf\n", (double)Max_steps/(double)Pop_size);
   fprintf(fp, "set size ratio 2\n");
   fprintf(fp, "set output \"run.%d.stepintensitywest.eps\"\n", Run_num);
   fprintf(fp, "set title \"Agent intensities: West\"\n");
   fprintf(fp, "set xlabel \"Agent\"\n");
   fprintf(fp, "set ylabel \"Timestep\"\n");
   fprintf(fp, "set palette maxcolors 100\n");
   fprintf(fp, "set palette defined (0 \"green\", %lf \"yellow\", %lf \"red\")\n", range/2.0, range);
   fprintf(fp, "set cbrange[%lf:%lf]\n", min, max);
   fprintf(fp, "set view map\n");
   fprintf(fp, "plot [%lf:%lf][%d:-2] \"run.%d.stepintensitywest\" matrix title \"\" w image\n",
           Pop_size*0.1*(-1), Pop_size*1.1, Max_steps+1, Run_num);
   fprintf(fp, "unset size\n");
   fprintf(fp, "unset title\n");
   fprintf(fp, "unset xlabel\n");
   fprintf(fp, "unset ylabel\n");
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end gnu_stepintensitywest()---\n");
#endif
   }  /* gnu_stepintensitywest */

/********** gnu_finalagent ***********/
/* Called by:           fprint_gnu(), gnu.c
   Parameters:
   Actions:             Print gnuplot file.
*/
void gnu_finalagent(FILE *fp)
   {
#ifdef DEBUG
printf("---in gnu_finalagent()---\n");
#endif

   // plot number of times each agent acted on each task
   fprintf(fp, "set term post eps color\n");
   fprintf(fp, "set size ratio 0.5\n");
   fprintf(fp, "set output \"run.%d.finalagent.count.eps\"\n", Run_num);
   fprintf(fp, "set xtics\n");
   fprintf(fp, "set ytics\n");
   fprintf(fp, "set xlabel \"Agent\"\n");
   fprintf(fp, "set ylabel \"Count\"\n");
   fprintf(fp, "set title \"Number of times acted on each task\"\n");
   fprintf(fp, "plot [-1:%lf][0:%lf]\\\n", Pop_size*1.2, Max_steps*1.1);
   fprintf(fp, "     \"run.%d.finalagent\" using 2:5 title \"N\" ", Run_num);
   fprintf(fp,       "w points pt 1 ps 2 lw 2 lc 1,\\\n");
   fprintf(fp, "     \"run.%d.finalagent\" using 2:6 title \"E\" ", Run_num);
   fprintf(fp,       "w points pt 2 ps 2 lw 2 lc 2,\\\n");
   fprintf(fp, "     \"run.%d.finalagent\" using 2:7 title \"S\" ", Run_num);
   fprintf(fp,       "w points pt 4 ps 2 lw 2 lc 3,\\\n");
   fprintf(fp, "     \"run.%d.finalagent\" using 2:8 title \"W\" ", Run_num);
   fprintf(fp,       "w points pt 6 ps 2 lw 2 lc 4,\\\n");
   fprintf(fp, "     \"run.%d.finalagent\" using 2:4 title \"I\" ", Run_num);
   fprintf(fp,       "w points pt 8 ps 2 lc 7\n");
   fprintf(fp, "unset xlabel\n");
   fprintf(fp, "unset ylabel\n");
   fprintf(fp, "unset title\n");
   fprintf(fp, "unset size\n");
   fprintf(fp, "\n");

   // plot final threshold for each task for each agent
   fprintf(fp, "set term post eps color\n");
   fprintf(fp, "set size ratio 0.5\n");
   fprintf(fp, "set output \"run.%d.finalagent.thresh.eps\"\n", Run_num);
   fprintf(fp, "set xtics\n");
   fprintf(fp, "set ytics\n");
   fprintf(fp, "set xlabel \"Agent\"\n");
   fprintf(fp, "set ylabel \"Threshold\"\n");
   fprintf(fp, "set title \"Final threshold for each task, Range = %lf\"\n",
		Range);
   fprintf(fp, "plot [-1:%lf][0:%lf]\\\n", Pop_size*1.2, Range*1.1);
   fprintf(fp, "     \"run.%d.finalagent\" using 2:10 title \"N\" ", Run_num);
   fprintf(fp,       "w points pt 1 ps 2 lw 2 lc 1,\\\n");
   fprintf(fp, "     \"run.%d.finalagent\" using 2:11 title \"E\" ", Run_num);
   fprintf(fp,       "w points pt 2 ps 2 lw 2 lc 2,\\\n");
   fprintf(fp, "     \"run.%d.finalagent\" using 2:12 title \"S\" ", Run_num);
   fprintf(fp,       "w points pt 4 ps 2 lw 2 lc 3,\\\n");
   fprintf(fp, "     \"run.%d.finalagent\" using 2:13 title \"W\" ", Run_num);
   fprintf(fp,       "w points pt 6 ps 2 lw 2 lc 4\n");
   fprintf(fp, "unset xlabel\n");
   fprintf(fp, "unset ylabel\n");
   fprintf(fp, "unset title\n");
   fprintf(fp, "unset size\n");
   fprintf(fp, "\n");

   // plot number of switches for each agent
   fprintf(fp, "set term post eps color\n");
   fprintf(fp, "set size ratio 0.5\n");
   fprintf(fp, "set output \"run.%d.finalagent.switch.eps\"\n", Run_num);
   fprintf(fp, "set xtics\n");
   fprintf(fp, "set ytics\n");
   fprintf(fp, "set xlabel \"Agent\"\n");
   fprintf(fp, "set ylabel \"Number of switches\"\n");
   fprintf(fp, "set title \"Number of task switches during run\"\n");
   fprintf(fp, "plot [-1:%lf][0:%lf]\\\n", Pop_size*1.2, Max_steps*1.1);
   fprintf(fp, "     \"run.%d.finalagent\" using 2:15 title \"N\" ", Run_num);
   fprintf(fp,       "w points pt 7 ps 2 lc 1\n");
   fprintf(fp, "unset xlabel\n");
   fprintf(fp, "unset ylabel\n"); 
   fprintf(fp, "unset title\n");
   fprintf(fp, "unset size\n");
   fprintf(fp, "\n");

   // plot number of switches for each agent -- idle doesn't count as task
   fprintf(fp, "set term post eps color\n");
   fprintf(fp, "set size ratio 0.5\n");
   fprintf(fp, "set output \"run.%d.finalagent.switch2.eps\"\n", Run_num);
   fprintf(fp, "set xtics\n");
   fprintf(fp, "set ytics\n");
   fprintf(fp, "set xlabel \"Agent\"\n");
   fprintf(fp, "set ylabel \"Number of switches\"\n");
   fprintf(fp, "set title \"Number of task switches during run - no idle\"\n");
   fprintf(fp, "plot [-1:%lf][0:%lf]\\\n", Pop_size*1.2, Max_steps*1.1);
   fprintf(fp, "     \"run.%d.finalagent\" using 2:27 title \"N\" ", Run_num);
   fprintf(fp,       "w points pt 7 ps 2 lc 1\n");
   fprintf(fp, "unset xlabel\n");
   fprintf(fp, "unset ylabel\n");
   fprintf(fp, "unset title\n");
   fprintf(fp, "unset size\n");
   fprintf(fp, "\n");

   // 2020.12.09.ASW
   // Plot threshold versus number of times acted for each task
   fprintf(fp, "set term post eps color\n");
   fprintf(fp, "set size ratio 0.7\n");
   fprintf(fp, "set output \"run.%d.finalagent.threshvact.eps\"\n", Run_num);
   fprintf(fp, "set xtics\n");
   fprintf(fp, "set ytics\n");
   fprintf(fp, "set xlabel \"Threshold value\"\n");
   fprintf(fp, "set ylabel \"Action count\"\n");
   fprintf(fp, 
  "set title \"Threshold values versus number of timesteps acting on task\"\n");
   fprintf(fp, "plot [%lf:%lf][0:%lf]\\\n",
                -0.05*Range, 1.05*Range, Max_steps*1.1);
   fprintf(fp, "     \"run.%d.finalagent\" using 10:5 title \"N\" ",
                Run_num);
   fprintf(fp,       "w points pt 7 ps 0.9 lc 1,\\\n");
   fprintf(fp, "     \"run.%d.finalagent\" using 11:6 title \"E\" ",
                Run_num);
   fprintf(fp,       "w points pt 7 ps 0.9 lc 5,\\\n");
   fprintf(fp, "     \"run.%d.finalagent\" using 12:7 title \"S\" ",
                Run_num);
   fprintf(fp,       "w points pt 7 ps 0.9 lc 7,\\\n");
   fprintf(fp, "     \"run.%d.finalagent\" using 13:8 title \"W\" ",
                Run_num);
   fprintf(fp,       "w points pt 7 ps 0.9 lc 2\n");
   fprintf(fp, "unset xlabel\n");
   fprintf(fp, "unset ylabel\n");
   fprintf(fp, "unset title\n");
   fprintf(fp, "unset size\n");
   fprintf(fp, "\n");


   // NB; 2020.06.03
   // plot number of spontaneous switches for each agent
   fprintf(fp, "set term post eps color\n");
   fprintf(fp, "set size ratio 0.5\n");
   fprintf(fp, "set output \"run.%d.finalagent.spontaneousswitch.eps\"\n", Run_num);
   fprintf(fp, "set xtics\n");
   fprintf(fp, "set ytics\n");
   fprintf(fp, "set xlabel \"Agent\"\n");
   fprintf(fp, "set ylabel \"Number of spontaneous switches\"\n");
   fprintf(fp, "set title \"Number of task switches due to spontaneous response prob during run\"\n");
   fprintf(fp, "plot [-1:%lf][0:%lf]\\\n", Pop_size*1.2, Max_steps*1.1);
   fprintf(fp, "     \"run.%d.finalagent\" using 2:17 title \"N\" ", Run_num);
   fprintf(fp,       "w points pt 7 ps 2 lc 1\n");
   fprintf(fp, "unset xlabel\n");
   fprintf(fp, "unset ylabel\n"); 
   fprintf(fp, "unset title\n");
   fprintf(fp, "unset size\n");
   fprintf(fp, "\n");

   // NB; 2020.06.29
   // plot all agents Response prob values
   fprintf(fp, "set term post eps color\n");
   fprintf(fp, "set size ratio 0.5\n");
   fprintf(fp, "set output \"run.%d.finalagent.response_prob.eps\"\n", Run_num);
   fprintf(fp, "set xtics\n");
   fprintf(fp, "set ytics\n");
   fprintf(fp, "set xlabel \"Agent\"\n");
   fprintf(fp, "set ylabel \"Response prob Value\"\n");
   fprintf(fp, "set title \"Agent Response prob Values\"\n");
   fprintf(fp, "plot [-1:%lf][0:1]\\\n", Pop_size*1.1);
   fprintf(fp, "     \"run.%d.finalagent\" using 2:21 title \"N\" ", Run_num);
   fprintf(fp,       "w points pt 7 ps 2 lc 1\n");
   fprintf(fp, "unset xlabel\n");
   fprintf(fp, "unset ylabel\n"); 
   fprintf(fp, "unset title\n");
   fprintf(fp, "unset size\n");
   fprintf(fp, "\n");

   // NB; 2020.06.29
   // plot all agents Spontaneous response prob values
   fprintf(fp, "set term post eps color\n");
   fprintf(fp, "set size ratio 0.5\n");
   fprintf(fp, "set output \"run.%d.finalagent.spontaneous_response_prob.eps\"\n", Run_num);
   fprintf(fp, "set xtics\n");
   fprintf(fp, "set ytics\n");
   fprintf(fp, "set xlabel \"Agent\"\n");
   fprintf(fp, "set ylabel \"Spontaneous Value\"\n");
   fprintf(fp, "set title \"Agent Spontaneous Values\"\n");
   fprintf(fp, "plot [-1:%lf][0:1]\\\n", Pop_size*1.1);
   fprintf(fp, "     \"run.%d.finalagent\" using 2:23 title \"N\" ", Run_num);
   fprintf(fp,       "w points pt 7 ps 2 lc 1\n");
   fprintf(fp, "unset xlabel\n");
   fprintf(fp, "unset ylabel\n"); 
   fprintf(fp, "unset title\n");
   fprintf(fp, "unset size\n");
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end gnu_finalagent()---\n");
#endif
   }  /* gnu_finalagent */

/********** gnu_finalthreshswitch ***********/
/* Called by:           fprint_gnu(), gnu.c
   Parameters:
   Actions:             Print gnuplot file.
*/
void gnu_finalthreshswitch(FILE *fp)
   {
#ifdef DEBUG
printf("---in gnu_finalthreshswitch()---\n");
#endif

   // plot number of times each agent acted on each task
   fprintf(fp, "set term post eps color\n");
   fprintf(fp, "set size ratio 0.7\n");
   fprintf(fp, "set output \"run.%d.finalthreshswitch.eps\"\n", Run_num);
   fprintf(fp, "set xtics\n");
   fprintf(fp, "set ytics\n");
   fprintf(fp, "set xlabel \"Threshold value\"\n");
   fprintf(fp, "set ylabel \"Switch count\"\n");
   fprintf(fp, "set title \"Threshold values versus switch count\"\n");
   fprintf(fp, "plot [%lf:%lf][0:%lf]\\\n", 
		-0.05*Range, 1.05*Range, Max_steps*1.1);
   fprintf(fp, "     \"run.%d.finalthreshswitch\" using 2:1 title \"N\" ",
		Run_num);
   fprintf(fp,       "w points pt 7 ps 0.9 lc 1,\\\n");
   fprintf(fp, "     \"run.%d.finalthreshswitch\" using 3:1 title \"E\" ",
		Run_num);
   fprintf(fp,       "w points pt 7 ps 0.9 lc 5,\\\n");
   fprintf(fp, "     \"run.%d.finalthreshswitch\" using 4:1 title \"S\" ",
		Run_num);
   fprintf(fp,       "w points pt 7 ps 0.9 lc 7,\\\n");
   fprintf(fp, "     \"run.%d.finalthreshswitch\" using 5:1 title \"W\" ",
		Run_num);
   fprintf(fp,       "w points pt 7 ps 0.9 lc 2\n");
   fprintf(fp, "unset xlabel\n");
   fprintf(fp, "unset ylabel\n");
   fprintf(fp, "unset title\n");
   fprintf(fp, "unset size\n");
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end gnu_finalthreshswitch()---\n");
#endif
   }  /* gnu_finalthreshswitch */

/********** gnu_finalthreshact ***********/
/* Called by:           fprint_gnu(), gnu.c
   Created:		21.09.10.AW
   Parameters:
   Actions:             Print histograms of agent threshold and agent actions
			for each task.
*/
void gnu_finalthreshact(FILE *fp)
   {
   int r;
   int period = 7;
   int total_act = 0;		// total activations
   int total_switch = 0;	// total task switches
   int total_switch_noidle = 0;	// total task switches
#ifdef DEBUG
printf("---in gnu_finalthreshact()---\n");
#endif

   // calculate total activations
   for (r=0; r<Pop_size; r++)
      {
      total_act += Agent[r].count_north + Agent[r].count_east
           + Agent[r].count_south + Agent[r].count_west;
      total_switch += Agent[r].count_switch;
      total_switch_noidle += Agent[r].count_switch_noidle;
      }

   // plot each agent's final thresholds for each task
   fprintf(fp, "set term post eps size 5.6,1\n");
   fprintf(fp, "set size ratio 0.1\n");

   // print commands to plot threshold
   fprintf(fp, "set output \"run.%d.finalthreshact.thresh.eps\"\n", Run_num);
   fprintf(fp, "set title \"Agent thresholds (Range - threshold), NESW (run.%d)\"\n", Run_num);
   fprintf(fp, "set ytics 5\n");
   // set xtics
   fprintf(fp, "set xtics ( ");
   for (r=0; r<Pop_size; r++)
      {
      fprintf(fp, " \"N\" %d 1, ", r*period+1);
      fprintf(fp, " \"E\" %d 1, ", r*period+2);
      fprintf(fp, " \"S\" %d 1, ", r*period+3);
      fprintf(fp, " \"W\" %d 1, ", r*period+4);
      }
   fprintf(fp, " )\n");
   fprintf(fp, "plot [-%d:%d][-%lf:%lf]\\\n", 
           period, period*(Pop_size+1), 0.1*Range, 1.1*Range);
   for (r=0; r<Pop_size-1; r++)
      {
      fprintf(fp,
         "   \"run.%d.finalthreshact\" using ($2+%d):(%lf-$%d) title \"\" w impulse lt 1,\\\n",
         Run_num, period*r, Range, r+5);
      }
   r = Pop_size-1;
   fprintf(fp,
      "   \"run.%d.finalthreshact\" using ($2+%d):(%lf-$%d) title \"\" w impulse lt 1\n",
      Run_num, period*r, Range, r+5);
   fprintf(fp, "unset xlabel\n");
   fprintf(fp, "unset ylabel\n");
   fprintf(fp, "unset title\n");
   fprintf(fp, "unset size\n");
   fprintf(fp, "\n");

   // plot number of times each agent acted on each task
   fprintf(fp, "set term post eps size 5.6,1\n");
   fprintf(fp, "set size ratio 0.1\n");

   // print commands to plot actions
   fprintf(fp, "set output \"run.%d.finalthreshact.act.eps\"\n", Run_num);
   fprintf(fp, "set title \"Agent action counts (total act %d, switch %d, switch-noidle %d), NESW (run.%d)\"\n", 
           total_act, total_switch, total_switch_noidle, Run_num);
   fprintf(fp, "set ytics %d\n", Max_steps/2);
   // set xtics
   fprintf(fp, "set xtics ( ");
   for (r=0; r<Pop_size; r++)
      {
      fprintf(fp, " \"N\" %d 1, ", r*period+1);
      fprintf(fp, " \"E\" %d 1, ", r*period+2);
      fprintf(fp, " \"S\" %d 1, ", r*period+3);
      fprintf(fp, " \"W\" %d 1, ", r*period+4);
      }
   fprintf(fp, " )\n");

   fprintf(fp, "plot [-%d:%d][-%lf:%lf]\\\n",
           period, period*(Pop_size+1), 0.1*Max_steps, 1.1*Max_steps);
   for (r=0; r<Pop_size-1; r++)
      {
      fprintf(fp,
         "   \"run.%d.finalthreshact\" using ($2+%d):%d title \"\" w impulse lt 1,\\\n",
         Run_num, period*r, r+5+1+Pop_size);
      }
   r = Pop_size-1;
   fprintf(fp,
      "   \"run.%d.finalthreshact\" using ($2+%d):%d title \"\" w impulse lt 1\n",
      Run_num, period*r, r+5+1+Pop_size);
   fprintf(fp, "unset xlabel\n");
   fprintf(fp, "unset ylabel\n");
   fprintf(fp, "unset title\n");
   fprintf(fp, "unset size\n");
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end gnu_finalthreshact()---\n");
#endif
   }  /* gnu_finalthreshact */

/********** gnu_stepprobnorth ***********/
/* Called by:           fprint_gnu(), gnu.c
   Parameters:
   Actions:             Plot target path.
*/
void gnu_stepprobnorth(FILE *fp)
   {
#ifdef DEBUG
printf("---in gnu_stepprobnorth()---\n");
#endif
   
   fprintf(fp, "set term post eps color\n");
//   fprintf(fp, "set size ratio %lf\n", (double)Max_steps/(double)Pop_size);
   fprintf(fp, "set size ratio 2\n");
   fprintf(fp, "set output \"run.%d.stepprobnorth.eps\"\n", Run_num);
   fprintf(fp, "set title \"Response probabilities: North\"\n");
   fprintf(fp, "set xlabel \"Agent\"\n");
   fprintf(fp, "set ylabel \"Timestep\"\n");
   fprintf(fp, "set palette maxcolors 100\n");
   fprintf(fp, "set palette defined (0 \"red\", 0.5 \"yellow\", 1.0 \"green\")\n");
   fprintf(fp, "set cbrange[0.0:1.0]\n");
   fprintf(fp, "set view map\n");
   fprintf(fp, "plot [%lf:%lf][%d:-2] \"run.%d.stepprobnorth\" matrix title \"\" w image\n",
           Pop_size*0.1*(-1), Pop_size*1.1, Max_steps+1, Run_num);
   fprintf(fp, "unset size\n");
   fprintf(fp, "unset title\n");
   fprintf(fp, "unset xlabel\n");
   fprintf(fp, "unset ylabel\n");
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end gnu_stepprobnorth()---\n");
#endif
   }  /* gnu_stepprobnorth */

/********** gnu_stepprobeast ***********/
/* Called by:           fprint_gnu(), gnu.c
   Parameters:
   Actions:             Plot target path.
*/
void gnu_stepprobeast(FILE *fp)
   {
#ifdef DEBUG
printf("---in gnu_stepprobeast()---\n");
#endif
   
   fprintf(fp, "set term post eps color\n");
//   fprintf(fp, "set size ratio %lf\n", (double)Max_steps/(double)Pop_size);
   fprintf(fp, "set size ratio 2\n");
   fprintf(fp, "set output \"run.%d.stepprobeast.eps\"\n", Run_num);
   fprintf(fp, "set title \"Response probabilities: East\"\n");
   fprintf(fp, "set xlabel \"Agent\"\n");
   fprintf(fp, "set ylabel \"Timestep\"\n");
   fprintf(fp, "set palette maxcolors 100\n");
   fprintf(fp, "set palette defined (0 \"red\", 0.5 \"yellow\", 1.0 \"green\")\n");
   fprintf(fp, "set cbrange[0.0:1.0]\n");
   fprintf(fp, "set view map\n");
   fprintf(fp, "plot [%lf:%lf][%d:-2] \"run.%d.stepprobeast\" matrix title \"\" w image\n",
           Pop_size*0.1*(-1), Pop_size*1.1, Max_steps+1, Run_num);
   fprintf(fp, "unset size\n");
   fprintf(fp, "unset title\n");
   fprintf(fp, "unset xlabel\n");
   fprintf(fp, "unset ylabel\n");
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end gnu_stepprobeast()---\n");
#endif
   }  /* gnu_stepprobeast */

/********** gnu_stepprobsouth ***********/
/* Called by:           fprint_gnu(), gnu.c
   Parameters:
   Actions:             Plot target path.
*/
void gnu_stepprobsouth(FILE *fp)
   {
#ifdef DEBUG
printf("---in gnu_stepprobsouth()---\n");
#endif
   
   fprintf(fp, "set term post eps color\n");
//   fprintf(fp, "set size ratio %lf\n", (double)Max_steps/(double)Pop_size);
   fprintf(fp, "set size ratio 2\n");
   fprintf(fp, "set output \"run.%d.stepprobsouth.eps\"\n", Run_num);
   fprintf(fp, "set title \"Response probabilities: South\"\n");
   fprintf(fp, "set xlabel \"Agent\"\n");
   fprintf(fp, "set ylabel \"Timestep\"\n");
   fprintf(fp, "set palette maxcolors 100\n");
   fprintf(fp, "set palette defined (0 \"red\", 0.5 \"yellow\", 1.0 \"green\")\n");
   fprintf(fp, "set cbrange[0.0:1.0]\n");
   fprintf(fp, "set view map\n");
   fprintf(fp, "plot [%lf:%lf][%d:-2] \"run.%d.stepprobsouth\" matrix title \"\" w image\n",
           Pop_size*0.1*(-1), Pop_size*1.1, Max_steps+1, Run_num);
   fprintf(fp, "unset size\n");
   fprintf(fp, "unset title\n");
   fprintf(fp, "unset xlabel\n");
   fprintf(fp, "unset ylabel\n");
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end gnu_stepprobsouth()---\n");
#endif
   }  /* gnu_stepprobsouth */

/********** gnu_stepprobwest ***********/
/* Called by:           fprint_gnu(), gnu.c
   Parameters:
   Actions:             Plot target path.
*/
void gnu_stepprobwest(FILE *fp)
   {
#ifdef DEBUG
printf("---in gnu_stepprobwest()---\n");
#endif
   
   fprintf(fp, "set term post eps color\n");
//   fprintf(fp, "set size ratio %lf\n", (double)Max_steps/(double)Pop_size);
   fprintf(fp, "set size ratio 2\n");
   fprintf(fp, "set output \"run.%d.stepprobwest.eps\"\n", Run_num);
   fprintf(fp, "set title \"Response probabilities: West\"\n");
   fprintf(fp, "set xlabel \"Agent\"\n");
   fprintf(fp, "set ylabel \"Timestep\"\n");
   fprintf(fp, "set palette maxcolors 100\n");
   fprintf(fp, "set palette defined (0 \"red\", 0.5 \"yellow\", 1.0 \"green\")\n");
   fprintf(fp, "set cbrange[0.0:1.0]\n");
   fprintf(fp, "set view map\n");
   fprintf(fp, "plot [%lf:%lf][%d:-2] \"run.%d.stepprobwest\" matrix title \"\" w image\n",
           Pop_size*0.1*(-1), Pop_size*1.1, Max_steps+1, Run_num);
   fprintf(fp, "unset size\n");
   fprintf(fp, "unset title\n");
   fprintf(fp, "unset xlabel\n");
   fprintf(fp, "unset ylabel\n");
   fprintf(fp, "\n");

#ifdef DEBUG
printf("---end gnu_stepprobwest()---\n");
#endif
   }  /* gnu_stepprobwest */
