#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <math.h>

namespace {

constexpr uint16_t kDisplayWidth = 320;
constexpr uint16_t kDisplayHeight = 240;
constexpr uint16_t kMaxRpm = 8000;
constexpr uint16_t kRedlineRpm = 6000;
constexpr float kFirstTickDegrees = 125.0f;
constexpr float kSweepDegrees = 250.0f;
constexpr float kPi = 3.14159265f;

TFT_eSPI tft;
alignas(4) static uint16_t drawBuffer[kDisplayWidth * 30];
static lv_point_precise_t scalePoints[81][2];

lv_obj_t* needle;
lv_obj_t* speedLabel;
lv_obj_t* gearLabel;
lv_point_precise_t needlePoints[2];

#if LV_FONT_MONTSERRAT_32
const lv_font_t* const kGaugeDigitFont = &lv_font_montserrat_32;
#elif LV_FONT_MONTSERRAT_24
const lv_font_t* const kGaugeDigitFont = &lv_font_montserrat_24;
#else
const lv_font_t* const kGaugeDigitFont = LV_FONT_DEFAULT;
#endif

lv_color_t color(uint32_t hex)
{
    return lv_color_hex(hex);
}

void flushDisplay(lv_display_t* displayToFlush, const lv_area_t* area, uint8_t* pixels)
{
    const uint32_t width = static_cast<uint32_t>(lv_area_get_width(area));
    const uint32_t height = static_cast<uint32_t>(lv_area_get_height(area));

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, width, height);
    tft.pushColors(reinterpret_cast<uint16_t*>(pixels), width * height, true);
    tft.endWrite();
    lv_display_flush_ready(displayToFlush);
}

lv_obj_t* makeOval(lv_obj_t* parent, int32_t width, int32_t height,
                   int32_t centerX, int32_t centerY, uint32_t topColor,
                   uint32_t bottomColor, uint32_t borderColor, int32_t borderWidth)
{
    lv_obj_t* oval = lv_obj_create(parent);
    lv_obj_remove_style_all(oval);
    lv_obj_clear_flag(oval, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(oval, width, height);
    lv_obj_set_pos(oval, centerX - width / 2, centerY - height / 2);
    lv_obj_set_style_radius(oval, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(oval, color(topColor), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(oval, color(bottomColor), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(oval, LV_GRAD_DIR_VER, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(oval, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(oval, color(borderColor), LV_PART_MAIN);
    lv_obj_set_style_border_width(oval, borderWidth, LV_PART_MAIN);
    lv_obj_set_style_pad_all(oval, 0, LV_PART_MAIN);
    return oval;
}

lv_obj_t* makeLabel(lv_obj_t* parent, const char* text, int32_t x, int32_t y,
                    int32_t width, int32_t height, uint32_t textColor,
                    const lv_font_t* font)
{
    lv_obj_t* label = lv_label_create(parent);
    lv_obj_clear_flag(label, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_label_set_text(label, text);
    lv_obj_set_size(label, width, height);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_style_text_color(label, color(textColor), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_opa(label, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(label, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(label, 0, LV_PART_MAIN);
    return label;
}

void drawScale(lv_obj_t* screen, int32_t centerX, int32_t centerY)
{
    for (int32_t tick = 0; tick <= 80; ++tick) {
        const float angle = (kFirstTickDegrees + kSweepDegrees * tick / 80.0f) * kPi / 180.0f;
        const bool major = tick % 10 == 0;
        const bool medium = tick % 5 == 0;
        const float innerRadiusX = major ? 60.0f : (medium ? 66.0f : 70.0f);
        const float innerRadiusY = major ? 49.0f : (medium ? 55.0f : 59.0f);
        const float outerRadiusX = 77.0f;
        const float outerRadiusY = 63.0f;
        const uint32_t tickColor = tick >= 60 ? 0xFF484A : (major ? 0xC6E9F8 : 0x81B3C9);
        const int32_t width = major ? 2 : 1;
        scalePoints[tick][0] = {
            static_cast<lv_coord_t>(roundf(centerX + cosf(angle) * innerRadiusX)),
            static_cast<lv_coord_t>(roundf(centerY + sinf(angle) * innerRadiusY)),
        };
        scalePoints[tick][1] = {
            static_cast<lv_coord_t>(roundf(centerX + cosf(angle) * outerRadiusX)),
            static_cast<lv_coord_t>(roundf(centerY + sinf(angle) * outerRadiusY)),
        };
        lv_obj_t* tickLine = lv_line_create(screen);
        lv_line_set_points(tickLine, scalePoints[tick], 2);
        lv_obj_set_style_line_color(tickLine, color(tickColor), LV_PART_MAIN);
        lv_obj_set_style_line_width(tickLine, width, LV_PART_MAIN);
        lv_obj_set_style_line_rounded(tickLine, true, LV_PART_MAIN);
    }

    for (int32_t value = 0; value <= 8; ++value) {
        const float angle = (kFirstTickDegrees + kSweepDegrees * value / 8.0f) * kPi / 180.0f;
        const int32_t x = static_cast<int32_t>(roundf(centerX + cosf(angle) * 51.0f));
        const int32_t y = static_cast<int32_t>(roundf(centerY + sinf(angle) * 42.0f));
        const uint32_t textColor = value >= 6 ? 0xFF7774 : 0xDCECF5;
        char numeral[2] = { static_cast<char>('0' + value), '\0' };
        makeLabel(screen, numeral, x - 10, y - 10, 20, 20, textColor, LV_FONT_DEFAULT);
    }
}

void updateNeedle(uint16_t rpm)
{
    if (rpm > kMaxRpm) {
        rpm = kMaxRpm;
    }

    const float fraction = static_cast<float>(rpm) / kMaxRpm;
    const float angle = (kFirstTickDegrees + kSweepDegrees * fraction) * kPi / 180.0f;
    constexpr int32_t centerX = kDisplayWidth / 2;
    constexpr int32_t centerY = 120;
    needlePoints[0] = { centerX, centerY };
    needlePoints[1] = {
        static_cast<lv_coord_t>(roundf(centerX + cosf(angle) * 62.0f)),
        static_cast<lv_coord_t>(roundf(centerY + sinf(angle) * 51.0f)),
    };
    lv_line_set_points(needle, needlePoints, 2);
    lv_obj_set_style_line_color(needle, color(rpm >= kRedlineRpm ? 0xFF303B : 0xFF594E), LV_PART_MAIN);
}

void createGauge(lv_obj_t* screen)
{
    constexpr int32_t centerX = kDisplayWidth / 2;
    constexpr int32_t centerY = 120;

    makeOval(screen, 170, 140, centerX, centerY, 0x435665, 0x1B2730, 0x748C9B, 2);
    makeOval(screen, 160, 132, centerX, centerY, 0x101D26, 0x263B49, 0xA9BBC4, 2);
    makeOval(screen, 146, 118, centerX, centerY, 0x0B1822, 0x07121B, 0x294253, 2);
    makeOval(screen, 88, 82, centerX, centerY, 0x0B2536, 0x071522, 0x1B4359, 2);

    drawScale(screen, centerX, centerY);

    needle = lv_line_create(screen);
    lv_obj_set_style_line_width(needle, 3, LV_PART_MAIN);
    lv_obj_set_style_line_rounded(needle, true, LV_PART_MAIN);
    updateNeedle(6000);

    makeLabel(screen, "RPM", 132, 83, 56, 17, 0x9CC8D9, LV_FONT_DEFAULT);
    speedLabel = makeLabel(screen, "75", 119, 101, 82, 43, 0x62D4FF, kGaugeDigitFont);
    makeLabel(screen, "KPM", 132, 143, 56, 18, 0x88C9E2, LV_FONT_DEFAULT);

    lv_obj_t* gearBox = lv_obj_create(screen);
    lv_obj_remove_style_all(gearBox);
    lv_obj_clear_flag(gearBox, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(gearBox, 68, 43);
    lv_obj_set_pos(gearBox, centerX - 34, 159);
    lv_obj_set_style_radius(gearBox, 7, LV_PART_MAIN);
    lv_obj_set_style_bg_color(gearBox, color(0x171C20), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(gearBox, color(0x40372F), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(gearBox, LV_GRAD_DIR_VER, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(gearBox, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(gearBox, color(0x9AA8AD), LV_PART_MAIN);
    lv_obj_set_style_border_width(gearBox, 2, LV_PART_MAIN);

    gearLabel = makeLabel(gearBox, "4", 0, 0, 68, 43, 0xFFE0C4, kGaugeDigitFont);
}

} // namespace

void smart_gauge_set_values(uint16_t rpm, uint16_t speed_kpm, uint8_t gear)
{
    if (rpm > kMaxRpm) {
        rpm = kMaxRpm;
    }
    updateNeedle(rpm);

    char text[8];
    snprintf(text, sizeof(text), "%u", static_cast<unsigned int>(speed_kpm));
    lv_label_set_text(speedLabel, text);
    snprintf(text, sizeof(text), "%u", static_cast<unsigned int>(gear));
    lv_label_set_text(gearLabel, text);
}

void setup()
{
    Serial.begin(115200);

    tft.begin();
    tft.setRotation(1);
    if (tft.width() != kDisplayWidth || tft.height() != kDisplayHeight) {
        Serial.printf("Smart gauge needs a 320x240 TFT_eSPI viewport; configured viewport is %dx%d.\n",
                      tft.width(), tft.height());
        while (true) {
            delay(1000);
        }
    }
    tft.fillScreen(TFT_BLACK);

    lv_init();
    lv_display_t* display = lv_display_create(kDisplayWidth, kDisplayHeight);
    if (display == nullptr) {
        Serial.println("LVGL could not create the display.");
        while (true) {
            delay(1000);
        }
    }
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(display, flushDisplay);
    lv_display_set_buffers(display, drawBuffer, nullptr, sizeof(drawBuffer), LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_obj_t* screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, color(0x05090C), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    createGauge(screen);
    smart_gauge_set_values(6000, 75, 4);
}

void loop()
{
    static uint32_t previousMillis = millis();
    const uint32_t now = millis();
    lv_tick_inc(now - previousMillis);
    previousMillis = now;
    lv_timer_handler();
    delay(5);
}
