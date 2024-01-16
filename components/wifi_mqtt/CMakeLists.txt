set(app_src wifi_mqtt.c  ) #wifi_connect.c
set(pri_req)
idf_component_register(SRCS ${app_src}
                    INCLUDE_DIRS "." "D:/ESP32/Espressif/frameworks/esp-idf-v4.3.6/components/mqtt/esp-mqtt/include" 
                    PRIV_REQUIRES  "mqtt" 
                    REQUIRES ${pri_req}
                    PRIV_REQUIRES main nvs_flash app_update)