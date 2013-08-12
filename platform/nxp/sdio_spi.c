/*
 * sdio_spi.c
 * Copyright (C) 2013 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include "macro.h"
#include "platform/nxp/sdio_spi.h"
#include "platform/nxp/system.h"
/*----------------------------------------------------------------------------*/
#define BLOCK_POW       9
#define TOKEN_DATA_MASK 0x1F
/*----------------------------------------------------------------------------*/
#define OCR_CCS         BIT(30) /* Card Capacity Status */
#define OCR_HCS         BIT(30) /* Host Capacity Support */
/*----------------------------------------------------------------------------*/
enum sdioCommand
{
  CMD_GO_IDLE_STATE     = 0,
  CMD_SEND_IF_COND      = 8,
  CMD_STOP_TRANSMISSION = 12,
  CMD_READ              = 17,
  CMD_READ_MULTIPLE     = 18,
  CMD_WRITE             = 24,
  CMD_WRITE_MULTIPLE    = 25,
  CMD_APP_CMD           = 55,
  CMD_READ_OCR          = 58,
  ACMD_SD_SEND_OP_COND  = 41
};
/*----------------------------------------------------------------------------*/
enum sdioResponse
{
  SDIO_INIT             = 0x01,
  SDIO_ERASE_RESET      = 0x02,
  SDIO_ILLEGAL_COMMAND  = 0x04,
  SDIO_CRC_ERROR        = 0x08,
  SDIO_ERASE_ERROR      = 0x10,
  SDIO_BAD_ADDRESS      = 0x20,
  SDIO_BAD_ARGUMENT     = 0x40
};
/*----------------------------------------------------------------------------*/
/* Direct operations with token variables are correct only on LE machines */
enum sdioToken
{
  TOKEN_DATA_ACCEPTED     = 0x05,
  TOKEN_DATA_CRC_ERROR    = 0x0B,
  TOKEN_DATA_WRITE_ERROR  = 0x0D,
  TOKEN_START             = 0xFE,
  TOKEN_START_MULTIPLE    = 0xFC,
  TOKEN_STOP              = 0xFD
};
/*----------------------------------------------------------------------------*/
struct LongResponse
{
  uint32_t payload;
  uint8_t value;
};
/*----------------------------------------------------------------------------*/
static inline enum result acquireBus(struct SdioSpi *);
static inline uint32_t calcPosition(struct SdioSpi *);
static inline void releaseBus(struct SdioSpi *);
/*----------------------------------------------------------------------------*/
static void sendCommand(struct SdioSpi *, enum sdioCommand, uint32_t);
static enum result waitForData(struct SdioSpi *, uint8_t *);
static uint8_t getShortResponse(struct SdioSpi *);
static enum result getLongResponse(struct SdioSpi *, struct LongResponse *);
static enum result resetCard(struct SdioSpi *);
static enum result readBlock(struct SdioSpi *, uint8_t *);
static enum result waitBusyState(struct SdioSpi *);
static enum result writeBlock(struct SdioSpi *, const uint8_t *, uint8_t);
/*----------------------------------------------------------------------------*/
static enum result sdioInit(void *, const void *);
static void sdioDeinit(void *);
static enum result sdioCallback(void *, void (*)(void *), void *);
static enum result sdioGet(void *, enum ifOption, void *);
static enum result sdioSet(void *, enum ifOption, const void *);
static uint32_t sdioRead(void *, uint8_t *, uint32_t);
static uint32_t sdioWrite(void *, const uint8_t *, uint32_t);
/*----------------------------------------------------------------------------*/
static const struct InterfaceClass sdioTable = {
    .size = sizeof(struct SdioSpi),
    .init = sdioInit,
    .deinit = sdioDeinit,

    .callback = sdioCallback,
    .get = sdioGet,
    .set = sdioSet,
    .read = sdioRead,
    .write = sdioWrite
};
/*----------------------------------------------------------------------------*/
const struct InterfaceClass *SdioSpi = &sdioTable;
/*----------------------------------------------------------------------------*/
static inline enum result acquireBus(struct SdioSpi *device)
{
  const uint32_t lockValue = 1;
  enum result res;

  if ((res = ifSet(device->interface, IF_LOCK, &lockValue)) != E_OK)
    return res;
  gpioWrite(&device->csPin, 0);
  return E_OK;
}
/*----------------------------------------------------------------------------*/
static inline uint32_t calcPosition(struct SdioSpi *device)
{
  return device->capacity == CARD_SD ? (uint32_t)device->position
      : (uint32_t)(device->position >> BLOCK_POW);
}
/*----------------------------------------------------------------------------*/
static inline void releaseBus(struct SdioSpi *device)
{
  const uint32_t lockValue = 0;

  gpioWrite(&device->csPin, 1);
  ifSet(device->interface, IF_LOCK, &lockValue);
}
/*----------------------------------------------------------------------------*/
static void sendCommand(struct SdioSpi *device, enum sdioCommand command,
    uint32_t parameter)
{
  uint8_t buffer[8];

  /* Fill buffer */
  buffer[0] = 0xFF;
  buffer[1] = 0x40 | (uint8_t)command;
  buffer[2] = (uint8_t)(parameter >> 24);
  buffer[3] = (uint8_t)(parameter >> 16);
  buffer[4] = (uint8_t)(parameter >> 8);
  buffer[5] = (uint8_t)(parameter);
  /* Checksum should be valid only for first CMD0 and CMD8 commands */
  switch (command)
  {
    case CMD_GO_IDLE_STATE:
      buffer[6] = 0x94;
      break;
    case CMD_SEND_IF_COND:
      buffer[6] = 0x86;
      break;
    default:
      buffer[6] = 0x00;
      break;
  }
  buffer[6] |= 0x01; /* Add end bit */
  buffer[7] = 0xFF;

  /* TODO Check for interface error? */
  ifWrite(device->interface, buffer, sizeof(buffer));
}
/*----------------------------------------------------------------------------*/
static enum result waitForData(struct SdioSpi *device, uint8_t *value)
{
  uint8_t counter;

  /* Response will come after 1..8 queries */
  for (counter = 8; counter; --counter)
  {
    if (!ifRead(device->interface, value, 1))
      return E_INTERFACE; /* Interface error */
    if (*value != 0xFF)
      break;
  }
  return !counter ? E_BUSY : E_OK;
}
/*----------------------------------------------------------------------------*/
static uint8_t getShortResponse(struct SdioSpi *device)
{
  uint8_t value;

  return waitForData(device, &value) == E_OK ? value : 0xFF;
}
/*----------------------------------------------------------------------------*/
/* Response to READ_OCR and SEND_IF_COND commands */
static enum result getLongResponse(struct SdioSpi *device,
    struct LongResponse *response)
{
  uint8_t buffer[4];
  enum result res;

  res = waitForData(device, buffer);
  if (res == E_OK)
  {
    response->value = *buffer;
    if (ifRead(device->interface, buffer, sizeof(buffer)) != sizeof(buffer))
      res = E_INTERFACE;
    /* Response comes in big-endian format */
    response->payload = (uint32_t)buffer[3] | ((uint32_t)buffer[2] << 8)
        | ((uint32_t)buffer[1] << 16) | ((uint32_t)buffer[0] << 24);
  }

  return res;
}
/*----------------------------------------------------------------------------*/
static enum result resetCard(struct SdioSpi *device)
{
  const uint32_t lowSpeed = 200000;
  const uint32_t lockValue = 1;
  const uint8_t dummyByte = 0xFF; /* Emulate high level during setup */
  uint32_t srcSpeed;
  uint16_t counter;
  uint8_t response;
  uint8_t version = 1; /* SD Card version */
  struct LongResponse longResp;
  enum result res;

  device->capacity = CARD_SD;
  device->position = 0;

  /* Acquire bus but leave chip select high */
  if ((res = ifSet(device->interface, IF_LOCK, &lockValue)) != E_OK)
    return res;

  ifGet(device->interface, IF_RATE, &srcSpeed);
  ifSet(device->interface, IF_RATE, &lowSpeed);

  /* Send 80 clocks with MOSI line high */
  for (counter = 10; counter; --counter)
    ifWrite(device->interface, &dummyByte, 1);

  /* Enable chip select */
  gpioWrite(&device->csPin, 0);

  /* Send reset command */
  sendCommand(device, CMD_GO_IDLE_STATE, 0);
  if (!(getShortResponse(device) & SDIO_INIT))
  {
    ifSet(device->interface, IF_RATE, &srcSpeed);
    releaseBus(device);
    return E_DEVICE;
  }

  /* Detect card version */
  sendCommand(device, CMD_SEND_IF_COND, 0x000001AA); /* TODO Add define */
  if ((res = getLongResponse(device, &longResp)) != E_OK)
  {
    ifSet(device->interface, IF_RATE, &srcSpeed);
    releaseBus(device);
    return res;
  }
  if (!(longResp.value & SDIO_ILLEGAL_COMMAND))
  {
    if (longResp.payload != 0x000001AA)
    {
      ifSet(device->interface, IF_RATE, &srcSpeed);
      releaseBus(device);
      return E_ERROR; /* Pattern mismatched */
    }
    version = 2; /* SD Card version 2.0 or later */
  }

  /* Wait till card becomes ready */
  for (counter = 100; counter; --counter)
  {
    sendCommand(device, CMD_APP_CMD, 0);
    if ((response = getShortResponse(device)) == 0xFF)
      break;
    sendCommand(device, ACMD_SD_SEND_OP_COND, OCR_HCS);
    if ((response = getShortResponse(device)) == 0xFF || !response)
      break;
    msleep(10); /* Retry after 10 ms */
  }
  if (response)
  {
    ifSet(device->interface, IF_RATE, &srcSpeed);
    releaseBus(device);
    return response == 0xFF ? E_DEVICE : E_ERROR;
  }

  /* Detect card capacity */
  if (version >= 2)
  {
    sendCommand(device, CMD_READ_OCR, 0);
    if ((res = getLongResponse(device, &longResp)) != E_OK)
    {
      ifSet(device->interface, IF_RATE, &srcSpeed);
      releaseBus(device);
      return res;
    }
    if (longResp.payload & OCR_CCS)
      device->capacity = CARD_SDHC;
  }

  /* Increase speed after initialization */
  ifSet(device->interface, IF_RATE, &srcSpeed);
  releaseBus(device);
  return E_OK;
}
/*----------------------------------------------------------------------------*/
static enum result readBlock(struct SdioSpi *device, uint8_t *buffer)
{
  uint32_t read = 0;
  uint16_t counter = 10000; /* Up to 100 ms */
  uint16_t crc;
  uint8_t response;

  /* Wait for data token */
  while (--counter)
  {
    if ((response = getShortResponse(device)) == 0xFF)
    {
      usleep(10);
      continue;
    }
    if (response != TOKEN_START)
      return E_DEVICE;
    break;
  }
  if (!counter)
    return E_DEVICE;

  /* Receive block data */
  read += ifRead(device->interface, buffer, 1 << BLOCK_POW);
  /* Receive block checksum */
  read += ifRead(device->interface, (uint8_t *)&crc, sizeof(crc));

  if (read != (1 << BLOCK_POW) + sizeof(crc))
    return E_INTERFACE;

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static enum result waitBusyState(struct SdioSpi *device)
{
  uint32_t read;
  uint16_t counter = 50000; /* Up to 500 ms */
  uint8_t state;

  /* Wait until busy condition will be removed from RX line */
  while (--counter)
  {
    read = ifRead(device->interface, &state, 1);
    if (read != 1)
      return E_INTERFACE;
    if (state == 0xFF)
      break;
    usleep(10);
  }

  /* TODO Add interface recovery */

  return counter ? E_OK : E_BUSY;
}
/*----------------------------------------------------------------------------*/
static enum result writeBlock(struct SdioSpi *device, const uint8_t *buffer,
    enum sdioToken token)
{
  const uint16_t crc = 0xFFFF;
  uint32_t written = 0;
  uint8_t response;

  /* Send start block token */
  written += ifWrite(device->interface, (uint8_t *)&token, 1);
  /* Send block data */
  written += ifWrite(device->interface, buffer, 1 << BLOCK_POW);
  /* Send block checksum */
  written += ifWrite(device->interface, (uint8_t *)&crc, sizeof(crc));

  if (written != 1 + (1 << BLOCK_POW) + sizeof(crc))
    return E_INTERFACE;

  /* Receive data response */
  if ((response = getShortResponse(device)) == 0xFF)
    return E_DEVICE;
  /* Check data token value */
  if ((response & TOKEN_DATA_MASK) != TOKEN_DATA_ACCEPTED)
    return E_ERROR; /* CRC error or write data error */

  /* Wait for the flash programming to be finished */
  waitBusyState(device);

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static enum result sdioInit(void *object, const void *configPtr)
{
  const struct SdioSpiConfig * const config = configPtr;
  struct SdioSpi *device = object;
  enum result res;

  /* Check device configuration data */
  assert(config);
  assert(config->interface);

  device->csPin = gpioInit(config->cs, GPIO_OUTPUT);
  if (!gpioGetKey(&device->csPin))
    return E_ERROR;
  gpioWrite(&device->csPin, 1);

  device->interface = config->interface;
  device->callback = 0;
  device->blocking = true;

  if ((res = resetCard(device)) != E_OK)
    return res;

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void sdioDeinit(void *object)
{
  struct SdioSpi *device = object;

  gpioDeinit(&device->csPin);
}
/*----------------------------------------------------------------------------*/
static enum result sdioCallback(void *object, void (*callback)(void *),
    void *argument)
{
  struct SdioSpi *device = object;

  device->callback = callback;
  device->callbackArgument = argument;
  return E_OK;
}
/*----------------------------------------------------------------------------*/
static enum result sdioGet(void *object, enum ifOption option, void *data)
{
  struct SdioSpi *device = object;

  switch (option)
  {
    case IF_ADDRESS:
      *(uint64_t *)data = device->position;
      return E_OK;
    case IF_NONBLOCKING:
      *(uint32_t *)data = !device->blocking;
      return E_OK;
    default:
      return E_ERROR;
  }
}
/*----------------------------------------------------------------------------*/
static enum result sdioSet(void *object, enum ifOption option,
    const void *data)
{
  struct SdioSpi *device = object;

  switch (option)
  {
    case IF_ADDRESS:
      /* TODO Add boundary check */
      device->position = *(uint64_t *)data;
      return E_OK;
    case IF_NONBLOCKING:
      device->blocking = *(uint32_t *)data ? false : true;
      return E_OK;
    default:
      return E_ERROR;
  }
}
/*----------------------------------------------------------------------------*/
static uint32_t sdioRead(void *object, uint8_t *buffer, uint32_t length)
{
  struct SdioSpi *device = object;
  uint16_t counter, blockCount;
  enum sdioCommand command;

  blockCount = length >> BLOCK_POW;
  if ((length & ((1 << BLOCK_POW) - 1)) || !blockCount)
    return 0; /* Unsupported block length */

  command = blockCount > 1 ? CMD_READ_MULTIPLE : CMD_READ;

  if (acquireBus(device) != E_OK)
    return 0;

  sendCommand(device, command, calcPosition(device));
  if (getShortResponse(device))
  {
    releaseBus(device);
    return 0;
  }

  for (counter = 0; counter < blockCount; ++counter)
  {
    if (readBlock(device, buffer + (counter << BLOCK_POW)) != E_OK)
      break;
  }

  sendCommand(device, CMD_STOP_TRANSMISSION, 0);
  getShortResponse(device);
  waitBusyState(device);

  releaseBus(device);
  return counter << BLOCK_POW;
}
/*----------------------------------------------------------------------------*/
static uint32_t sdioWrite(void *object, const uint8_t *buffer, uint32_t length)
{
  struct SdioSpi *device = object;
  uint16_t counter, blockCount;
  enum sdioCommand command;
  enum sdioToken token;

  blockCount = length >> BLOCK_POW;
  if ((length & ((1 << BLOCK_POW) - 1)) || !blockCount)
    return 0; /* Unsupported block length */

  if (blockCount > 1)
  {
    command = CMD_WRITE_MULTIPLE;
    token = TOKEN_START_MULTIPLE;
  }
  else
  {
    command = CMD_WRITE;
    token = TOKEN_START;
  }

  if (acquireBus(device) != E_OK)
    return 0;

  sendCommand(device, command, calcPosition(device));
  if (getShortResponse(device))
  {
    releaseBus(device);
    return 0;
  }

  for (counter = 0; counter < blockCount; ++counter)
  {
    if (writeBlock(device, buffer + (counter << BLOCK_POW), token) != E_OK)
      break;
  }

  if (blockCount > 1)
  {
    /* Send stop token in multiple block write operation */
    token = TOKEN_STOP;
    ifWrite(device->interface, (uint8_t *)&token, 1); //TODO Check result?
    waitBusyState(device);
  }

  releaseBus(device);
  return counter << BLOCK_POW;
}