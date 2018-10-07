/*file: orderProcessing.c
 *
*/

#include "stdint.h"
#include "string.h"
#include "stdbool.h"
#include "orderProcessing.h"

#define ORDER_ITEM_FREE  0xFF

#define ORDER_HEAP_ITEM_FREE    0xFF
#define ORDER_HEAP_ITEM_BUSY    0x00


typedef struct order
{
    uint8_t order[ORDER_ITEM_QUANTITY];
} deviceOrderS;


struct
{
    deviceOrderS heap[HEAP_ORDER_QUANTITY];
    uint8_t      heapItemState[HEAP_ORDER_QUANTITY];
}heapDescriptor =
{
    .heapItemState = {[0 ... HEAP_ORDER_QUANTITY - 1] = ORDER_HEAP_ITEM_FREE}
};


static inline bool getIDPos(orderT orderIn, uint8_t *rezPos, uint8_t inItem)
{
    for(uint8_t cnt = 0; cnt < ORDER_ITEM_QUANTITY; cnt++ )
    {
        if( inItem != orderIn->order[cnt])
        {
            continue;
        }
        *rezPos = cnt;
        return true;
    }
    return false;
}


orderT orderMalloc(void)
{
    for(uint8_t cnt = 0; cnt < HEAP_ORDER_QUANTITY; cnt++)
    {
        if(heapDescriptor.heapItemState[cnt] != ORDER_HEAP_ITEM_FREE)
        {
            continue;
        }
        heapDescriptor.heapItemState[cnt] = ORDER_HEAP_ITEM_BUSY;
        memset(heapDescriptor.heap[cnt].order, ORDER_ITEM_FREE, sizeof(heapDescriptor.heap[cnt].order));
        return &heapDescriptor.heap[cnt];
    }
    return NULL;
}

bool orderFree(orderT orderIn)
{
    for(uint8_t cnt = 0; cnt < HEAP_ORDER_QUANTITY; cnt++)
    {
        if(orderIn != &heapDescriptor.heap[cnt])
        {
            continue;
        }
        heapDescriptor.heapItemState[cnt] = ORDER_HEAP_ITEM_FREE;
        return true;
    }
    return false;
}


bool orderSetFirst (orderT orderIn, uint8_t inItem)
{
    uint8_t currentPos;
    if( orderIn == NULL)
    {
        return false;
    }
    if( !getIDPos(orderIn, &currentPos, inItem))
    {
        currentPos = ORDER_ITEM_QUANTITY - 1;
    }
    if(currentPos == 0)
    {
        return true;
    }
    for(; currentPos > 0; currentPos--)
    {
        orderIn->order[currentPos] = orderIn->order[currentPos - 1];
    }
    orderIn->order[0] = inItem;
    return true;
}


bool orderGetPos(const orderT orderIn, uint8_t deviceID, uint8_t *pos)
{
    for(uint8_t cnt = 0; cnt < ORDER_ITEM_QUANTITY; cnt++)
    {
        if(orderIn->order[cnt] != deviceID)
        {
            continue;
        }
        *pos = cnt;
        return true;
    }
    return false;
}


uint8_t orderGetQuantity(const orderT orderIn)
{
    uint8_t cnt = 0;
    for(; cnt < ORDER_ITEM_QUANTITY; cnt++)
    {
        if(orderIn->order[cnt] == ORDER_ITEM_FREE)
        {
            break;
        }
    }
    return cnt;
}


void orderClear(orderT orderIn)
{
    memset(orderIn->order, ORDER_ITEM_FREE, sizeof(orderIn->order));
}
