idf_component_register(
    SRCS
        "main.c"
        "gpio/gpio.c"
        "i2c/i2c.c"
        "lora/lora.c"
        "oled/oled.c"
        "packet/packet.c"
    INCLUDE_DIRS "."
)
