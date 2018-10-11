/*file: systemTime.c
 *
*/
#include "stdint.h"
#include "systemTime.h"

#include "nrf.h"
#include "nrf_timer.h"

#define TIMER_INT_MS   1


static volatile uint32_t cntMs;
static bool              isNewTick = false;

static volatile struct timerCallback
{
     timerCallbackFunT fun;
     uint32_t          timeCallback;
     bool              runCallback;
     bool              waiteCallback;
} timerCallbackHeap[CALLBACK_QUANTITY];

bool isCallbackFree[CALLBACK_QUANTITY] = {[0 ... CALLBACK_QUANTITY - 1] = true};


const uint32_t timFrqList[]=
{
    [NRF_TIMER_FREQ_16MHz]    = 16000000, ///< Timer frequency 16 MHz.
    [NRF_TIMER_FREQ_8MHz]     = 8000000,  ///< Timer frequency 8 MHz.
    [NRF_TIMER_FREQ_4MHz]     = 4000000,  ///< Timer frequency 4 MHz.
    [NRF_TIMER_FREQ_2MHz]     = 2000000,  ///< Timer frequency 2 MHz.
    [NRF_TIMER_FREQ_1MHz]     = 1000000,  ///< Timer frequency 1 MHz.
    [NRF_TIMER_FREQ_500kHz]   = 500000,   ///< Timer frequency 500 kHz.
    [NRF_TIMER_FREQ_250kHz]   = 250000,   ///< Timer frequency 250 kHz.
    [NRF_TIMER_FREQ_125kHz]   = 125000,   ///< Timer frequency 125 kHz.
    [NRF_TIMER_FREQ_62500Hz]  = 62500,    ///< Timer frequency 62500 Hz.
    [NRF_TIMER_FREQ_31250Hz]  = 31250     ///< Timer frequency 31250 Hz.
};


void initUserTimer(void)
{
    /*-------------Config timer for interrupt in CC---------*/
    uint32_t valCC = (timFrqList[NRF_TIMER_FREQ_500kHz] / 1000) * TIMER_INT_MS;
    nrf_timer_mode_set(USER_SCHEDULER_TIMER, NRF_TIMER_MODE_TIMER);
    nrf_timer_bit_width_set(USER_SCHEDULER_TIMER, NRF_TIMER_BIT_WIDTH_16);
    nrf_timer_frequency_set(USER_SCHEDULER_TIMER, NRF_TIMER_FREQ_500kHz);
    nrf_timer_cc_write(USER_SCHEDULER_TIMER, NRF_TIMER_CC_CHANNEL0, valCC);
    nrf_timer_int_enable(USER_SCHEDULER_TIMER, (0b1 << 16));

    NVIC_SetPriority(TIMER1_IRQn, 3);
    NVIC_EnableIRQ(TIMER1_IRQn);

    nrf_timer_task_trigger(USER_SCHEDULER_TIMER, NRF_TIMER_TASK_START);
}


void TIMER1_IRQHandler(void)
{
    cntMs++;
    nrf_timer_task_trigger(USER_SCHEDULER_TIMER, NRF_TIMER_TASK_CLEAR);
    nrf_timer_event_clear(USER_SCHEDULER_TIMER, NRF_TIMER_EVENT_COMPARE0);
    for(uint16_t cnt = 0; cnt < CALLBACK_QUANTITY; cnt++)
    {
        if(timerCallbackHeap[cnt].waiteCallback && (timerCallbackHeap[cnt].timeCallback <= cntMs))
        {
            if(timerCallbackHeap[cnt].timeCallback >= cntMs)
            timerCallbackHeap[cnt].waiteCallback = false;
            timerCallbackHeap[cnt].runCallback   = true;
        }

    }
    isNewTick = true;
}


uint32_t getTime(void)
{
    return cntMs;
}


timerCallbacT timerGetCallback(timerCallbackFunT timerCallbackFun)
{
    for(uint16_t cnt = 0; cnt < CALLBACK_QUANTITY; cnt ++)
    {
        if(!isCallbackFree[cnt])
        {
            continue;
        }
        isCallbackFree[cnt]                  = false;
        timerCallbackHeap[cnt].fun           = timerCallbackFun;
        timerCallbackHeap[cnt].runCallback   = false;
        timerCallbackHeap[cnt].waiteCallback = false;
        return &timerCallbackHeap[cnt];
    }
    return NULL;
}


void timerRun(timerCallbacT inTimerCallbac, int32_t waitTime)
{
    inTimerCallbac->timeCallback  = cntMs + waitTime;
    inTimerCallbac->runCallback   = false;
    inTimerCallbac->waiteCallback = true;
}


void userProcessingTimerCallbackFun(void)
{
    if(!isNewTick)
    {
        return;
    }
    isNewTick = false;
    for(uint16_t cnt = 0; cnt < CALLBACK_QUANTITY; cnt++)
    {
        if(!timerCallbackHeap[cnt].runCallback)
        {
            continue;
        }
        timerCallbackHeap[cnt].runCallback   = false;
        timerCallbackHeap[cnt].fun();
    }
}

