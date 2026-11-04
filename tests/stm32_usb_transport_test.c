#include "usbd_cdc_if.h"
#include "sony_usb.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
USBD_HandleTypeDef hUsbDeviceFS;
static USBD_CDC_HandleTypeDef cdc;
static uint8_t *inflight;
static uint32_t inflight_size;
static uint32_t irq_enabled=1;
uint32_t NVIC_GetEnableIRQ(int irq) { (void)irq;return irq_enabled; }
void NVIC_DisableIRQ(int irq) { (void)irq;irq_enabled=0; }
void NVIC_EnableIRQ(int irq) { (void)irq;irq_enabled=1; }
uint8_t USBD_CDC_SetTxBuffer(USBD_HandleTypeDef *dev,uint8_t *buf,uint32_t size) {
    (void)dev;inflight=buf;inflight_size=size;return USBD_OK;
}
uint8_t USBD_CDC_TransmitPacket(USBD_HandleTypeDef *dev) {
    (void)dev;assert(!irq_enabled);cdc.TxState=1;return USBD_OK;
}
uint8_t USBD_CDC_SetRxBuffer(USBD_HandleTypeDef *dev,uint8_t *buf) { (void)dev;(void)buf;return USBD_OK; }
uint8_t USBD_CDC_ReceivePacket(USBD_HandleTypeDef *dev) { (void)dev;return USBD_OK; }
static void dtr(unsigned on) {
    USBD_SetupReqTypedef req={0};req.wValue=on;
    USBD_Interface_fops_FS.Control(CDC_SET_CONTROL_LINE_STATE,(uint8_t *)&req,0);
}
int main(void) {
    assert(sony_usb_session()==0);
    assert(CDC_Transmit_FS((uint8_t *)"x",1)==USBD_FAIL); /* NULL class data */
    hUsbDeviceFS.dev_state=USBD_STATE_CONFIGURED;hUsbDeviceFS.pClassData=&cdc;
    USBD_Interface_fops_FS.Init();assert(sony_usb_session()==0);
    dtr(1);uint32_t session=sony_usb_session();assert(session!=0);
    uint8_t caller[]={1,2,3};assert(sony_usb_write(caller,3,session));
    caller[0]=99;assert(inflight_size==3 && inflight[0]==1);
    assert(!sony_usb_write(caller,3,session));assert(inflight[0]==1); /* Busy retains buffer */
    dtr(0);assert(sony_usb_session()==0);assert(!sony_usb_write(caller,3,session));
    dtr(1);assert(sony_usb_session()!=session);
    cdc.TxState=0;
    assert(!sony_usb_write(caller,3,session)); /* Pending previous session */
    assert(sony_usb_write(caller,3,sony_usb_session()));assert(inflight[0]==99);
    USBD_Interface_fops_FS.DeInit();hUsbDeviceFS.pClassData=NULL;
    assert(sony_usb_session()==0);assert(!sony_usb_write(caller,3,session));
    /* Standard serial clients can round-trip their baud/format request. */
    uint8_t coding[7]={0x80,0x25,0,0,0,0,8}, roundtrip[7]={0};
    USBD_Interface_fops_FS.Control(CDC_SET_LINE_CODING,coding,7);
    USBD_Interface_fops_FS.Control(CDC_GET_LINE_CODING,roundtrip,7);
    assert(memcmp(coding,roundtrip,7)==0);
    assert(irq_enabled);
    puts("CDC transport: absent class, DTR, async buffer ownership, reopen and line coding passed");
}
