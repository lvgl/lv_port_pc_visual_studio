#include "lvgl.h"
#include "my_ui.h"

void my_ui_init(void) {
    lv_obj_t* label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "こんにちは世界");  // Japanese text
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}
