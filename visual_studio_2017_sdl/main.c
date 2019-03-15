/**
* @file main
*
*/

/*********************
*      INCLUDES
*********************/
#include <stdlib.h>
#include <Windows.h>
#include <SDL.h>
#include "lvgl/lvgl.h"
#include "lv_drivers/display/monitor.h"
#include "lv_drivers/indev/mouse.h"
#include "lv_drivers/indev/keyboard.h"
#include "lv_examples/lv_apps/demo/demo.h"
#include "lv_examples/lv_apps/benchmark/benchmark.h"
#include "lv_examples/lv_tests/lv_test_theme/lv_test_theme_1.h"
#include "lv_examples/lv_tutorial/10_keyboard/lv_tutorial_keyboard.h"

/*********************
*      DEFINES
*********************/

/**********************
*      TYPEDEFS
**********************/

/**********************
*  STATIC PROTOTYPES
**********************/
static void hal_init(void);
static int tick_thread(void *data);

/**********************
*  STATIC VARIABLES
**********************/
static lv_indev_t * kb_indev;

/**********************
*      MACROS
**********************/

/**********************
*   GLOBAL FUNCTIONS
**********************/

int main(int argc, char** argv)
{
    /*Initialize LittlevGL*/
    lv_init();

    /*Initialize the HAL for LittlevGL*/
    hal_init();

    /*
     * Demo, benchmark, tests and tutorial.
     *
     * Uncomment any one (and only one) of the functions below to run that
     * particular demo, test or tutorial.
     */

    demo_create();
    //benchmark_create();
    //lv_test_theme_1(lv_theme_night_init(210, NULL));
    //lv_test_theme_1(lv_theme_night_init(100, NULL));
    //lv_test_theme_1(lv_theme_material_init(210, NULL));
    //lv_test_theme_1(lv_theme_alien_init(210, NULL));
    //lv_test_theme_1(lv_theme_zen_init(210, NULL));
    //lv_test_theme_1(lv_theme_nemo_init(210, NULL));
    //lv_test_theme_1(lv_theme_mono_init(210, NULL));
    //lv_test_theme_1(lv_theme_default_init(210, NULL));
    //lv_tutorial_keyboard(kb_indev);

    while (1) {
        /* Periodically call the lv_task handler.
        * It could be done in a timer interrupt or an OS task too.*/
        lv_task_handler();
        Sleep(10);       /*Just to let the system breathe */
    }

    return 0;
}

/**********************
*   STATIC FUNCTIONS
**********************/


/**
* Initialize the Hardware Abstraction Layer (HAL) for the Littlev graphics library
*/
static void hal_init(void)
{
    /* Add a display
    * Use the 'monitor' driver which creates window on PC's monitor to simulate a display*/
    monitor_init();
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);            /*Basic initialization*/
    disp_drv.disp_flush = monitor_flush;
    disp_drv.disp_fill = monitor_fill;
    disp_drv.disp_map = monitor_map;
    lv_disp_drv_register(&disp_drv);

    /* Add the mouse (or touchpad) as input device
    * Use the 'mouse' driver which reads the PC's mouse*/
    mouse_init();
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);          /*Basic initialization*/
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read = mouse_read;         /*This function will be called periodically (by the library) to get the mouse position and state*/
    lv_indev_drv_register(&indev_drv);

    /* If the PC keyboard driver is enabled in`lv_drv_conf.h`
    * add this as an input device. It might be used in some examples. */
#if USE_KEYBOARD
    lv_indev_drv_t kb_drv;
    kb_drv.type = LV_INDEV_TYPE_KEYPAD;
    kb_drv.read = keyboard_read;
    kb_indev = lv_indev_drv_register(&kb_drv);
#endif

    /* Tick init.
    * You have to call 'lv_tick_inc()' in every milliseconds
    * Create an SDL thread to do this*/
    SDL_CreateThread(tick_thread, "tick", NULL);
}

/**
* A task to measure the elapsed time for LittlevGL
* @param data unused
* @return never return
*/
static int tick_thread(void *data)
{
    while (1) {
        lv_tick_inc(1);
        SDL_Delay(1);   /*Sleep for 1 millisecond*/
    }

    return 0;
}
