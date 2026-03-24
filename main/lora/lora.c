#include "lora/lora.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "sx1276_regs_lora.h"
#include <freertos/task.h>
#include <stdint.h>

#define MOSI 27
#define SCLK 5
#define CS 18
#define RST 23
#define MISO 19

#define CHECK(expr)              \
    do {                         \
        esp_err_t _err = (expr); \
        if (_err != ESP_OK) {    \
            return _err;         \
        }                        \
    } while (0)

void lora_reset(void) {
    gpio_set_level(RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(RST, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
}

static spi_device_handle_t lora_spi_init(void) {
    spi_device_handle_t lora_spi;
    spi_bus_config_t buscfg = {
        .mosi_io_num = MOSI,
        .miso_io_num = MISO,
        .sclk_io_num = SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1000000,
        .mode = 0,
        .spics_io_num = CS,
        .queue_size = 1,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &lora_spi));
    return lora_spi;
}

esp_err_t lora_read_reg(spi_device_handle_t handle, uint8_t reg, uint8_t *out) {
    if (out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    spi_transaction_t t = {0};
    uint8_t tx[2] = {reg & 0x7F, 0};
    uint8_t rx[2] = {0};

    t.length = 16;
    t.tx_buffer = tx;
    t.rx_buffer = rx;

    esp_err_t err = spi_device_transmit(handle, &t);
    if (err != ESP_OK) {
        return err;
    }

    *out = rx[1];
    return ESP_OK;
}

esp_err_t lora_write_reg(spi_device_handle_t handle, uint8_t reg,
                         uint8_t value) {
    spi_transaction_t t = {0};
    uint8_t tx[2] = {reg | 0x80, value};

    t.length = 16;
    t.tx_buffer = tx;

    return spi_device_transmit(handle, &t);
}

spi_device_handle_t lora_init(void) {
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << RST),
    };

    if (gpio_config(&io_conf) != ESP_OK) {
        return NULL;
    }

    spi_device_handle_t handle = lora_spi_init();
    lora_reset();

    if (lora_write_reg(handle, REG_LR_OPMODE, 0x80) != ESP_OK) {
        return NULL;
    }
    if (lora_write_reg(handle, REG_LR_FRFMSB, 0xE4) != ESP_OK) {
        return NULL;
    }
    if (lora_write_reg(handle, REG_LR_FRFMID, 0xC0) != ESP_OK) {
        return NULL;
    }
    if (lora_write_reg(handle, REG_LR_FRFLSB, 0x00) != ESP_OK) {
        return NULL;
    }
    if (lora_write_reg(handle, REG_LR_OPMODE, 0x81) != ESP_OK) {
        return NULL;
    }

    uint8_t version = 0;
    if (lora_read_reg(handle, REG_LR_VERSION, &version) != ESP_OK) {
        return NULL;
    }
    if (version != 0x12 && version != 0x11) {
        return NULL;
    }

    return handle;
}

esp_err_t lora_get_freq(spi_device_handle_t handle, float *freq_mhz_out) {
    if (freq_mhz_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t msb = 0;
    uint8_t mid = 0;
    uint8_t lsb = 0;

    CHECK(lora_read_reg(handle, REG_LR_FRFMSB, &msb));
    CHECK(lora_read_reg(handle, REG_LR_FRFMID, &mid));
    CHECK(lora_read_reg(handle, REG_LR_FRFLSB, &lsb));

    uint32_t frf = ((uint32_t)msb << 16) | ((uint32_t)mid << 8) | lsb;
    *freq_mhz_out = (float)frf * 61.03515625f / 1e6f;
    return ESP_OK;
}

esp_err_t lora_set_frequency(spi_device_handle_t handle, uint32_t freq_hz) {
    uint32_t frf = ((uint64_t)freq_hz << 19) / 32000000;

    CHECK(lora_write_reg(handle, REG_LR_FRFMSB, (frf >> 16) & 0xFF));
    CHECK(lora_write_reg(handle, REG_LR_FRFMID, (frf >> 8) & 0xFF));
    CHECK(lora_write_reg(handle, REG_LR_FRFLSB, frf & 0xFF));
    return ESP_OK;
}

esp_err_t lora_set_tx_power(spi_device_handle_t handle, uint8_t dbm) {
    if (dbm < 2) {
        dbm = 2;
    }
    if (dbm > 17) {
        dbm = 17;
    }

    uint8_t pa_config = 0;
    pa_config |= RFLR_PACONFIG_PASELECT_PABOOST;
    pa_config |= (7 << 4);
    pa_config |= (dbm - 2);

    return lora_write_reg(handle, REG_LR_PACONFIG, pa_config);
}

esp_err_t lora_get_tx_power(spi_device_handle_t handle, uint8_t *dbm_out) {
    if (dbm_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t reg = 0;
    CHECK(lora_read_reg(handle, REG_LR_PACONFIG, &reg));

    *dbm_out = (reg & 0x0F) + 2;
    return ESP_OK;
}

esp_err_t lora_set_opmode(spi_device_handle_t handle, uint8_t mode) {
    uint8_t opmode = 0;
    CHECK(lora_read_reg(handle, REG_LR_OPMODE, &opmode));

    opmode = (opmode & RFLR_OPMODE_MASK) | mode;
    CHECK(lora_write_reg(handle, REG_LR_OPMODE, opmode));
    return ESP_OK;
}

esp_err_t lora_set_mode_standby(spi_device_handle_t handle) {
    return lora_set_opmode(handle, RFLR_OPMODE_STANDBY);
}

esp_err_t lora_set_mode_tx(spi_device_handle_t handle) {
    return lora_set_opmode(handle, RFLR_OPMODE_TRANSMITTER);
}

esp_err_t lora_set_fifo_tx_base_addr(spi_device_handle_t handle, uint8_t addr) {
    CHECK(lora_write_reg(handle, REG_LR_FIFOTXBASEADDR, addr));
    CHECK(lora_write_reg(handle, REG_LR_FIFOADDRPTR, addr));
    return ESP_OK;
}

esp_err_t lora_write_fifo(spi_device_handle_t handle, const uint8_t *buf,
                          uint8_t len) {
    CHECK(lora_write_reg(handle, REG_LR_PAYLOADLENGTH, len));

    for (uint8_t i = 0; i < len; i++) {
        CHECK(lora_write_reg(handle, REG_LR_FIFO, buf[i]));
    }

    return ESP_OK;
}

esp_err_t lora_get_irq_flags(spi_device_handle_t handle, uint8_t *flags_out) {
    return lora_read_reg(handle, REG_LR_IRQFLAGS, flags_out);
}

esp_err_t lora_clear_irq_flags(spi_device_handle_t handle, uint8_t flags) {
    return lora_write_reg(handle, REG_LR_IRQFLAGS, flags);
}

esp_err_t lora_set_dio0_mapping(spi_device_handle_t handle, bool tx_mode) {
    uint8_t current_mapping = 0;
    CHECK(lora_read_reg(handle, REG_LR_DIOMAPPING1, &current_mapping));

    current_mapping &= RFLR_DIOMAPPING1_DIO0_MASK;
    if (tx_mode) {
        current_mapping |= RFLR_DIOMAPPING1_DIO0_01;
    } else {
        current_mapping |= RFLR_DIOMAPPING1_DIO0_00;
    }

    CHECK(lora_write_reg(handle, REG_LR_DIOMAPPING1, current_mapping));
    return ESP_OK;
}

esp_err_t lora_send_packet(spi_device_handle_t handle, const lora_packet_t *pkt) {
    if (pkt == NULL || pkt->payload_len > LORA_MAX_PAYLOAD) {
        return ESP_ERR_INVALID_ARG;
    }

    CHECK(lora_set_mode_standby(handle));
    CHECK(lora_write_reg(handle, REG_LR_FIFOTXBASEADDR, 0x00));
    CHECK(lora_write_reg(handle, REG_LR_FIFOADDRPTR, 0x00));
    CHECK(lora_write_reg(handle, REG_LR_PAYLOADLENGTH, pkt->payload_len + 3));

    uint8_t tx_len = (uint8_t)(3 + pkt->payload_len);
    const uint8_t *raw = (const uint8_t *)pkt;
    for (uint8_t i = 0; i < tx_len; i++) {
        CHECK(lora_write_reg(handle, REG_LR_FIFO, raw[i]));
    }

    CHECK(lora_set_mode_tx(handle));

    uint32_t timeout_ms = 2000;
    uint32_t start = xTaskGetTickCount() * portTICK_PERIOD_MS;
    while (1) {
        uint8_t flags = 0;
        esp_err_t err = lora_get_irq_flags(handle, &flags);
        if (err != ESP_OK) {
            lora_set_mode_standby(handle);
            return err;
        }

        if ((flags & RFLR_IRQFLAGS_TXDONE) != 0) {
            break;
        }

        if ((xTaskGetTickCount() * portTICK_PERIOD_MS - start) > timeout_ms) {
            lora_set_mode_standby(handle);
            return ESP_ERR_TIMEOUT;
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }

    esp_err_t err = lora_clear_irq_flags(handle, RFLR_IRQFLAGS_TXDONE);
    if (err != ESP_OK) {
        lora_set_mode_standby(handle);
        return err;
    }

    return lora_set_mode_standby(handle);
}

esp_err_t lora_set_mode_rx_single(spi_device_handle_t handle) {
    CHECK(lora_set_mode_standby(handle));
    CHECK(lora_write_reg(handle, REG_LR_FIFORXBASEADDR, 0x00));
    CHECK(lora_write_reg(handle, REG_LR_FIFOADDRPTR, 0x00));
    CHECK(lora_set_opmode(handle, RFLR_OPMODE_RECEIVER_SINGLE));
    return ESP_OK;
}

esp_err_t lora_set_mode_rx_continuous(spi_device_handle_t handle) {
    CHECK(lora_set_mode_standby(handle));
    CHECK(lora_write_reg(handle, REG_LR_FIFORXBASEADDR, 0x00));
    CHECK(lora_write_reg(handle, REG_LR_FIFOADDRPTR, 0x00));
    CHECK(lora_set_opmode(handle, RFLR_OPMODE_RECEIVER));
    return ESP_OK;
}

esp_err_t lora_is_packet_received(spi_device_handle_t handle,
                                  bool *received_out) {
    if (received_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t flags = 0;
    CHECK(lora_get_irq_flags(handle, &flags));

    *received_out = (flags & RFLR_IRQFLAGS_RXDONE) != 0;
    return ESP_OK;
}

esp_err_t lora_is_crc_error(spi_device_handle_t handle, bool *crc_error_out) {
    if (crc_error_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t flags = 0;
    CHECK(lora_get_irq_flags(handle, &flags));

    *crc_error_out = (flags & RFLR_IRQFLAGS_PAYLOADCRCERROR) != 0;
    return ESP_OK;
}

esp_err_t lora_get_rx_payload_length(spi_device_handle_t handle, uint8_t *len_out) {
    return lora_read_reg(handle, REG_LR_RXNBBYTES, len_out);
}

esp_err_t lora_read_fifo_payload(spi_device_handle_t handle, uint8_t *buf,
                                 uint8_t len) {
    if (buf == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    for (uint8_t i = 0; i < len; i++) {
        CHECK(lora_read_reg(handle, REG_LR_FIFO, &buf[i]));
    }

    return ESP_OK;
}

esp_err_t lora_clear_rx_flags(spi_device_handle_t handle) {
    return lora_clear_irq_flags(handle,
                                RFLR_IRQFLAGS_RXDONE | RFLR_IRQFLAGS_PAYLOADCRCERROR);
}

/*
 * returns -3: bus/register error
 * returns -2: crc error
 * returns -1: timeout
 */
int16_t lora_receive_packet(spi_device_handle_t handle, uint8_t *buf,
                            uint16_t buf_size, uint32_t timeout_ms) {
    if (buf == NULL) {
        return -3;
    }

    if (buf_size > 256) {
        buf_size = 256;
    }

    if (lora_set_mode_rx_single(handle) != ESP_OK) {
        return -3;
    }

    uint32_t start = xTaskGetTickCount() * portTICK_PERIOD_MS;
    while (1) {
        bool received = false;
        if (lora_is_packet_received(handle, &received) != ESP_OK) {
            return -3;
        }
        if (received) {
            break;
        }

        if (timeout_ms > 0) {
            uint32_t elapsed = (xTaskGetTickCount() * portTICK_PERIOD_MS) - start;
            if (elapsed > timeout_ms) {
                lora_clear_irq_flags(handle, RFLR_IRQFLAGS_RXTIMEOUT);
                return -1;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }

    bool crc_error = false;
    if (lora_is_crc_error(handle, &crc_error) != ESP_OK) {
        return -3;
    }
    if (crc_error) {
        lora_clear_rx_flags(handle);
        return -2;
    }

    uint8_t rx_len = 0;
    if (lora_get_rx_payload_length(handle, &rx_len) != ESP_OK) {
        return -3;
    }

    uint16_t bytes_to_read = (rx_len > buf_size) ? buf_size : rx_len;
    if (bytes_to_read > 256) {
        bytes_to_read = 256;
    }

    if (lora_read_fifo_payload(handle, buf, (uint8_t)bytes_to_read) != ESP_OK) {
        return -3;
    }
    if (lora_clear_rx_flags(handle) != ESP_OK) {
        return -3;
    }
    if (lora_set_mode_standby(handle) != ESP_OK) {
        return -3;
    }

    return (int16_t)bytes_to_read;
}

esp_err_t lora_set_bandwidth(spi_device_handle_t handle, lora_bandwidth_t bw) {
    uint8_t reg = 0;
    CHECK(lora_read_reg(handle, REG_LR_MODEMCONFIG1, &reg));

    reg = (reg & RFLR_MODEMCONFIG1_BW_MASK) | bw;
    CHECK(lora_write_reg(handle, REG_LR_MODEMCONFIG1, reg));
    return ESP_OK;
}

esp_err_t lora_get_bandwidth(spi_device_handle_t handle,
                             lora_bandwidth_t *bw_out) {
    if (bw_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t val = 0;
    CHECK(lora_read_reg(handle, REG_LR_MODEMCONFIG1, &val));

    val &= 0xF0;
    *bw_out = (lora_bandwidth_t)val;
    return ESP_OK;
}

esp_err_t lora_set_coding_rate(spi_device_handle_t handle, lora_coding_rate_t cr) {
    uint8_t reg = 0;
    CHECK(lora_read_reg(handle, REG_LR_MODEMCONFIG1, &reg));

    reg = (reg & RFLR_MODEMCONFIG1_CODINGRATE_MASK) | cr;
    CHECK(lora_write_reg(handle, REG_LR_MODEMCONFIG1, reg));
    return ESP_OK;
}

esp_err_t lora_get_coding_rate(spi_device_handle_t handle,
                               lora_coding_rate_t *cr_out) {
    if (cr_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t val = 0;
    CHECK(lora_read_reg(handle, REG_LR_MODEMCONFIG1, &val));

    val &= 0x0E;
    *cr_out = (lora_coding_rate_t)val;
    return ESP_OK;
}

esp_err_t lora_set_spreading_factor(spi_device_handle_t handle,
                                    lora_spreading_factor_t sf) {
    uint8_t reg = 0;
    CHECK(lora_read_reg(handle, REG_LR_MODEMCONFIG2, &reg));

    reg = (reg & RFLR_MODEMCONFIG2_SF_MASK) | sf;
    CHECK(lora_write_reg(handle, REG_LR_MODEMCONFIG2, reg));
    return ESP_OK;
}

esp_err_t lora_get_spreading_factor(spi_device_handle_t handle,
                                    lora_spreading_factor_t *sf_out) {
    if (sf_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t val = 0;
    CHECK(lora_read_reg(handle, REG_LR_MODEMCONFIG2, &val));

    val &= 0xF0;
    *sf_out = (lora_spreading_factor_t)val;
    return ESP_OK;
}
