/*file: orderProcessing.h
 *
*/

#ifndef ORDERPROCESSING
#define ORDERPROCESSING

#include "stdint.h"
#include "stdbool.h"

#define ORDER_ITEM_QUANTITY  5
#define HEAP_ORDER_QUANTITY  2

typedef struct order *orderT;

orderT  orderMalloc     (void);
bool    orderFree       (orderT orderIn);
bool    orderSetFirst   (orderT orderIn, uint8_t inItem);
bool    orderGetPos     (const orderT orderIn, uint8_t deviceID, uint8_t *pos);
uint8_t orderGetQuantity(const orderT orderIn);
void    orderClear      (orderT orderIn);

#endif
