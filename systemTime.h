/*file: systemTime.h
 *
*/

#ifndef SYSTEMTIME_H_
#define SYSTEMTIME_H_

#define USER_SCHEDULER_TIMER NRF_TIMER1
#define CALLBACK_QUANTITY        5

typedef volatile struct timerCallback *timerCallbacT;
typedef void (*timerCallbackFunT)(void);


void initUserTimer(void);
uint32_t getTime(void);
timerCallbacT timerGetCallback(timerCallbackFunT timerCallbackFun);
void timerRun(timerCallbacT inTimerCallbac, int32_t waitTime);
void userProcessingTimerCallbackFun(void);

#endif
