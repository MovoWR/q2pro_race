#ifndef SH_NETBAR_H
#define SH_NETBAR_H

void SH_NetBar_Init(void);
void SH_NetBar_Sample(unsigned ping);
void SH_NetBar_PredictionError(int len);
void SH_NetBar_Draw(void);

#endif
