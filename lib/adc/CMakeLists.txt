idf_component_register(SRCS "adc.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES esp_adc esp_timer driver freertos esp_hw_support)