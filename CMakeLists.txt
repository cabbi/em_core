file(GLOB SOURCES_LIST "src/*.cpp" "src/*.c")

idf_component_register(
    SRCS "${SOURCES_LIST}"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_timer esp_driver_gpio esp_driver_uart esp_adc nvs_flash
)