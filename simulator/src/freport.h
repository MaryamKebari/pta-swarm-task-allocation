/* freport.h
   Report-based feedback: each agent estimates task feedback from its own
   observation and reports received from sampled peers.
*/

#ifndef FREPORT_H
#define FREPORT_H

int init_report_feedback(void);
void end_report_feedback(void);
void update_report_feedback(void);

double report_feedback(int agent_num, int task);
double report_prev_feedback(int agent_num, int task);
double report_error_integral(int agent_num, int task);

#endif
