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

orderT   orderMalloc     (void);
bool     orderFree       (orderT orderIn);
bool     orderSetFirst   (orderT orderIn, uint8_t inItem);
bool     orderGetPos     (const orderT orderIn, uint8_t deviceID,  uint8_t *pos);
uint16_t orderGetItem    (const orderT orderIn, uint8_t pos);
uint8_t  orderGetQuantity(const orderT orderIn);
void     orderClean      (orderT orderIn);
bool     orderReadFlash  (orderT orderIn, uint32_t  flashAddress);
void     orderWriteFlash (orderT orderIn, uint32_t flashAddress);


/*********USER IMPLEMENTED FUNCTION****************/
void flashMemWriteBytes(uint32_t flashAddress, uint8_t buffer[], uint32_t bufferSize);
void flashMemReadBytes(uint32_t flashAddress, uint8_t buffer[], uint32_t bufferSize);

#endif
