#ifndef _SCREEN_H_
#define _SCREEN_H_

#include <stdint.h>

#define TFT_DC 2
#define TFT_CS 15
#define TS_CS 4
#define TS_IRQ 5

class Screen
{
    private:
        void drawRedArrow(bool erase_last);
        void drawGreenArrow(bool erase_last);

    public:
        bool is_sleeping = false;
        bool overload_warn = false;
        bool car_turned_on = true;  // true così non cambia lo stato quando si accende l'esp
        uint32_t seconds_without_overload = 0;
        uint32_t overload_time = 0;
        uint32_t blynk_overload_time = 0;
        uint32_t enable_work_led = 0;
        uint32_t slp_time = 0;          // tempo che è stato acceso lo schermo
        uint32_t user_slp_time = 15;    // secondi
        uint32_t user_override = 0;
        uint32_t user_wake_power = 350; // watt
        void loop();
        void begin();
        void checkWake(uint32_t pwr, uint32_t last_pwr);
        void wake();
        void sleep();
        void ledBlink(uint32_t pin);
        void writePower(uint32_t pwr, float current, uint32_t p_pwr, float p_current);
        void decreasedBy(uint32_t diff, uint32_t current_pwr, uint32_t last_pwr);
        void increasedBy(uint32_t diff, uint32_t current_pwr, uint32_t last_pwr);
};

#endif