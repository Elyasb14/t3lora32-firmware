# LoRa Driver Implementation Plan

## Summary
Complete the LoRa driver by adding configuration functions for bandwidth, spreading factor, coding rate, and other modem settings.

## Changes to lora.h

### Enums (already present)
- `lora_bandwidth_t` - Bandwidth values for REG_LR_MODEMCONFIG1
- `lora_spreading_factor_t` - Spreading factor values for REG_LR_MODEMCONFIG2
- `lora_coding_rate_t` - Coding rate values for REG_LR_MODEMCONFIG1

### New Function Prototypes
```c
// Modem configuration
void lora_set_spreading_factor(spi_device_handle_t handle, lora_spreading_factor_t sf);
void lora_set_coding_rate(spi_device_handle_t handle, lora_coding_rate_t cr);

// Packet configuration
void lora_set_preamble_length(spi_device_handle_t handle, uint16_t length);
void lora_set_sync_word(spi_device_handle_t handle, uint8_t sync_word);
void lora_set_crc_enable(spi_device_handle_t handle, bool enable);
void lora_set_implicit_header_mode(spi_device_handle_t handle, bool enable);

// Timing configuration
void lora_set_symbol_timeout(spi_device_handle_t handle, uint8_t timeout);
```

## Changes to lora.c

### Fix existing bug
- `lora_set_bandwidth` currently overwrites REG_LR_MODEMCONFIG1 completely
- Should mask and OR with existing settings

### Add stub implementations
- Implement all new functions with basic register read/write operations
- Use proper masking to avoid overwriting other settings

## Implementation Details

### lora_set_bandwidth (fix)
1. Read REG_LR_MODEMCONFIG1
2. Mask with RFLR_MODEMCONFIG1_BW_MASK (0x0F)
3. OR with bandwidth value
4. Write back

### lora_set_spreading_factor
1. Read REG_LR_MODEMCONFIG2
2. Mask with RFLR_MODEMCONFIG2_SF_MASK (0x0F)
3. OR with spreading factor value
4. Write back

### lora_set_coding_rate
1. Read REG_LR_MODEMCONFIG1
2. Mask with RFLR_MODEMCONFIG1_CODINGRATE_MASK (0xF1)
3. OR with coding rate value
4. Write back

### Other functions
Similar pattern: read, mask, OR, write