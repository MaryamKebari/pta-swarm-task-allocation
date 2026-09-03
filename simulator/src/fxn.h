/* fxn.h
   14.10.15.AW	Created.
*/

/* prototypes */
int init_fxn();
void print_fxn_params(FILE *fp);
int end_fxn();
int run_fxn();
int step_run(int t);
double rand_agent_intensity();                          // HDM; 2019.09.19
int init_agent(int n);
void scale_thresholds(int n);
void init_task_metadata();
int init_agent_task_channels(int n);
void init_agent_task_intensities(int n);
void sync_direction_thresholds_from_tasks(int n);
void sync_direction_intensities_from_tasks(int n);
void update_task_demands(int t);
void update_pre_service_task_error();
void apply_task_service();
void update_task_vector_stats();
void update_run_metrics(int t);
int init_target();
int init_tracker();
int init_raw_thresholds(int n);
int init_dynamic_threshold_range_TD2(int n);
int init_dynamic_threshold_range_TD3(int n);
int init_intensities(int n);
