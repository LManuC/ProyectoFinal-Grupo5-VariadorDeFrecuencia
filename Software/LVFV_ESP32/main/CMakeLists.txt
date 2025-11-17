idf_component_register(SRCS "./main.c" "./adc/adc.c" "./display/display.c" "./display/sh1106_graphics.c" "./display/sh1106_i2c.c" "./io_control/MCP23017.c" "./io_control/io_control.c" "./nvs/nvs.c" "./rtc/rtc.c" "./system/sysAdmin.c" "./system/sysControl.c" 
                       PRIV_REQUIRES spi_flash
                       INCLUDE_DIRS "." "adc" "display" "io_control" "nvs" "rtc" "system"
                       REQUIRES driver nvs_flash esp_timer esp_adc)
