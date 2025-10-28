idf_component_register(SRCS "main.c" "nvs.c" "display.c" "./SH1106/sh1106_graphics.c" "./SH1106/sh1106_i2c.c" "RTC.c" "MCP23017/MCP23017.c" "io_control.c" "STM32_comm.c" "ADC/adc.c"
                       PRIV_REQUIRES spi_flash
                       INCLUDE_DIRS "ADC" "."
                       REQUIRES driver nvs_flash esp_timer esp_adc)
