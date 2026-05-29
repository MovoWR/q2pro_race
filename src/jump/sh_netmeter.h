#ifndef SH_NETMETER_H
#define SH_NETMETER_H

void SH_NetMeter_Init(void);
void SH_NetMeter_Sample(unsigned ping);
void SH_NetMeter_PredictionError(int len);
void SH_NetMeter_Draw(void);
void SH_NetMeter_Clear(void);
unsigned SH_NetMeter_GetAvgPing(void);

#endif
