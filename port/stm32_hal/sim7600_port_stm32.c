/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Ankit Virdi
 *
 * Part of sim7600-aws-mqtt — https://github.com/ankitvirdi4/sim7600-aws-mqtt
 */

#include "sim7600_port_stm32.h"

void sim7600_port_stm32_init(sim7600_port_t *p,
                             const sim7600_port_stm32_config_t *cfg)
{
    p->huart           = cfg->huart;
    p->dma_rx_buf      = cfg->dma_rx_buf;
    p->dma_rx_buf_size = cfg->dma_rx_buf_size;
    p->dma_rx_pos      = 0;
    p->pwrkey          = cfg->pwrkey;
    p->reset           = cfg->reset;
    p->status_in       = cfg->status_in;
    p->at_ctx          = NULL;
}

void sim7600_port_stm32_attach_at(sim7600_port_t *p, sim7600_at_ctx_t *at)
{
    p->at_ctx = at;
}

HAL_StatusTypeDef sim7600_port_stm32_start_rx(sim7600_port_t *p)
{
    p->dma_rx_pos = 0;
    return HAL_UARTEx_ReceiveToIdle_DMA(p->huart,
                                        p->dma_rx_buf,
                                        (uint16_t)p->dma_rx_buf_size);
}

void sim7600_port_stm32_rx_event_cb(sim7600_port_t *p, uint16_t Size)
{
    if (p->at_ctx == NULL) {
        return;
    }

    /* Size is the cumulative write position in the circular DMA buffer.
     * Bytes from p->dma_rx_pos up to Size are new, with possible wrap.
     */
    if (Size >= p->dma_rx_pos) {
        sim7600_at_feed(p->at_ctx,
                        &p->dma_rx_buf[p->dma_rx_pos],
                        (size_t)(Size - p->dma_rx_pos));
    } else {
        sim7600_at_feed(p->at_ctx,
                        &p->dma_rx_buf[p->dma_rx_pos],
                        p->dma_rx_buf_size - p->dma_rx_pos);
        sim7600_at_feed(p->at_ctx,
                        &p->dma_rx_buf[0],
                        (size_t)Size);
    }
    p->dma_rx_pos = (size_t)Size;
}

/* sim7600_port.h interface implementations. */

int sim7600_port_uart_init(sim7600_port_t *p, uint32_t baud)
{
    /* The user's MX_USARTx_UART_Init() already configured the UART at the
     * desired baud (typically 115200). Nothing to do here.
     */
    (void)p; (void)baud;
    return 0;
}

int sim7600_port_uart_write(sim7600_port_t *p, const uint8_t *buf, size_t len)
{
    /* Blocking transmit for M2 simplicity. v0.2 candidate: HAL_UART_Transmit_DMA
     * with TxCplt callback for non blocking behaviour.
     */
    if (HAL_UART_Transmit(p->huart, (uint8_t *)buf,
                          (uint16_t)len, 1000) != HAL_OK) {
        return -1;
    }
    return (int)len;
}

int sim7600_port_uart_read(sim7600_port_t *p, uint8_t *buf, size_t maxlen)
{
    /* Polling read is not used when DMA RX is active. */
    (void)p; (void)buf; (void)maxlen;
    return 0;
}

static const sim7600_stm32_pin_t *resolve_pin(sim7600_port_t *p,
                                              sim7600_pin_t pin)
{
    switch (pin) {
        case SIM7600_PIN_PWRKEY: return &p->pwrkey;
        case SIM7600_PIN_RESET:  return &p->reset;
        case SIM7600_PIN_STATUS: return &p->status_in;
    }
    return NULL;
}

void sim7600_port_gpio_set(sim7600_port_t *p, sim7600_pin_t pin, bool high)
{
    const sim7600_stm32_pin_t *gp = resolve_pin(p, pin);
    if (gp == NULL) return;
    HAL_GPIO_WritePin(gp->port, gp->pin,
                      high ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

bool sim7600_port_gpio_get(sim7600_port_t *p, sim7600_pin_t pin)
{
    const sim7600_stm32_pin_t *gp = resolve_pin(p, pin);
    if (gp == NULL) return false;
    return HAL_GPIO_ReadPin(gp->port, gp->pin) == GPIO_PIN_SET;
}

uint32_t sim7600_port_millis(void)
{
    return HAL_GetTick();
}

void sim7600_port_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}
