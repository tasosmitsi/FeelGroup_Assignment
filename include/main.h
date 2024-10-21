#ifndef MAIN_H
#define MAIN_H

// Pin definitions
#define BUTTON_PIN 15
#define RED_PIN 5
#define GREEN_PIN 4
#define BLUE_PIN 2
#define VIBRATOR_PIN 21

// PWM settings
#define PWM_FREQUENCY 5000
#define PWM_RESOLUTION 8

// System settings
#define IDLE_TIMEOUT 10000     // 5 seconds of idle before sleep
#define DEBOUNCE_DELAY 50     // Debounce time in milliseconds
#define LONG_PRESS_DELAY 3000 // Long press duration in milliseconds

enum LEDState
{
    LED_OFF,
    LED_STATIC_RED,
    LED_FADE_RGB
};
enum BUTTONState
{
    NONE,
    PRESSED_ONCE,
    LONG_PRESS
};

bool setupPWM();
void debounceButton();
void fadeTask(void *pvParameters);
void updateLEDState();
void updateButtonState();
void monitorTask(void *pvParameters);
void vibratorRunning(TimerHandle_t xTimer);
void enterSleepIfIdle();

#endif