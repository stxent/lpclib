/*
 * usb/usb_control.h
 * Copyright (C) 2015 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef USB_USB_CONTROL_H_
#define USB_USB_CONTROL_H_
/*----------------------------------------------------------------------------*/
#include <pin.h>
#include <containers/queue.h>
#include <usb/usb.h>
#include <usb/usb_requests.h>
/*----------------------------------------------------------------------------*/
extern const struct EntityClass * const UsbControl;
/*----------------------------------------------------------------------------*/
struct UsbControlConfig
{
  /** Mandatory: parent device. */
  struct UsbDevice *parent;
};
/*----------------------------------------------------------------------------*/
struct UsbControl
{
  struct Entity parent;

  struct UsbDevice *base;
  struct UsbDriver *driver;

  struct UsbEndpoint *ep0in;
  struct UsbEndpoint *ep0out;
  struct Queue requestPool;
  struct UsbRequest *requests;

  uint8_t currentConfiguration;

  struct
  {
    struct UsbSetupPacket packet;
    uint8_t *buffer;
    uint16_t left;
  } state;
};
/*----------------------------------------------------------------------------*/
enum result usbControlSetDriver(struct UsbControl *, void *);
void usbControlUpdateStatus(struct UsbControl *, uint8_t);
/*----------------------------------------------------------------------------*/
#endif /* USB_USB_CONTROL_H_ */