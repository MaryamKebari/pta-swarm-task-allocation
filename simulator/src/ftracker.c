 /* ftracker.c
   19.07.16.AW	Created.
		File containing target movement functions.
*/

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "types.h"
#include "extern.h"
#include "ftracker.h"
#include "fxn.h"
#include "random.h"

// for debugging -- the code within these defines can be deleted.
//#define TEMP
//#define TEMP2
//#define PUSH

// #define HISTOGRAM_SIZE (int)((Intensity_aging_max - Intensity_aging_min) * 10)
#define HISTOGRAM_SIZE 10

static double task_stimulus(int tasknum)
   {
   double feedback;

   if (tasknum <= 0 || tasknum > MAX_TASKS) return 0.0;

   /* In the paired-demand experiments, both the selector and the adaptive
      update receive the same delivered feedback.  Clean feedback therefore
      reduces exactly to max(e, 0), while noise or bias can alter eligibility. */
   if (!strncmp(Task_demand_pattern, "legacy_vector", 13))
      {
      feedback = Task_feedback_error[tasknum];
      return (feedback > 0.0) ? feedback : 0.0;
      }

   return Task_demand[tasknum];
   }

static double agent_task_threshold(int agent_num, int tasknum)
   {
   double threshold;
   if (tasknum <= 0 || tasknum > MAX_TASKS) return 0.0;
   threshold = Agent[agent_num].thresh_task[tasknum];

   /* PID may keep latent values outside the physical threshold range so
      agents do not lose individual history at either boundary. Task choice
      still uses the effective clamped threshold. */
   if (threshold < Agent[agent_num].thresh_min_task[tasknum])
      threshold = Agent[agent_num].thresh_min_task[tasknum];
   if (threshold > Agent[agent_num].thresh_max_task[tasknum])
      threshold = Agent[agent_num].thresh_max_task[tasknum];
   return threshold;
   }

/********** move_tracker **********/
/* parameters:	t	current timestep
   called by:   step_run(), fxn.c
   actions:
*/
int move_tracker(int t)
   {

#ifdef DEBUG
printf("---in move_tracker()---\n");
#endif

   update_task_demands(t);
   update_pre_service_task_error();
   if (select_tasks() == ERROR)  return ERROR;

   zero_pushes();
    
   // HDM; 2020.03.19
   // if killing agents enabled, check if should occur this time step
   if(Kill_number > 0)
      {
      if(First_extinction == t || 
         (Extinction_period && 
          (t != 0) && 
          (t - First_extinction > 0) &&
          ((t - First_extinction) % Extinction_period == 0)
         )
        )
         kill_agents(t);
      }
    
   // HDM; 2019.09.19
   if(Intensity_aging && Intensity_variation)
       {
       update_intensities();
       }
   count_task_actors();
   apply_task_service();
   update_tracker_task_vector();

#ifdef DEBUG
printf("---end move_tracker()---\n");
#endif

   return OK;
   }  /* move_tracker */

/********** get_distances **********/
/* parameters:
   called by:   move_tracker()
   actions:	get distance between target and tracker,
		absolute, and in individual directions
*/
void get_distances()
   {

#ifdef DEBUG
printf("---in get_distances()---\n");
#endif

   Tracker.difference = sqrt( (Target.x - Tracker.x) * (Target.x - Tracker.x) +
                    (Target.y - Tracker.y) * (Target.y - Tracker.y) );
//  191004.AW  Moved this to step_run() in fxn.c
//   Tracker.total_difference += Tracker.difference;
//
//   if (Tracker.difference > Tracker.max_difference)
//      Tracker.max_difference = Tracker.difference;
//   if (Tracker.difference < Tracker.min_difference)
//      Tracker.min_difference = Tracker.difference;
   Tracker.prev_diff_to_north = Tracker.diff_to_north;
   Tracker.prev_diff_to_south = Tracker.diff_to_south;
   Tracker.prev_diff_to_east = Tracker.diff_to_east;
   Tracker.prev_diff_to_west = Tracker.diff_to_west;
   Tracker.diff_to_north = 0;
   Tracker.diff_to_south = 0;
   Tracker.diff_to_east = 0;
   Tracker.diff_to_west = 0;
   Tracker.time=Tracker.time+1;
#ifdef DEBUG
   printf("      prev_diff_to_north %lf prev_diff_to_south %lf",
	Tracker.prev_diff_to_north, Tracker.prev_diff_to_south);
   printf(" prev_diff_to_east %lf prev_diff_to_west %lf\n",
	Tracker.prev_diff_to_east, Tracker.prev_diff_to_west);
#endif
   
   
   if (Target.y > Tracker.y)
      {
      Tracker.diff_to_north = Target.y - Tracker.y;
      }
   else if (Target.y < Tracker.y)
      {
      Tracker.diff_to_south = Tracker.y - Target.y;
      }

   if (Target.x > Tracker.x)
      {
      Tracker.diff_to_east = Target.x - Tracker.x;
      }
   else if (Target.x < Tracker.x)
      {
      Tracker.diff_to_west = Tracker.x - Target.x;
      }
#ifdef DEBUG
printf("      diff_to_north %lf diff_to_south %lf",
	Tracker.diff_to_north, Tracker.diff_to_south);
   printf(" diff_to_east %lf diff_to_west %lf\n",
	Tracker.diff_to_east, Tracker.diff_to_west);
#endif

   Tracker.sum_diff_to_north=Tracker.sum_diff_to_north+ Tracker.diff_to_north - Tracker.diff_to_south;
   Tracker.sum_diff_to_east=Tracker.sum_diff_to_east+Tracker.diff_to_east - Tracker.diff_to_west;
   Tracker.sum_diff_to_south=Tracker.sum_diff_to_south+ Tracker.diff_to_south - Tracker.diff_to_north;
   Tracker.sum_diff_to_west=Tracker.sum_diff_to_west+Tracker.diff_to_west - Tracker.diff_to_east;
 
#ifdef TEMP
   printf(" *** get_distances()\n");
   printf("      dist %lf target at %lf %lf tracker at %lf %lf\n",
          Tracker.difference, Target.x, Target.y, Tracker.x, Tracker.y);
   printf("      diff_to_north %lf diff_to_south %lf",
	Tracker.diff_to_north, Tracker.diff_to_south);
   printf(" diff_to_east %lf diff_to_west %lf\n",
	Tracker.diff_to_east, Tracker.diff_to_west);
#endif

#ifdef DEBUG
printf("---end get_distances()---\n");
#endif
   }  /* get_distances */

int select_tasks()
   {
   int i;
   double rand_num;

#ifdef DEBUG
printf("---in select_tasks()---\n");
#endif

   // initialize count of number of task switches in this timestep
   // as well as the number of actors in this timestep
   Tracker.num_switch = 0;
   Tracker.num_switch_noidle = 0;
   Tracker.num_prev_actors = 0;

   // all live agents get a chance (subject to making it thru Prob_check 
   // and Response_prob) to select a task to work on in next timestep
   //
   // 2020-06-09-AW:
   // Move Response_prob code from this function into agent_select_task()
   for (i=0; i<Pop_size; i++)
      {
      if(!Agent[i].dead)
         {
         // prepare for tracking statistics on agent task switching
         Agent[i].previous_task = Agent[i].current_task;
         // to calculate switch_noidle, also save the previous non-idle tasks
         if (Agent[i].current_task > 0)
            Agent[i].previous_task_noidle = Agent[i].current_task;

         // variable task duration
         // select task with probability p = agent.prob_check
         // remain on same task with probability 1 - p
         rand_num = knuth_random();
         if (rand_num <= Agent[i].prob_check)
            {
            if (agent_select_task(i) == ERROR)  return ERROR;
            // NB; Spontaneous_response_prob; 2020.06.23
            if (Agent[i].current_task == 0)  // If an agent has not already selected a task
               {
               if (agent_select_task_spontaneous(i) == ERROR) return ERROR;
               }
            }

         /* track agent activity statistics */
         /* check to see if agent switched tasks */
         if (Agent[i].current_task != Agent[i].previous_task)
            {
            Agent[i].count_switch++;
            Tracker.num_switch++;
            }
         // record number of actors in previous step
         if (Agent[i].previous_task > 0 && Agent[i].previous_task <= Num_tasks)  
            {
            Tracker.num_prev_actors++;
            }
         /* update count of how many times agents worked on each task */
         if (Agent[i].current_task > 0 && Agent[i].current_task <= Num_tasks)
            {
            Agent[i].count_task[Agent[i].current_task]++;
            if (Agent[i].current_task == 1)       Agent[i].count_north++;
            else if (Agent[i].current_task == 2)  Agent[i].count_east++;
            else if (Agent[i].current_task == 3)  Agent[i].count_south++;
            else if (Agent[i].current_task == 4)  Agent[i].count_west++;
            }
         else Agent[i].count_idle++;

         /* Do save as above but don't count idle as a task */
         /* check to see if agent switched tasks */
         if (Agent[i].previous_task_noidle > 0 &&
             Agent[i].current_task != 0 &&
             Agent[i].current_task != Agent[i].previous_task_noidle)
            {
            Agent[i].count_switch_noidle++;
            Tracker.num_switch_noidle++;
            }

         /* 200414AW Adjust agent thresholds here if dynamic thresholds? */
         if (Thresh_dynamic == 1 || Thresh_dynamic == 2 ||
             Thresh_dynamic == 3)
            {
            adjust_agent_thresholds(i, Agent[i].current_task);
            }

         /* 220404.AW Adjust agent probabilities if dynamic probabilities */
         if (Prob_dynamic == 1)
            {
            adjust_agent_probabilities(i, Agent[i].current_task);
            }
         }
      }

   // track two different percentages:
   //   1.  percent of actors in prev timestep that switched tasks --
   //       denominator does not include idle agents; as a result,
   //       a percent above 100% means that some previously idle agents
   //       decided to start acting in a given timestep.
   //   2.  percent of total population of agents that swtiched tasks
   //       (in this case switching tasks includes switching from idle
   //       to an active task)
   if (Tracker.num_prev_actors == 0)
      {
      Tracker.pct_actors_switch = 0;
      Tracker.pct_all_switch = 0;
      }
   else
      {
      Tracker.pct_actors_switch = 
         (double)Tracker.num_switch/Tracker.num_prev_actors*100;
      Tracker.pct_all_switch = (double)Tracker.num_switch/Pop_size*100;
      }

#ifdef DEBUG
printf("---end select_tasks()---\n");
#endif
   return OK;
   }  /* select_tasks */

/********** agent_select_task **********/
/* parameters:	agent_num		index of agent
   called by:   select_tasks()
   actions:     A single agent chooses an action for the next time
		step -- choose an abstract task channel or idle.
		If more than one task channel has unmet demand,
		agent selects one randomly for now. May try other strategies
		in the future.
		1..Num_tasks = task channels, 0 = not active.
*/
int agent_select_task(int agent_num)
   {
   int j;
   int active_tasks[MAX_TASKS + 1];
   int sum;		// count # tasks that need work
   int chosen_task;
   double rand_num;

#ifdef DEBUG
printf("---in agent_select_task()---\n");
#endif

#ifdef TEMP2
   printf(" *** agent_select_task()\n");
#endif

   sum = 0;

   /* zero out all task channels */
   for (j=0; j<=MAX_TASKS; j++)  active_tasks[j] = 0;

   /* check each selectable task channel */
   for (j=1; j<=Num_tasks; j++)
      {
      if (task_stimulus(j) > 0 &&
          agent_task_threshold(agent_num, j) < task_stimulus(j))
         {
         active_tasks[j] = 1;
         sum++;
         }
      }

   // count number of times multiple thresholds met for an agent
   if(sum >= 2)
      Agent[agent_num].count_multi_tasks += 1;
    
   /* each agent chooses from 0=none and 1..Num_tasks task channels */
   if ( strcmp(Task_selection, "random") == 0 )
      {
      chosen_task = choose_random(agent_num, sum, active_tasks);
      }
   else if ( strcmp(Task_selection, "urgent") == 0 )
      {
      chosen_task = choose_urgent(agent_num, sum, active_tasks);
      }
   else
      {
      printf(
      " Error(agent_select_task): Invalid value for Task_selection: %s\n",
		Task_selection);
      return ERROR;
      }

   // 220404.ASW:  apply individual task response probability
   rand_num = knuth_random();
//printf("      a %d chosen_task %d randnum %lf resprob %lf ",
//agent_num, chosen_task, rand_num, Agent[agent_num].resprob[chosen_task]);
   if (chosen_task != 0 && rand_num <= Agent[agent_num].resprob[chosen_task])
      {
      Agent[agent_num].current_task = chosen_task;
      }
   else
      {
      Agent[agent_num].current_task = 0;
      }
//printf(" chose: %d\n", Agent[agent_num].current_task);

//rp 
// 220404.ASW: need to delete section below
   // 2020-06-09.AW:  apply response probability:
   // Agent takes on chosen task with probability given by its own 
   // response_prob; otherwise agent chooses to become idle.
//rp start of block to delete
/*
   rand_num = knuth_random();
   if (rand_num <= Agent[agent_num].response_prob)
      {
      Agent[agent_num].current_task = chosen_task;
      }
   else
      {
      Agent[agent_num].current_task = 0;
      }
*/
//rp end of block to delete

#ifdef DEBUG
printf("---end agent_select_task()---\n");
#endif
   return OK;
   }  /* agent_select_task */

/********** agent_select_task_spontaneous **********/
/* parameters:	agent_num		index of agent
   called by:   select_tasks()
   actions:     A single agent chooses an action for the next time
		step -- choose an abstract task channel or idle.
		If more than one task channel has unmet demand,
		agent selects one randomly for now.  May try other strategies
		in the future.
		1..Num_tasks = task channels, 0 = not active.

      Differs from agent_select_task! This function incorporates 
      Spontaneous_response_prob and therefore response thresholds
      do NOT need to be met for an agent to select a task. 
      Made this a separate function in case we want to add more 
      differing code for Spontaneous_response_prob specifically.    
*/
int agent_select_task_spontaneous(int agent_num)
   {
   int j;
   int active_tasks[MAX_TASKS + 1];
   int sum;		// count # tasks that need work
   int chosen_task;

#ifdef DEBUG
printf("---in agent_select_task_spontaneous()---\n");
#endif

#ifdef TEMP2
   printf(" *** agent_select_task_spontaneous()\n");
#endif

   sum = 0;

   /* zero out all task channels */
   for (j=0; j<=MAX_TASKS; j++)  active_tasks[j] = 0;

   /* spontaneous choice uses task channels with unmet demand that did
      not pass the response threshold. */
   for (j=1; j<=Num_tasks; j++)
      {
      if (task_stimulus(j) > 0 &&
          agent_task_threshold(agent_num, j) >= task_stimulus(j))
         {
         active_tasks[j] = 1;
         sum++;
         }
      }

   /* each agent chooses an abstract task channel */
   if ( strcmp(Task_selection, "random") == 0 )
      {
      chosen_task = choose_random(agent_num, sum, active_tasks);
      }
   else if ( strcmp(Task_selection, "urgent") == 0 )
      {
      chosen_task = choose_urgent(agent_num, sum, active_tasks);
      }
   else
      {
      printf(
      " Error(agent_select_task): Invalid value for Task_selection: %s\n",
		Task_selection);
      return ERROR;
      }

   // NB; Spontaneous Response Prob; 2020.06.23
   double rand_num = knuth_random();
   if (rand_num < Agent[agent_num].spontaneous_response_prob){
      Agent[agent_num].current_task = chosen_task;
   }else{
      Agent[agent_num].current_task = 0;
   }

   // NB; Spontaneous Response Prob; 2020.06.03
   // Keep track of how many agents are acting due to Spontaneous_response_prob
   if (Agent[agent_num].current_task != 0)
      {
      Agent[agent_num].count_switch_spontaneous += 1;
      }

#ifdef DEBUG
printf("---end agent_select_task_spontaneous()---\n");
#endif
   return OK;
   }  /* agent_select_task_spontaneous */

/********** adjust_agent_thresholds **********/
/* created:	20.04.17.ASW
   parameters:	n		agent index
		tasknum		task that agent chose for current timestep
   called by:	select_tasks()
   actions:	Update per-task thresholds for TD1/TD2/TD3. Pid selects the
                update rule: 0=LFTA, 1=PTA, 2=SETA, 3=SBTA, and 4=CT.
                CT keeps the initialized HM/HT1/HT2 thresholds fixed.
                1..Num_tasks = independent task channels, 0 = not active.
*/
void adjust_agent_thresholds(int n, int tasknum)
   {
   int task;
#ifdef DEBUG
printf("---in adjust_agent_thresholds()---\n");
#endif

   if (Pid == 0 &&
       (Thresh_dynamic == 1 || Thresh_dynamic == 2 ||
        Thresh_dynamic == 3))
      {
      if (tasknum > 0 && tasknum <= Num_tasks)
         {
         for (task=1; task<=Num_tasks; task++)
            {
            if (task == tasknum) Agent[n].thresh_task[task] -= Thresh_decrease;
            else                 Agent[n].thresh_task[task] += Thresh_increase;
            }
         }
      }

   if (Pid == 2)
      {
      double eta = P_gain;
      for (task=1; task<=Num_tasks; task++)
         Agent[n].thresh_task[task] -= eta * Task_feedback_error[task];
      }

   if (Pid == 3)
      {
      double eta_sign = P_gain;
      for (task=1; task<=Num_tasks; task++)
         {
         double feedback = Task_feedback_error[task];
         if (feedback > 0.0)
            Agent[n].thresh_task[task] -= eta_sign;
         else if (feedback < 0.0)
            Agent[n].thresh_task[task] += eta_sign;
         }
      }

   if (Pid == 1)
      {
      double alpha = Agent_pid_gains ? Agent[n].agent_P_gain : P_gain;
      double beta = Agent_pid_gains ? Agent[n].agent_D_gain : D_gain;
      double gamma = Agent_pid_gains ? Agent[n].agent_I_gain : I_gain;

      if (!strncmp(Task_demand_pattern, "legacy_vector", 13))
         {
         for (task=1; task<=Num_tasks; task++)
            {
            double demand_error = Task_feedback_error[task];
            double current_stimulus = fmax(Task_feedback_error[task], 0.0);
            double previous_stimulus =
               fmax(Task_prev_feedback_error[task], 0.0);
            double derivative = current_stimulus - previous_stimulus;
            double integral = Task_error_integral[task];

            Agent[n].thresh_task[task] -= alpha * demand_error;
            Agent[n].thresh_task[task] -= beta * derivative;
            Agent[n].thresh_task[task] -= gamma * integral;
            }
         }
      else
         {
         for (task=1; task<=Num_tasks; task++)
            {
            double demand_error = Task_feedback_error[task];
            double current_stimulus = fmax(Task_feedback_error[task], 0.0);
            double previous_stimulus =
               fmax(Task_prev_feedback_error[task], 0.0);
            double derivative = current_stimulus - previous_stimulus;
            double integral = Task_error_integral[task];

            Agent[n].thresh_task[task] -= alpha * demand_error;
            Agent[n].thresh_task[task] -= beta * derivative;
            Agent[n].thresh_task[task] -= gamma * integral;
            }
         }
   }

   for (task=1; task<=Num_tasks; task++)
      {
      if (!((Pid == 1 || Pid == 2) && Pid_latent_thresholds == 1) &&
          Agent[n].thresh_task[task] > Agent[n].thresh_max_task[task])
         Agent[n].thresh_task[task] = Agent[n].thresh_max_task[task];
      else if (!((Pid == 1 || Pid == 2) && Pid_latent_thresholds == 1) &&
               Agent[n].thresh_task[task] < Agent[n].thresh_min_task[task])
         Agent[n].thresh_task[task] = Agent[n].thresh_min_task[task];
      Agent[n].raw_thresh_task[task] = Agent[n].thresh_task[task] / Range;
      }
   sync_direction_thresholds_from_tasks(n);

//printf("               after   N %lf S %lf E %lf W %lf\n",
//	Agent[n].thresh_north,
//	Agent[n].thresh_south,
//	Agent[n].thresh_east,
//	Agent[n].thresh_west);
 
#ifdef DEBUG
printf("---end adjust_agent_thresholds()---\n");
#endif
   }  /* adjust_agent_thresholds */
int choose_random(int agent_num, int sum, int active_tasks[])
   {
   int randnum;
   int count;
   int i;
   int choose_new_task;
   int current_task;		/* chosen task */

#ifdef DEBUG
printf("---in choose_random()---\n");
#endif

#ifdef TEMP2
   printf(" *** choose_random() sum = %d\n", sum);
#endif

//   Agent[agent_num].current_task = 0;
   current_task = 0;

   randnum = uniform(sum)+1;

   count = 0;
   choose_new_task = 0;
   for (i=1; i<=Num_tasks; i++)
      {
      if (active_tasks[i] == 1)
         {
         count++;
         if (count == randnum)
            {
            choose_new_task = i;
            //Agent[agent_num].current_task = i;
            current_task = i;
            break;
            }
         }
      }

#ifdef TEMP2
   printf("      agent %d, sum %d, choose %d current %d\n",
	agent_num, sum, choose_new_task, current_task);
#endif

#ifdef DEBUG
printf("---end choose_random()---\n");
#endif
   return current_task;
   }  /* choose_random */

/********** choose_urgent **********/
/* parameters:	agent-num	agent that is selecting task
		sum		# tasks below agent's corresponding thresholds
		active_tasks	array showing which tasks are below agent's
				corresponding thresholds
   called by:   agent_select_task()
   actions:     the specified agent selects the most urgent task that falls
		under the agent's corresponding threshold.
*/
int choose_urgent(int agent_num, int sum, int active_tasks[])
   {
   int most_urgent_task;
   double most_urgent_stimulus;
   int current_task;

#ifdef DEBUG
printf("---in choose_urgent()---\n");
#endif

#ifdef TEMP2
   printf(" *** choose_urgent() sum %d\n", sum);
#endif

   //Agent[agent_num].current_task = 0;
   current_task = 0;

   if (sum == 1)
      {
#ifdef TEMP2
printf(" sum = 1\n");
#endif
      // if only one task stimulus falls under this agent's corresponding
      // threshold, that is this agent's most urgent task.
      for (int i=1; i<=Num_tasks; i++)
         {
         if (active_tasks[i] == 1)
            {
            current_task = i;
            break;
            }
         }
      }
   else
      {
#ifdef TEMP2
printf(" sum = %d\n", sum);
#endif
      // if sum is greater than one, find the most urgent task of the tasks
      // whose stimuli fall under the agent's corresponding threshold.
      most_urgent_task = 0;
      most_urgent_stimulus = -1.0;
      for (int i=1; i<=Num_tasks; i++)
         {
         if (active_tasks[i] == 1 && task_stimulus(i) > most_urgent_stimulus)
            {
            most_urgent_task = i;
            most_urgent_stimulus = task_stimulus(i);
#ifdef TEMP2
printf(" task mutask %d mustim %lf\n", most_urgent_task, most_urgent_stimulus);
#endif
            }
         }
      //Agent[agent_num].current_task = most_urgent_task;
      current_task = most_urgent_task;
      }

#ifdef TEMP2
printf("      agent %d, current task %d\n", agent_num, current_task);
#endif

#ifdef DEBUG
printf("---end choose_urgent()---\n");
#endif
   return current_task;
   }  /* choose_urgent */

// HDM; 2019.09.19
/********** update_intensities **********/
/* parameters:
   called by:   move_tracker()
   actions:     apply aging to agent intensities for all agents
                increases by Intensity_aging_step if selected
                decreases by Intensity_aging_step if not selected
*/
void update_intensities()
    {
#ifdef DEBUG
printf("---in update_intensities()---\n");
#endif
    if(Intensity_aging)
       zero_tracker_distribution();
    for(int i = 0; i < Pop_size; i++)
        {
        if(!Agent[i].dead)
           {
           age_one_agent(i);
           update_intensity_totals(i);
           }
        }
    if(Intensity_aging)
       update_tracker_distribution();

#ifdef DEBUG
printf("---end update_intensities()---\n");
#endif
    }   /* update_intensities */

// HDM; 2019.09.19
/********** age_one_agent **********/
/* parameters:  i  agent number
   called by:   update_intensities()
   actions:     apply intensity aging to Agent[i]
*/
void age_one_agent(int i)
    {
    int task;
#ifdef DEBUG
printf("---in age_one_agent()---\n");
#endif
    for (task=1; task<=Num_tasks; task++)
       {
       if (Agent[i].current_task == task)
          Agent[i].intensity_task[task] += Intensity_aging_up;
       else
          Agent[i].intensity_task[task] -= Intensity_aging_down;

       if (Agent[i].intensity_task[task] > Agent[i].int_aging_max_task[task])
          Agent[i].intensity_task[task] = Agent[i].int_aging_max_task[task];
       else if (Agent[i].intensity_task[task] < Agent[i].int_aging_min_task[task])
          Agent[i].intensity_task[task] = Agent[i].int_aging_min_task[task];

       if (Agent[i].intensity_task[task] < Agent[i].int_min_task[task])
          Agent[i].int_min_task[task] = Agent[i].intensity_task[task];
       if (Agent[i].intensity_task[task] > Agent[i].int_max_task[task])
          Agent[i].int_max_task[task] = Agent[i].intensity_task[task];
       }

    sync_direction_intensities_from_tasks(i);

    // update min and max intensities for compatibility direction summaries
    if(Agent[i].intensity_north < Agent[i].int_min_n) Agent[i].int_min_n = Agent[i].intensity_north;
    if(Agent[i].intensity_south < Agent[i].int_min_s) Agent[i].int_min_s = Agent[i].intensity_south;
    if(Agent[i].intensity_east < Agent[i].int_min_e) Agent[i].int_min_e = Agent[i].intensity_east;
    if(Agent[i].intensity_west < Agent[i].int_min_w) Agent[i].int_min_w = Agent[i].intensity_west;
    if(Agent[i].intensity_north > Agent[i].int_max_n) Agent[i].int_max_n = Agent[i].intensity_north;
    if(Agent[i].intensity_south > Agent[i].int_max_s) Agent[i].int_max_s = Agent[i].intensity_south;
    if(Agent[i].intensity_east > Agent[i].int_max_e) Agent[i].int_max_e = Agent[i].intensity_east;
    if(Agent[i].intensity_west > Agent[i].int_max_w) Agent[i].int_max_w = Agent[i].intensity_west;
#ifdef DEBUG
printf("---end age_one_agent()---\n");
#endif

    }   /* age_one_agent */

// HDM; 2020.03.19
// MODIFIED: 2025-08-11 - Changed from targeting most active agents to random selection
/*************** kill_agents *****************/
/* parameters:  int t -- current time step
   called by:   move_tracker()
   actions:     kill agents randomly (instead of targeting most active)
                number killed = Kill_number
                This prevents adverse selection against productive agents
*/
void kill_agents(int t)
   {
   int agents_killed = 0;
   int attempts = 0;
   int max_attempts = Pop_size * 10; // Prevent infinite loops
   
   while(agents_killed < Kill_number && attempts < max_attempts)
      {
      // Select a random agent (clamp so we never index Pop_size; (int)(1.0*Pop_size) can be out-of-bounds)
      int random_index = (int)(knuth_random() * Pop_size);
      if (random_index >= Pop_size) random_index = Pop_size - 1;
      if (random_index < 0) random_index = 0;

      // Check if this agent is alive and not already killed
      if(!Agent[random_index].dead)
         {
         Agent[random_index].dead = 1;
         Agent[random_index].time_killed = t;
         Num_alive--;
         agents_killed++;
         }
      
      attempts++;
      }
   
   // If we couldn't kill enough agents (e.g., most are already dead),
   // just kill the remaining living ones
   if(agents_killed < Kill_number)
      {
      for(int i = 0; i < Pop_size && agents_killed < Kill_number; i++)
         {
         if(!Agent[i].dead)
            {
            Agent[i].dead = 1;
            Agent[i].time_killed = t;
            Num_alive--;
            agents_killed++;
            }
         }
      }
   
#ifdef DEBUG
printf("---end kill_agents()---\n");
#endif
    
   }  /* kill_agents */
   
// HDM; 2019.09.26
/********** update_intensity_totals **********/
/* parameters:  i  agent number
   called by:   update_intensities()
   actions:     update intensity totals for Agent[i]
*/
void update_intensity_totals(int i)
   {
   int task;
#ifdef DEBUG
printf("---in update_intensity_totals()---\n");
#endif
   Agent[i].int_no_act_n += Agent[i].intensity_north;
   Agent[i].int_no_act_e += Agent[i].intensity_east;
   Agent[i].int_no_act_s += Agent[i].intensity_south;
   Agent[i].int_no_act_w += Agent[i].intensity_west;

   for (task=1; task<=Num_tasks; task++)
      Agent[i].int_no_act_task[task] += Agent[i].intensity_task[task];

   if(Agent[i].current_task > 0 && Agent[i].current_task <= Num_tasks)
      {
      Agent[i].int_tot_task[Agent[i].current_task] +=
         Agent[i].intensity_task[Agent[i].current_task];

      }
    
#ifdef DEBUG
printf("---end update_intensity_totals()---\n");
#endif

   } /* update_intensity_totals */


// HDM; 2019.10.10
/********** zero_tracker_distribution **********/
/* parameters:
   called by:   update_intensities()
   actions:     zero tracker intensity counts used for histograms
*/
void zero_tracker_distribution()
   {
#ifdef DEBUG
printf("---in zero_tracker distribution()---\n");
#endif

      for(int i = 0; i < HISTOGRAM_SIZE; i++)
         {
            Tracker.intensity_north_dist[i] = 0;
            Tracker.intensity_south_dist[i] = 0;
            Tracker.intensity_east_dist[i] = 0;
            Tracker.intensity_west_dist[i] = 0;
         }
#ifdef DEBUG
printf("---end zero_tracker_distribution()---\n");
#endif

   } /* zero_tracker_distribution */


// HDM; 2019.10.10
/********** update_tracker_distribution **********/
/* parameters:
   called by:   update_intensities()
   actions:     update tracker intensity counts, used for histograms,
                for this timestep
*/
void update_tracker_distribution()
   {
#ifdef DEBUG
printf("---in update_tracker_distribution()---\n");
#endif

      int index;
      double min = Intensity_aging_min;
      double max = Intensity_aging_max;
    
      for(int i = 0; i < Pop_size; i++)
         {
            if(!Agent[i].dead)
                {
                // TODO make this work for intensity range variation
                if(Intensity_variation == 2)
                   {
                   min = Agent[i].int_aging_min_n;
                   max = Agent[i].int_aging_max_n;
                   }
                index = (int)((Agent[i].intensity_north - min)/(max - min) * HISTOGRAM_SIZE);
                if(index >= HISTOGRAM_SIZE)
                   index = HISTOGRAM_SIZE - 1;
                else if(index < 0)
                   index = 0;
                Tracker.intensity_north_dist[index] += 1;

                if(Intensity_variation == 2)
                   {
                   min = Agent[i].int_aging_min_s;
                   max = Agent[i].int_aging_max_s;
                   }
                index = (int)((Agent[i].intensity_south - min)/(max - min) * HISTOGRAM_SIZE);
                if(index >= HISTOGRAM_SIZE)
                   index = HISTOGRAM_SIZE - 1;
                else if(index < 0)
                   index = 0;
                Tracker.intensity_south_dist[index] += 1;

                if(Intensity_variation == 2)
                   {
                   min = Agent[i].int_aging_min_e;
                   max = Agent[i].int_aging_max_e;
                   }
                index = (int)((Agent[i].intensity_east - min)/(max - min) * HISTOGRAM_SIZE);
                if(index >= HISTOGRAM_SIZE)
                   index = HISTOGRAM_SIZE - 1;
                else if(index < 0)
                   index = 0;
                Tracker.intensity_east_dist[index] += 1;

                if(Intensity_variation == 2)
                   {
                   min = Agent[i].int_aging_min_w;
                   max = Agent[i].int_aging_max_w;
                   }
                index = (int)((Agent[i].intensity_west - min)/(max - min) * HISTOGRAM_SIZE);
                if(index >= HISTOGRAM_SIZE)
                   index = HISTOGRAM_SIZE - 1;
                else if(index < 0)
                   index = 0;
                Tracker.intensity_west_dist[index] += 1;
                }  /* !Agent[i].dead */
         }
#ifdef DEBUG
printf("---end update_tracker_distribution()---\n");
#endif

   } /* update_tracker_distribution */


/********** zero_pushes **********/
/* parameters:
   called by:   move_tracker()
   actions:     zero out the push count so that we can
		count number of agents pushing in each direction.
                190727 -- just north/south for now.
*/
void zero_pushes()
   {
#ifdef DEBUG
printf("---in zero_pushes()---\n");
#endif

   Tracker.push_north = 0;
   Tracker.push_south = 0;
   Tracker.push_east = 0;
   Tracker.push_west = 0;
   for (int i=0; i<=MAX_TASKS; i++)
      {
      Task_actor_count[i] = 0;
      Task_service[i] = 0.0;
      }
   if (Intensity_variation)
      {
      Tracker.intensity_push_north = 0.0;
      Tracker.intensity_push_south = 0.0;
      Tracker.intensity_push_east = 0.0;
      Tracker.intensity_push_west = 0.0;

      Tracker.intensity_all_north = 0.0;
      Tracker.intensity_all_south = 0.0;
      Tracker.intensity_all_east = 0.0;
      Tracker.intensity_all_west = 0.0;
      }

#ifdef DEBUG
printf("---end zero_pushes()---\n");
#endif
   }  /* zero_pushes */

/********** count_task_actors **********/
/* parameters:
   called by:   move_tracker()
   actions:     Count how many agents are allocated to each task channel.
                This is task-space accounting, not physical 2D pushing.
*/
void count_task_actors()
   {
   int i;

#ifdef DEBUG
printf("---in count_task_actors()---\n");
#endif

#ifdef TEMP
   printf(" *** count_task_actors()\n");
#endif

   for (i=0; i<Pop_size; i++)
      {
      if(!Agent[i].dead)
         {
         int current_task = Agent[i].current_task;
         if (current_task > 0 && current_task <= Num_tasks)
            Task_actor_count[current_task]++;
         }
      }

#ifdef DEBUG
printf("---end count_task_actors()---\n");
#endif
   }  /* count_task_actors */

/********** update_tracker_task_vector **********/
/* parameters:
   called by:   move_tracker()
   actions:     Update task-vector tracker summaries after service has been
                applied. The tracker no longer moves in x/y in this version.
*/
void update_tracker_task_vector()
   {
#ifdef DEBUG
printf("---in update_tracker_task_vector()---\n");
#endif

   update_task_vector_stats();

#ifdef DEBUG
printf("---end update_tracker_task_vector()---\n");
#endif
   }  /* update_tracker_task_vector */

/********** adjust_agent_probabilities **********/
/* created:	20.04.17.ASW
   parameters:	n		agent index
		tasknum		task that agent chose for current timestep
   called by:	select_tasks()
   actions:	Update task-specific response probabilities for task channels.
		If the agent's new task is idle (=0), increase all task response
		probabilities by Prob_idle_increase.
                1..Num_tasks = independent task channels, 0 = not active.
*/
void adjust_agent_probabilities(int n, int tasknum)
   {
   int i;
#ifdef DEBUG
printf("---in adjust_agent_probabilities()---\n");
#endif

/*
printf(" Adjust: agent %d, current task %d\n", n, tasknum);
printf("               before  N %lf S %lf E %lf W %lf\n",
	Agent[n].resprob[1],
	Agent[n].resprob[2],
	Agent[n].resprob[3],
	Agent[n].resprob[4]);
*/

   // adjust task response probabilities
   if (tasknum == 0)          // current task is idle
      {
      for (i=1; i<=Num_tasks; i++)  Agent[n].resprob[i] += Prob_idle_increase;
      }
   else
      {
      for (i=1; i<=Num_tasks; i++)
         {
         if (i == tasknum)  Agent[n].resprob[i] += Prob_increase;
         else              Agent[n].resprob[i] -= Prob_decrease;
         }
      }

   // check to make sure they are still within the allowed range
   for (i=1; i<=Num_tasks; i++)
      {
      if (Agent[n].resprob[i] > Agent[n].resprob_max[i])
         Agent[n].resprob[i] = Agent[n].resprob_max[i];
      else if (Agent[n].resprob[i] < Agent[n].resprob_min[i])
         Agent[n].resprob[i] = Agent[n].resprob_min[i];
      }

/*
printf("               after   N %lf S %lf E %lf W %lf\n",
	Agent[n].resprob[1],
	Agent[n].resprob[2],
	Agent[n].resprob[3],
	Agent[n].resprob[4]);
*/

#ifdef DEBUG
printf("---end adjust_agent_probabilities()---\n");
#endif
   }  /* adjust_agent_probabilities */
