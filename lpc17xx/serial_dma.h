/*
 * serial.h
 * Copyright (C) 2012 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef SERIAL_DMA_H_
#define SERIAL_DMA_H_
/*----------------------------------------------------------------------------*/
#include "uart.h"
#include "dma.h"
#include "mutex.h"
/*----------------------------------------------------------------------------*/
extern const struct UartClass *SerialDma;
/*----------------------------------------------------------------------------*/
struct SerialDmaConfig
{
  struct UartConfig uart;
  int8_t rxChannel, txChannel; /* DMA channels */
  uint32_t rate;
//  uint16_t rxLength, txLength; /* Receive buffer length */
};
/*----------------------------------------------------------------------------*/
struct SerialDma
{
  struct Uart parent;

  struct Dma *rxDma, *txDma;
  Mutex dmaLock; /* Access to DMA channels */
};
/*----------------------------------------------------------------------------*/
#endif /* SERIAL_DMA_H_ */