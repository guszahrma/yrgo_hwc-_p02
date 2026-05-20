idf_component_register(
    SRCS
        "source/main.cpp"
        #"source/driver/factory/esp32s3.cpp"
        "source/driver/adc/esp32s3.cpp"
        "source/driver/gpio/esp32s3.cpp"
        "source/driver/serial/esp32s3.cpp"
        "source/driver/tempsensor/tmp36.cpp"
        #"source/driver/timer/esp32s3.cpp"
    INCLUDE_DIRS
        "include"
    PRIV_REQUIRES
        esp_driver_uart
        esp_driver_gpio   
        esp_driver_usb_serial_jtag
        
)
