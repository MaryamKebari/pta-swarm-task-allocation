/* gnu.h
   19.07.30.AW	Created.
*/

/* prototypes */
void plot_gnuplot_files();
void fprint_gnu(FILE *fp);
void gnu_stephistnorth(FILE *fp);
void gnu_stephistsouth(FILE *fp);
void gnu_stephisteast(FILE *fp);
void gnu_stephistwest(FILE *fp);
void gnu_stepdemand(FILE *fp);
void gnu_stepsummary(FILE *fp);
void gnu_finaltask(FILE *fp);
void gnu_steptaskthresh(FILE *fp);
void gnu_steptaskcounts(FILE *fp);
void gnu_steptaskdemand(FILE *fp);
void gnu_steptargetpath(FILE *fp);
void gnu_steptrackerpath(FILE *fp);
void gnu_stepbothpaths(FILE *fp);
void gnu_initpop(FILE *fp);
void gnu_finalpop(FILE *fp);
void gnu_stepnorthsouth(FILE *fp);
void gnu_stepeastwest(FILE *fp);
void gnu_stepagentaction(FILE *fp);
void gnu_stepagentactionwtime(FILE *fp);
void gnu_stepagentmintask(FILE *fp);
void gnu_stepagentmintaskaction(FILE *fp);
void gnu_stepthreshnorth(FILE *fp);
void gnu_stepthreshsouth(FILE *fp);
void gnu_stepthresheast(FILE *fp);
void gnu_stepthreshwest(FILE *fp);
void gnu_threshrange(FILE *fp);
void gnu_intensityrange(FILE *fp);
void gnu_stepintensitynorth(FILE *fp);
void gnu_stepintensitysouth(FILE *fp);
void gnu_stepintensityeast(FILE *fp);
void gnu_stepintensitywest(FILE *fp);
void gnu_finalagent(FILE *fp);
void gnu_finalthreshswitch(FILE *fp);
void gnu_finalthreshact(FILE *fp);
void gnu_stepprobnorth(FILE *fp);
void gnu_stepprobeast(FILE *fp);
void gnu_stepprobsouth(FILE *fp);
void gnu_stepprobwest(FILE *fp);
