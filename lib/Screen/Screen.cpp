#include "Screen.h"
#include <Adafruit_ILI9341.h>
#include <Adafruit_GFX.h>
#include <XPT2046.h>
#include "Settings.h"

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
XPT2046 touch(TS_CS, TS_IRQ);
Adafruit_GFX_Button test;
uint32_t last_current_pwr = 0;
uint32_t last_last_pwr = 0;
uint32_t last_diff = 0;

void Screen::begin()
{
    SPI.setFrequency(80000000L);
    tft.begin();
    touch.begin(tft.width(), tft.height());
    touch.setCalibration(1835, 283, 283, 1768);
    tft.fillScreen(ILI9341_BLACK);
    tft.setRotation(1);

    /*char test[] = "ON";
    test.initButton(&tft, 60, 200, 100, 40, ILI9341_WHITE, ILI9341_CYAN, ILI9341_BLACK, test, 2);
    test.drawButton(false);*/
}

void Screen::sleep()
{
    is_sleeping = true;
    slp_time = 0;
    digitalWrite(SCR_LED, HIGH);
}

void Screen::wake()
{
    is_sleeping = false;
    slp_time = 0;
    digitalWrite(SCR_LED, LOW);
}

void Screen::checkWake(uint32_t pwr, uint32_t last_pwr)
{
    uint32_t pwr_diff = abs((int)pwr - (int)last_pwr);
    if (pwr_diff > user_wake_power)
    {
        if (pwr > last_pwr)
            increasedBy(abs((int32_t)pwr - (int32_t)last_pwr), pwr, last_pwr);
        else
            decreasedBy(abs((int32_t)pwr - (int32_t)last_pwr), pwr, last_pwr);

        if (pwr > last_pwr)
            wake();
    }
}

void Screen::ledBlink(uint32_t pin)
{
    if (pin != WORK_LED)
    {
        digitalWrite(pin, LOW);
        delay(10);
        digitalWrite(pin, HIGH);
    }
    else
    {
        if (enable_work_led)
        {
            digitalWrite(pin, LOW);
            delay(10);
            digitalWrite(pin, HIGH);
        }
    }
}

void Screen::loop()
{
    if (touch.isTouching())
    {
        wake();
        /*uint16_t p, x;
        touch.getPosition(p, x);
        Serial.println("X Y ");
        Serial.print(p);
        Serial.print(" ");
        Serial.println(x);
        test.press(test.contains(p, x));
        if (test.justPressed())
            test.drawButton(true);*/
    }
}

void Screen::writePower(uint32_t pwr, float current, uint32_t p_pwr, float p_current)
{
    tft.setTextSize(8);
    tft.setTextColor(ILI9341_BLACK);
    tft.setCursor(0, 0);
    tft.print(p_current);
    tft.print("A");
    tft.setCursor(0, 170);
    tft.print(p_pwr);
    tft.print("W");
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(0, 0);
    tft.print(current);
    tft.print("A");
    tft.setCursor(0, 170);
    tft.print(pwr);
    tft.print("W");
}

void Screen::drawGreenArrow(bool erase_last)
{
    if (erase_last)
    {
        tft.fillRect(15, 115, 25, 25, ILI9341_BLACK);
        tft.fillTriangle(0, 115, 27, 90, 55, 115, ILI9341_BLACK);
    }
    tft.fillRect(15, 90, 25, 25, ILI9341_GREEN);
    tft.fillTriangle(0, 115, 27, 140, 55, 115, ILI9341_GREEN);
}

void Screen::drawRedArrow(bool erase_last)
{
    if (erase_last)
    {
        tft.fillRect(15, 90, 25, 25, ILI9341_BLACK);
        tft.fillTriangle(0, 115, 27, 140, 55, 115, ILI9341_BLACK);
    }
    tft.fillRect(15, 115, 25, 25, ILI9341_RED);
    tft.fillTriangle(0, 115, 27, 90, 55, 115, ILI9341_RED);
}

void Screen::increasedBy(uint32_t diff, uint32_t current_pwr, uint32_t last_pwr)
{
    drawRedArrow(true);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(5);
    tft.setCursor(60, 76);
    tft.println(last_last_pwr);
    tft.print("  ");
    tft.print(last_current_pwr);
    tft.setCursor(190, 96);
    tft.print(last_diff);

    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(60, 76);
    tft.println(last_pwr);
    tft.print("  ");
    tft.print(current_pwr);
    tft.setCursor(190, 96);
    tft.print(diff);
    last_diff = diff;
    last_current_pwr = current_pwr;
    last_last_pwr = last_pwr;
}

void Screen::decreasedBy(uint32_t diff, uint32_t current_pwr, uint32_t last_pwr)
{
    drawGreenArrow(true);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(5);
    tft.setCursor(60, 76);
    tft.println(last_last_pwr);
    tft.print("  ");
    tft.print(last_current_pwr);
    tft.setCursor(190, 96);
    tft.print(last_diff);

    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(60, 76);
    tft.println(last_pwr);
    tft.print("  ");
    tft.print(current_pwr);
    tft.setCursor(190, 96);
    tft.print(diff);
    last_diff = diff;
    last_current_pwr = current_pwr;
    last_last_pwr = last_pwr;
}
