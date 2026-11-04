#ifndef SONY_SPI_H
#define SONY_SPI_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define SONY_SPI_CAPACITY 32U
#define SONY_SPI_QUEUE_DEPTH 8U
typedef struct {
    uint32_t at_ms, sequence;
    uint8_t bus, size, flags;
    uint8_t bytes[SONY_SPI_CAPACITY];
} sony_spi_frame_t;
enum {
    SONY_SPI_UNSYNCED = 1U, SONY_SPI_BAD_LENGTH = 2U,
    SONY_SPI_OVERRUN = 4U, SONY_SPI_DMA_ERROR = 8U
};
typedef struct { uint32_t accepted, rejected, dropped; } sony_spi_stats_t;
/* Stats are useful in the debugger. No HAL/LVGL work is done by the consumer ISR. */
extern volatile sony_spi_stats_t sony_spi_stats[2];
void sony_spi_start(void);
void sony_spi_select_end(unsigned bus);
int sony_spi_pop(sony_spi_frame_t *frame);
void sony_live_init(void);
void sony_live_tick(void);
#ifdef __cplusplus
}
#endif
#endif
