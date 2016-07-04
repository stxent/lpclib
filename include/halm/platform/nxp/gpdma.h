/*
 * halm/platform/nxp/gpdma.h
 * Copyright (C) 2012 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef HALM_PLATFORM_NXP_GPDMA_H_
#define HALM_PLATFORM_NXP_GPDMA_H_
/*----------------------------------------------------------------------------*/
#include <stdbool.h>
#include <halm/platform/nxp/gpdma_base.h>
/*----------------------------------------------------------------------------*/
extern const struct DmaClass * const GpDma;
/*----------------------------------------------------------------------------*/
struct GpDmaConfig
{
  /** Mandatory: channel number. */
  uint8_t channel;

  /** Mandatory: destination configuration. */
  struct
  {
    bool increment;
  } destination;

  /** Mandatory: source configuration. */
  struct
  {
    bool increment;
  } source;

  /** Mandatory: number of transfers that make up a burst transfer request. */
  enum dmaBurst burst;
  /** Mandatory: request connection to the peripheral or memory. */
  enum gpDmaEvent event;
  /** Mandatory: transfer type. */
  enum gpDmaType type;
  /** Mandatory: source and destination transfer widths. */
  enum dmaWidth width;
};
/*----------------------------------------------------------------------------*/
struct GpDmaRuntimeConfig
{
  /** Mandatory: destination configuration. */
  struct
  {
    bool increment;
  } destination;

  /** Mandatory: source configuration. */
  struct
  {
    bool increment;
  } source;
};
/*----------------------------------------------------------------------------*/
struct GpDma
{
  struct GpDmaBase base;

  void (*callback)(void *);
  void *callbackArgument;

  /* Status of the last transfer */
  bool error;
};
/*----------------------------------------------------------------------------*/
#endif /* HALM_PLATFORM_NXP_GPDMA_H_ */