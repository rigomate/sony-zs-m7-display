#include "sony_spi.h"
#include "stm32f4xx_hal.h"
#include <string.h>

extern SPI_HandleTypeDef hspi1, hspi2;
volatile sony_spi_stats_t sony_spi_stats[2];
typedef struct {
    SPI_TypeDef *spi;
    DMA_Stream_TypeDef *dma;
    uint32_t channel, cr1;
    uint8_t bytes[SONY_SPI_CAPACITY];
    sony_spi_frame_t queue[SONY_SPI_QUEUE_DEPTH];
    volatile uint32_t write, read;
    uint32_t sequence;
    uint8_t synchronized;
} receiver_t;
static receiver_t receivers[2];
static uint8_t started;

static void disable_dma(receiver_t *r) {
    r->dma->CR &= ~DMA_SxCR_EN;
    while (r->dma->CR & DMA_SxCR_EN) { }
}
static void arm(unsigned bus) {
    receiver_t *r = &receivers[bus];
    /* A peripheral reset also discards a partial trailing byte / OVR state. */
    if (bus == 0) { __HAL_RCC_SPI1_FORCE_RESET(); __HAL_RCC_SPI1_RELEASE_RESET(); }
    else { __HAL_RCC_SPI2_FORCE_RESET(); __HAL_RCC_SPI2_RELEASE_RESET(); }
    r->spi->CR1 = r->cr1;
    if (bus == 0) DMA2->LIFCR = DMA_LIFCR_CFEIF0 | DMA_LIFCR_CDMEIF0 | DMA_LIFCR_CTEIF0 | DMA_LIFCR_CHTIF0 | DMA_LIFCR_CTCIF0;
    else DMA1->LIFCR = DMA_LIFCR_CFEIF3 | DMA_LIFCR_CDMEIF3 | DMA_LIFCR_CTEIF3 | DMA_LIFCR_CHTIF3 | DMA_LIFCR_CTCIF3;
    r->dma->PAR = (uint32_t)&r->spi->DR;
    r->dma->M0AR = (uint32_t)r->bytes;
    r->dma->NDTR = SONY_SPI_CAPACITY;
    r->dma->FCR = 0; /* byte transfers, direct mode */
    r->dma->CR = r->channel | DMA_SxCR_MINC | DMA_SxCR_PL_1;
    r->dma->CR |= DMA_SxCR_EN;
    r->spi->CR2 = SPI_CR2_RXDMAEN;
    r->spi->CR1 |= SPI_CR1_SPE;
}
void sony_spi_start(void) {
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();
    receivers[0].spi = SPI1;
    receivers[0].dma = DMA2_Stream0; /* SPI1_RX, channel 3 */
    receivers[0].channel = DMA_CHANNEL_3;
    receivers[0].cr1 = hspi1.Instance->CR1 & ~SPI_CR1_SPE;
    receivers[1].spi = SPI2;
    receivers[1].dma = DMA1_Stream3; /* SPI2_RX, channel 0; LCD uses streams 0/5 */
    receivers[1].channel = DMA_CHANNEL_0;
    /* Software-selected slave: PB12 is unused. CS1 rising edges delimit
       both streams; SPI2 samples its own clock between those boundaries. */
    receivers[1].cr1 = (hspi2.Instance->CR1 | SPI_CR1_SSM) &
                       ~(SPI_CR1_SPE | SPI_CR1_BIDIOE | SPI_CR1_SSI);
    for (unsigned i = 0; i < 2; ++i) {
        receiver_t *r = &receivers[i];
        r->spi->CR1 &= ~SPI_CR1_SPE;
        disable_dma(r);
        /* Drop the first transaction even if startup happens inside a frame. */
        r->synchronized = 0;
        arm(i);
    }
    /* PA4 remains SPI1 hardware NSS and also frames SPI2 through EXTI4.
       SPI2 uses software NSS; ignore the damaged/unconnected PB12 line. */
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    SYSCFG->EXTICR[1] &= ~SYSCFG_EXTICR2_EXTI4;
    EXTI->IMR &= ~GPIO_PIN_12;
    EXTI->FTSR &= ~(GPIO_PIN_4 | GPIO_PIN_12);
    EXTI->RTSR = (EXTI->RTSR & ~GPIO_PIN_12) | GPIO_PIN_4;
    EXTI->PR = GPIO_PIN_4 | GPIO_PIN_12;
    EXTI->IMR |= GPIO_PIN_4;
    started = 1;
    /* Above LCD DMA/USB IRQs, so display traffic cannot delay frame rearming. */
    HAL_NVIC_SetPriority(OTG_FS_IRQn, 2, 0);
    HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI4_IRQn);
}
void sony_spi_select_end(unsigned bus) {
    if (!started || bus > 1) return;
    receiver_t *r = &receivers[bus];
    r->spi->CR2 &= ~SPI_CR2_RXDMAEN;
    disable_dma(r);
    uint32_t count = SONY_SPI_CAPACITY - r->dma->NDTR;
    uint32_t sr = r->spi->SR;
    uint32_t dma_errors = bus == 0 ? DMA2->LISR & (DMA_LISR_TEIF0 | DMA_LISR_DMEIF0 | DMA_LISR_FEIF0)
                                          : DMA1->LISR & (DMA_LISR_TEIF3 | DMA_LISR_DMEIF3 | DMA_LISR_FEIF3);
    /* A final RXNE byte may not yet have been transferred by DMA at NSS rise. */
    if ((sr & SPI_SR_RXNE) && count < SONY_SPI_CAPACITY) r->bytes[count++] = *(volatile uint8_t *)&r->spi->DR;
    const uint32_t expected = bus == 0 ? 16 : 15;
    const uint32_t now = HAL_GetTick();
    uint8_t flags = (!r->synchronized ? SONY_SPI_UNSYNCED : 0) |
        (count != expected ? SONY_SPI_BAD_LENGTH : 0) |
        ((sr & SPI_SR_OVR) ? SONY_SPI_OVERRUN : 0) |
        (dma_errors ? SONY_SPI_DMA_ERROR : 0);
    uint32_t sequence = ++r->sequence;
    if (flags) ++sony_spi_stats[bus].rejected;
    else ++sony_spi_stats[bus].accepted;
    uint32_t write = r->write;
    if (write - r->read < SONY_SPI_QUEUE_DEPTH) {
        sony_spi_frame_t *f = &r->queue[write % SONY_SPI_QUEUE_DEPTH];
        f->at_ms = now; f->sequence = sequence;
        f->bus = bus; f->size = count; f->flags = flags;
        memcpy(f->bytes, r->bytes, count);
        __DMB();
        r->write = write + 1;
    } else ++sony_spi_stats[bus].dropped;
    r->synchronized = 1;
    arm(bus);
}
int sony_spi_pop(sony_spi_frame_t *frame) {
    /* The critical section only copies one small fixed-size record. */
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    receiver_t *r = NULL;
    if (receivers[0].read != receivers[0].write) r = &receivers[0];
    if (receivers[1].read != receivers[1].write &&
        (!r || (int32_t)(receivers[1].queue[receivers[1].read % SONY_SPI_QUEUE_DEPTH].at_ms -
                         r->queue[r->read % SONY_SPI_QUEUE_DEPTH].at_ms) < 0)) r = &receivers[1];
    if (r) { *frame = r->queue[r->read % SONY_SPI_QUEUE_DEPTH]; ++r->read; }
    __set_PRIMASK(primask);
    return r != NULL;
}
