idf_component_register(SRCS "main.c" "sh1106.c" "sh1106_i2c.c" "sh1106_graphics.c"
                       PRIV_REQUIRES spi_flash
                       INCLUDE_DIRS ""
                       REQUIRES driver )
