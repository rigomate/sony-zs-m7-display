/* Host shims for the real ST CDC interface headers; no USB hardware emulated. */
#ifndef TEST_USBD_CONF_H
#define TEST_USBD_CONF_H
#include <stdint.h>
#include <stddef.h>
#define __IO volatile
#define __PACKED __attribute__((packed))
#define __STATIC_INLINE static inline
#define UNUSED(x) ((void)(x))
#define OTG_FS_IRQn 67
uint32_t NVIC_GetEnableIRQ(int irq);
void NVIC_DisableIRQ(int irq);
void NVIC_EnableIRQ(int irq);
#endif
