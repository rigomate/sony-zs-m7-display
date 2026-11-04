#ifndef SONY_USB_H
#define SONY_USB_H
#include "sony_spi.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Main-loop only: bounded, independent of the SPI ISR queues. */
extern uint32_t sony_usb_dropped;
void sony_usb_tick(uint32_t now_ms);
void sony_usb_frame(const sony_spi_frame_t *frame);
/* Transport supplied by usbd_cdc_if.c. Session=0 means disconnected/DTR low.
   write() copies the data before returning 1; busy/disconnected returns 0. */
uint32_t sony_usb_session(void);
int sony_usb_write(const uint8_t *bytes, uint16_t size, uint32_t session);
#ifdef __cplusplus
}
#endif
#endif
