/* main.c
   08.05.26.AW	Created.
*/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "types.h"
#include "sim.h"
#include "ftarget.h"

int main(int argc, char **argv)
   {
   int error;
   char time_string[100];
   char *s;
   time_t *tstart, *tend;
   if (argc < 3)
      {
      printf(" Usage: %s <parameter file> <opfiles file>\n",
		argv[0]);
      return ERROR;
      }  /* if */

   printf(" parameter file: %s\n", argv[1]);
   printf(" opfiles file: %s\n", argv[2]);

  /* get start time */
   tstart = (time_t *)malloc(sizeof(time_t));
   time(tstart);
   strftime(time_string, 99, " %m/%d/%y  %H:%M:%S ", localtime(tstart)); 
   printf(" Start time: %s\n", time_string);

   error = run_sim(argv[1], argv[2]);
   if (error == ERROR)  printf( "Error(main): run_sim ends on error.\n");
   else                 printf(" --- run_sim() ends ok ---\n");

  /* get end time */
   tend = (time_t *)malloc(sizeof(time_t));
   time(tend);
   strftime(time_string, 99, " %m/%d/%y  %H:%M:%S ", localtime(tend));
   printf(" End time: %s\n\n", time_string);

   free(tstart);
   free(tend);

   return OK;
   }  /* main */

