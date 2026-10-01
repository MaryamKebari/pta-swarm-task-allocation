/* freport.c
   Report-based feedback for decentralized PTA.

   With Report_feedback = 0 (the default) nothing in this file runs and the
   simulator uses the shared feedback channel computed in fxn.c.

   With Report_feedback = 1, every live agent i forms its own estimate of the
   paired feedback for each task j:

      o[i,j]   = e[j] + sigma * z[i,j]          private observation
      m[k,j]   = o[k,j]                         report sent by honest peer k
      ehat[i,j] = o[i,j] + sum_k w[i,k] * (m[k,j] - o[i,j]) / (1 + sum_k w[i,k])

   where e[j] is the shared feedback from update_pre_service_task_error(),
   sigma = Obs_noise_alpha * (mean positive task demand), and k ranges over
   Report_k peers sampled each step (Report_k <= 0 means all live agents).
   Each agent keeps its own previous feedback and leaky, bounded integral, so
   the PID terms become per agent.

   The estimate is written as a correction to the agent's own observation so
   that, with sigma = 0 and honest peers, every difference is exactly zero
   and ehat[i,j] equals e[j] bit for bit. That reduces this mode to the
   shared-feedback simulator, which tests/test_report_feedback.py checks.

   All draws use a counter-based hash stream seeded by Report_seed, so this
   mode never consumes the agent random stream used for task choice.
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "extern.h"
#include "freport.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static double *Obs = NULL;            /* o[i,j] for the current step */
static double *Feedback = NULL;       /* ehat[i,j] */
static double *Prev_feedback = NULL;  /* ehat[i,j] at the previous step */
static double *Integral = NULL;       /* per-agent leaky integral */
static int *Peers = NULL;             /* scratch list of sampled peers */
static int Stride = 0;

#define IDX(agent, task) ((size_t)(agent) * (size_t)Stride + (size_t)(task))

static unsigned long long report_mix(unsigned long long x)
   {
   x += 0x9e3779b97f4a7c15ULL;
   x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
   x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
   return x ^ (x >> 31);
   }  /* report_mix */

/* Uniform (0,1) draw keyed by step, agent, slot and salt. */
static double report_uniform(int agent, int slot, int salt)
   {
   unsigned long long x = (unsigned long long)Report_seed;
   x ^= ((unsigned long long)(Current_timestep + 1)) * 0x9e3779b97f4a7c15ULL;
   x ^= ((unsigned long long)(agent + 7)) * 0xd1b54a32d192ed03ULL;
   x ^= ((unsigned long long)(slot + 17)) * 0xbf58476d1ce4e5b9ULL;
   x ^= ((unsigned long long)(salt + 101)) * 0x94d049bb133111ebULL;
   x = report_mix(x);
   return ((double)(x >> 11) + 0.5) * (1.0 / 9007199254740992.0);
   }  /* report_uniform */

static double report_normal(int agent, int task)
   {
   double u1 = report_uniform(agent, task, 0);
   double u2 = report_uniform(agent, task, 1);
   return sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
   }  /* report_normal */

static double observation_scale(void)
   {
   int task;
   int active = 0;
   double active_sum = 0.0;

   for (task=1; task<=Num_tasks; task++)
      {
      if (Task_demand[task] > 0.0)
         {
         active_sum += Task_demand[task];
         active++;
         }
      }
   return (active > 0) ? active_sum / (double)active : 0.0;
   }  /* observation_scale */

/* Fill Peers with up to Report_k distinct live agents other than i.
   Returns the number of peers. Report_k <= 0, or a sample at least as large
   as the live population, selects every other live agent. */
static int sample_peers(int i)
   {
   int k;
   int count = 0;
   int attempts = 0;
   int others = Num_alive - (Agent[i].dead ? 0 : 1);

   if (Report_k <= 0 || Report_k >= others)
      {
      for (k=0; k<Pop_size; k++)
         if (k != i && !Agent[k].dead)
            Peers[count++] = k;
      return count;
      }

   while (count < Report_k && attempts < 64 * Report_k)
      {
      int c;
      int duplicate = 0;
      int candidate =
         (int)(report_uniform(i, MAX_TASKS + 1 + attempts, 2) * Pop_size);

      attempts++;
      if (candidate >= Pop_size) candidate = Pop_size - 1;
      if (candidate == i || Agent[candidate].dead) continue;
      for (c=0; c<count; c++)
         if (Peers[c] == candidate) { duplicate = 1; break; }
      if (!duplicate) Peers[count++] = candidate;
      }
   return count;
   }  /* sample_peers */

int init_report_feedback(void)
   {
   size_t cells;

   if (!Report_feedback) return OK;

   Stride = Num_tasks + 1;
   cells = (size_t)Pop_size * (size_t)Stride;
   Obs = (double *)calloc(cells, sizeof(double));
   Feedback = (double *)calloc(cells, sizeof(double));
   Prev_feedback = (double *)calloc(cells, sizeof(double));
   Integral = (double *)calloc(cells, sizeof(double));
   Peers = (int *)calloc((size_t)Pop_size, sizeof(int));
   if (!Obs || !Feedback || !Prev_feedback || !Integral || !Peers)
      {
      printf(" Error(init_report_feedback): out of memory\n");
      return ERROR;
      }
   return OK;
   }  /* init_report_feedback */

void end_report_feedback(void)
   {
   free(Obs);
   free(Feedback);
   free(Prev_feedback);
   free(Integral);
   free(Peers);
   Obs = Feedback = Prev_feedback = Integral = NULL;
   Peers = NULL;
   }  /* end_report_feedback */

/* Called once per step after update_pre_service_task_error(), before any
   agent selects a task. */
void update_report_feedback(void)
   {
   int i, j, p;
   double sigma;

   if (!Report_feedback) return;

   sigma = (Obs_noise_alpha > 0.0) ? Obs_noise_alpha * observation_scale() : 0.0;

   for (i=0; i<Pop_size; i++)
      {
      if (Agent[i].dead) continue;
      for (j=1; j<=Num_tasks; j++)
         {
         Obs[IDX(i, j)] = Task_feedback_error[j];
         if (sigma > 0.0)
            Obs[IDX(i, j)] += sigma * report_normal(i, j);
         }
      }

   for (i=0; i<Pop_size; i++)
      {
      int num_peers;

      if (Agent[i].dead) continue;
      num_peers = sample_peers(i);

      for (j=1; j<=Num_tasks; j++)
         {
         double own = Obs[IDX(i, j)];
         double correction = 0.0;
         double weight_sum = 1.0;
         double estimate;
         double *integral = &Integral[IDX(i, j)];

         for (p=0; p<num_peers; p++)
            {
            double report = Obs[IDX(Peers[p], j)];
            correction += report - own;
            weight_sum += 1.0;
            }
         estimate = own + correction / weight_sum;

         Prev_feedback[IDX(i, j)] = Feedback[IDX(i, j)];
         Feedback[IDX(i, j)] = estimate;

         /* Same order as the shared integral in fxn.c: accumulate, leak,
            then clamp. */
         *integral += estimate;
         if (Pid_integral_leak > 0.0 && Pid_integral_leak < 1.0)
            *integral *= Pid_integral_leak;
         if (Pid_integral_bound > 0.0)
            {
            if (*integral > Pid_integral_bound)
               *integral = Pid_integral_bound;
            else if (*integral < -Pid_integral_bound)
               *integral = -Pid_integral_bound;
            }
         }
      }
   }  /* update_report_feedback */

double report_feedback(int agent_num, int task)
   {
   return Feedback[IDX(agent_num, task)];
   }

double report_prev_feedback(int agent_num, int task)
   {
   return Prev_feedback[IDX(agent_num, task)];
   }

double report_error_integral(int agent_num, int task)
   {
   return Integral[IDX(agent_num, task)];
   }
