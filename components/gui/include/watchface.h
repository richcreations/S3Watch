#pragma once
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif

void watchface_create(lv_obj_t* parent);
lv_obj_t* watchface_screen_get(void);
void watchface_rebuild(void);

// Atualiza indicadores de energia (VBUS/Carregamento/Bateria)
void watchface_set_power_state(bool vbus_in, bool charging, int battery_percent);


#ifdef __cplusplus
}
#endif
