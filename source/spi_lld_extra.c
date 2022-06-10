#include "spi_lld_extra.h"

/**
 * @brief   send one frame using a polled wait.
 * @details This synchronous function send one frame using a polled
 *          synchronization method. This function is useful when in 3 wires mode
 *          where half duplex data use a shared MOSI line.
 *          Send and Read have to be done sequentially 
 * 
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] frame     the data frame to send over the SPI bus
 * @return              void
 */
void spi_lld_polled_send(SPIDriver *spip, uint16_t frame) {

  /*
   * Data register must be accessed with the appropriate data size.
   * Byte size access (uint8_t *) for transactions that are <= 8-bit.
   * Halfword size access (uint16_t) for transactions that are <= 8-bit.
   */
  chDbgAssert(spip->state == SPI_READY, "spi invalid state");
  chDbgAssert(spip->spi->CR1 & SPI_CR1_BIDIMODE, "spi bidimode must be activated");
  if ((spip->config->cr2 & SPI_CR2_DS) <= (SPI_CR2_DS_2 |
                                           SPI_CR2_DS_1 |
                                           SPI_CR2_DS_0)) {
    volatile uint8_t *dr8p = (volatile uint8_t *)&spip->spi->DR;
    *dr8p = (uint8_t)frame;
    while ((spip->spi->SR & SPI_SR_TXE) == 0U) {
      /* Waiting frame sending.*/
    }
  }
  else {
    volatile uint16_t *dr16p = (volatile uint16_t *)&spip->spi->DR;
    *dr16p = (uint16_t)frame;
    while ((spip->spi->SR & SPI_SR_TXE) == 0U) {
      /* Waiting frame transfer.*/
    }
  }

}

/**
 * @brief   receive one frame using a polled wait.
 * @details This synchronous function receive one frame using a polled
 *          synchronization method. This function is useful when in 3 wires mode
 *          where half duplex data use a shared MOSI line.
 *          Send and Read have to be done sequentially 
 * @note    as soon as SPI_CR1_BIDIOE is 0, sck is clocking and data is 
 *           deserialised from MOSI line
 * @param[in] spip      pointer to the @p SPIDriver object
 * @return              The received data frame from the SPI bus.
 */

uint16_t spi_lld_polled_receive(SPIDriver *spip) {
  uint16_t frame;
  /*
   * Data register must be accessed with the appropriate data size.
   * Byte size access (uint8_t *) for transactions that are <= 8-bit.
   * Halfword size access (uint16_t) for transactions that are <= 8-bit.
   */
  chDbgAssert(spip->state == SPI_READY, "spi invalid state");
  chDbgAssert(spip->spi->CR1 & SPI_CR1_BIDIMODE, "spi bidimode must be activated");
  /* uint32_t cr1 = spip->spi->CR1; */
  /* spip->spi->CR1 = 0; */
  /* spip->spi->CR1 = cr1 & ~SPI_CR1_BIDIOE; */
  spip->spi->CR1 &= ~SPI_CR1_BIDIOE; /* this activate receive mode, sclk clocking */
  if ((spip->config->cr2 & SPI_CR2_DS) <= (SPI_CR2_DS_2 |
                                           SPI_CR2_DS_1 |
                                           SPI_CR2_DS_0)) {
    volatile uint8_t *dr8p = (volatile uint8_t *)&spip->spi->DR;
    while ((spip->spi->SR & SPI_SR_RXNE) == 0U) {
      /* Waiting frame transfer.*/
    }
    frame = (uint16_t)*dr8p;
  }
  else {
    volatile uint16_t *dr16p = (volatile uint16_t *)&spip->spi->DR;
    while ((spip->spi->SR & SPI_SR_RXNE) == 0U) {
      /* Waiting frame transfer.*/
    }
    frame = (uint16_t)*dr16p;
  }

  spip->spi->CR1 |= SPI_CR1_BIDIOE; /* this stop sclk clocking */
  return frame;
}

void spi_lld_wait_until_not_busy(SPIDriver *spip) {
  chDbgAssert(spip->state == SPI_READY, "spi invalid state");
  chDbgAssert(spip->spi->CR1 & SPI_CR1_BIDIMODE, "spi bidimode must be activated");
  /* Waiting for current frame completion */
  while ((spip->spi->SR & SPI_SR_BSY) != 0U) {
    /* Still busy.*/
  }
}
