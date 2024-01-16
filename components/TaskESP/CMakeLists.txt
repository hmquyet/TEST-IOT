set(app_src gpio_task.c initiate_task.c nvs_task.c)
set(pri_req)
idf_component_register(SRCS ${app_src}
                    INCLUDE_DIRS "." 
                    REQUIRES ${pri_req}
                    PRIV_REQUIRES main nvs_flash app_update) #link với file main