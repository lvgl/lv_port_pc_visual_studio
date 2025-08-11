#include "lvgl.h"
#include "lvgl/lvgl.h"
#include "my_ui.h"

void my_ui_init() {
    // Create main screen
    lv_obj_t* scr = lv_scr_act();

    // --- Yellow Box (Data Area) ---
    lv_obj_t* data_area = lv_obj_create(scr);
    lv_obj_set_size(data_area, 750, 250); // width, height in px
    lv_obj_align(data_area, LV_ALIGN_TOP_MID, 0, 25); // center top with margin
    lv_obj_set_style_border_width(data_area, 2, 0); // border only
    lv_obj_set_style_border_color(data_area, lv_color_black(), 0);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_EVENLY);


    // --- Message Area ---
    lv_obj_t* msg_area = lv_obj_create(scr);
    lv_obj_set_size(msg_area, 750, 100);
    lv_obj_align_to(msg_area, data_area, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_set_style_border_width(msg_area, 2, 0);
    lv_obj_set_style_border_color(msg_area, lv_color_black(), 0);

    lv_obj_t* label = lv_label_create(msg_area);
    lv_label_set_text(label, "Message Area");
    lv_obj_center(label);

    /* Container for buttons (centered horizontally) */
    lv_obj_t* btn_cont = lv_obj_create(scr);
    lv_obj_set_size(btn_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* btn1 = lv_btn_create(btn_cont);
    lv_obj_set_size(btn1, 225, 50);
    lv_obj_t* label1 = lv_label_create(btn1);
    lv_label_set_text(label1, "button 1");
    lv_obj_center(label1);

    lv_obj_t* btn2 = lv_btn_create(btn_cont);
    lv_obj_set_size(btn2, 225, 50);
    lv_obj_t* label2 = lv_label_create(btn2);
    lv_label_set_text(label2, "button 2");
    lv_obj_center(label2);

    lv_obj_t* btn3 = lv_btn_create(btn_cont);
    lv_obj_set_size(btn3, 225, 50);
    lv_obj_t* label3 = lv_label_create(btn3);
    lv_label_set_text(label3, "button 3");
    lv_obj_center(label3);


}
