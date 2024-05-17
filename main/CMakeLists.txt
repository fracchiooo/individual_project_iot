idf_component_register(SRCS "progetto_singolo.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_adc read_continuous fft_wrapper mqtt_wrapper wifi_wrapper
                    )
