idf_component_register(
    SRCS
        "source/main.cpp"
        "source/driver/adc/esp32s3.cpp"
        "source/driver/gpio/esp32s3.cpp"
        "source/driver/serial/esp32s3.cpp"
        "source/communication/wifi/esp32s3.cpp"
        "source/communication/mqtt/publish/esp32s3.cpp"
        "source/communication/mqtt/subscribe/esp32s3.cpp"
        #"source/driver/tempsensor/tmp36.cpp"
        #"source/driver/timer/esp32s3.cpp"
    INCLUDE_DIRS
        "include"
    REQUIRES
        esp_driver_gpio
        esp_adc
        esp_wifi
        esp_netif
        nvs_flash
        esp_event
        espressif__mqtt

    PRIV_REQUIRES
        esp_driver_uart
        esp_driver_gpio   
        esp_driver_usb_serial_jtag

        
)
