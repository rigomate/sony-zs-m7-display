#include "sony_usb.h"
#include <stdio.h>
#include <string.h>
#define USB_QUEUE_DEPTH 32U
#define USB_PACKET_SIZE 512U
static sony_spi_frame_t queue[USB_QUEUE_DEPTH];
static uint32_t read_index, write_index, session, last_stats;
static uint8_t packet[USB_PACKET_SIZE];
static uint16_t packet_size;
static uint8_t banner;
uint32_t sony_usb_dropped;

void sony_usb_frame(const sony_spi_frame_t *frame) {
    if (!session) return; /* Offline is not a logging session. */
    if (write_index - read_index == USB_QUEUE_DEPTH) { ++sony_usb_dropped; return; }
    queue[write_index++ % USB_QUEUE_DEPTH] = *frame;
}
void sony_usb_tick(uint32_t now_ms) {
    uint32_t current = sony_usb_session();
    if (current != session) {
        /* Never replay stale records after disconnect/reconnect. */
        session = current; read_index = write_index = 0; packet_size = 0;
        last_stats = now_ms - 1000U; banner = 1;
    }
    if (!session) return;
    if (!packet_size) {
        if (banner) {
            static const char header[] = "#ZS-M7 SPI TRACE v1\n"
                "#F,bus,sequence,ms,flags_hex,data_hex\n"
                "#flags:01=unsynced,02=length,04=overrun,08=dma;00=valid\n"
                "#S,ms,spi1_valid,spi1_rejected,spi1_dropped,spi2_valid,spi2_rejected,spi2_dropped,usb_dropped\n";
            memcpy(packet, header, sizeof(header) - 1);
            packet_size = sizeof(header) - 1; banner = 0;
        }
        if ((uint32_t)(now_ms - last_stats) >= 1000 && sizeof(packet) - packet_size >= 120) {
            int n = snprintf((char *)packet + packet_size, sizeof(packet) - packet_size,
                "S,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n", (unsigned long)now_ms,
                (unsigned long)sony_spi_stats[0].accepted, (unsigned long)sony_spi_stats[0].rejected,
                (unsigned long)sony_spi_stats[0].dropped, (unsigned long)sony_spi_stats[1].accepted,
                (unsigned long)sony_spi_stats[1].rejected, (unsigned long)sony_spi_stats[1].dropped,
                (unsigned long)sony_usb_dropped);
            packet_size += (uint16_t)n; last_stats = now_ms;
        }
        /* Leave room for a maximum-size frame and a periodic status line.
           Batching avoids a USB transaction per frame. */
        for (unsigned budget = 0; budget < USB_QUEUE_DEPTH && read_index != write_index; ++budget) {
            if (sizeof(packet) - packet_size < 120) break;
            const sony_spi_frame_t *f = &queue[read_index++ % USB_QUEUE_DEPTH];
            int n = snprintf((char *)packet + packet_size, sizeof(packet) - packet_size,
                "F,%u,%lu,%lu,%02X,", (unsigned)f->bus + 1,
                (unsigned long)f->sequence, (unsigned long)f->at_ms, (unsigned)f->flags);
            packet_size += (uint16_t)n;
            static const char hex[] = "0123456789ABCDEF";
            for (unsigned i = 0; i < f->size && i < SONY_SPI_CAPACITY; ++i) {
                packet[packet_size++] = hex[f->bytes[i] >> 4];
                packet[packet_size++] = hex[f->bytes[i] & 15];
            }
            packet[packet_size++] = '\n';
        }

    }
    /* Exactly one attempt per tick, no retries/waits. The transport owns a
       separate persistent buffer until the USB IN transfer completes. */
    if (packet_size && sony_usb_write(packet, packet_size, session)) packet_size = 0;
}
