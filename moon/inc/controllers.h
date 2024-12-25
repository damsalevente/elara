#ifndef CONTROLLERS_H
#define CONTROLLERS_H
#include <pid.h>

extern pi_struct controller_w;
extern pi_struct controller_id;
extern pi_struct controller_iq;

void control_runner(float *w_ref, float *wr, float *id, float *iq, float *ud, float *uq);
void control_fast(float *, float *, float *, float *, float *, float *);
void control_slow(float *, float *, float *);
float pos_control(float target_pos, float current_pos);
#endif
