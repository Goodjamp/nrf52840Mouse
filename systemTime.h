/*file: systemTime.h
 *
*/

#ifndef SYSTEMTIME_H_
#define SYSTEMTIME_H_

#define USER_SCHEDULER_TIMER NRF_TIMER1

void initUserTimer(void);
uint32_t getTime(void);

#endif
