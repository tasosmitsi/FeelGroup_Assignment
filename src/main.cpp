#include <Arduino.h>
#include <main.h>

// RGB LED states
volatile LEDState currentLEDState = LED_OFF;

// button machine states
volatile BUTTONState currentBTNState = NONE;

// Monitoring system variables
String ledStatus = "OFF";
uint vibratorActive = 0;

unsigned long lastDebounceTime = 0; // the last time the button was toggled
unsigned long idleStartTime = 0;    // Time when system became idle

int buttonState = HIGH;         // current state of the button (HIGH is unpressed)
int lastButtonState = HIGH;     // previous state of the button
bool longPressDetected = false; // flag to track if a long press was detected

// FreeRTOS handles
TimerHandle_t vibartorTimer;
TaskHandle_t fadeTaskHandle = NULL;
TaskHandle_t monitorTaskHandle = NULL;
TaskHandle_t buttonTaskHandle = NULL;

/**
 * setupPWM - PWM setup.
 *
 * This function initializes the PWM channels for the RGB LED.
 *
 * @return: TRUE if PWM setup was successful, FALSE otherwise.
 */
bool setupPWM()
{
  if (!ledcSetup(0, PWM_FREQUENCY, PWM_RESOLUTION)) {
    Serial.println("Failed to setup channel 0");
    return false;
  }
  if (!ledcSetup(1, PWM_FREQUENCY, PWM_RESOLUTION)) {
    Serial.println("Failed to setup channel 1");
    return false;
  }
  if (!ledcSetup(2, PWM_FREQUENCY, PWM_RESOLUTION)) {
    Serial.println("Failed to setup channel 2");
    return false;
  }

  ledcAttachPin(RED_PIN, 0);
  ledcAttachPin(GREEN_PIN, 2);
  ledcAttachPin(BLUE_PIN, 3);
  return true;
}

/**
 * debounceButton - Button debouncing. 
 *
 * This function debounces the button by checking the button state.
 *
 * @return: void.
 */
void debounceButton()
{
  // Read the current button state
  int reading = digitalRead(BUTTON_PIN);

  // If the button state has changed due to noise or pressing, reset the debounce timer
  if (reading != lastButtonState)
  {
    lastDebounceTime = millis(); // Reset the debounce timer
  }

  // If the button state has been stable longer than the debounce delay
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY)
  {
    // Detect button press (LOW state)
    if (reading == LOW && buttonState == HIGH)
    {
      buttonState = LOW;         // Update button state to pressed
      longPressDetected = false; // Reset long press detection
    }

    // Detect long press while the button is still pressed
    if (buttonState == LOW && (millis() - lastDebounceTime) > LONG_PRESS_DELAY && !longPressDetected)
    {
      longPressDetected = true; // Mark long press as detected
      // Serial.println("Long press detected");
      currentBTNState = LONG_PRESS;
    }

    // Detect button release (HIGH state)
    if (reading == HIGH && buttonState == LOW)
    {
      buttonState = HIGH; // Update button state to released
      if (!longPressDetected)
      {
        // If no long press was detected, it's a short press
        // Serial.println("Short press detected");
        currentBTNState = PRESSED_ONCE;
      }
      // Reset longPressDetected for the next cycle
      longPressDetected = false;
    }
  }
  // Update the last button state for the next loop iteration
  lastButtonState = reading;
}

/**
 * fadeTask - Task for fading RGB LED. 
 *
 * This function fades the RGB LED through different colors.
 *
 * @param pvParameters: void pointer to task parameters.
 * @return: void.
 */
void fadeTask(void *pvParameters)
{
  while (true)
  {
    for (int i = 0; i <= 255; i++)
    {
      ledcWrite(0, i);       // Red
      ledcWrite(1, 255 - i); // Green
      vTaskDelay(3 / portTICK_PERIOD_MS);
    }

    for (int i = 0; i <= 255; i++)
    {
      ledcWrite(2, i);       // Blue
      ledcWrite(0, 255 - i); // Red
      vTaskDelay(3 / portTICK_PERIOD_MS);
    }

    for (int i = 0; i <= 255; i++)
    {
      ledcWrite(1, i);       // Green
      ledcWrite(2, 255 - i); // Blue
      vTaskDelay(3 / portTICK_PERIOD_MS);
    }
  }
}

/**
 * updateLEDState - Function to handle LED states. 
 *
 * This function updates the LED state based on the current state.
 *
 * @return: void.
 */
void updateLEDState()
{
  switch (currentLEDState)
  {
  case LED_OFF:
    vTaskSuspend(fadeTaskHandle);
    ledcWrite(0, 0);
    ledcWrite(1, 0);
    ledcWrite(2, 0);
    ledStatus = "OFF";
    break;
  case LED_STATIC_RED:
    vTaskSuspend(fadeTaskHandle);
    ledcWrite(0, 255);
    ledcWrite(1, 0);
    ledcWrite(2, 0);
    ledStatus = "STATIC RED";
    break;
  case LED_FADE_RGB:
    if (fadeTaskHandle != NULL)
    {
      vTaskResume(fadeTaskHandle);
    }
    ledStatus = "FADING RGB";
    break;
  }
}

/**
 * updateButtonState - Function to handle button states.
 *
 * This function updates the button state based on the current state.
 *
 * @return: void.
 */
void updateButtonState()
{
  switch (currentBTNState)
  {
  case NONE:
    // Serial.println("No button press detected...");
    break;
  case PRESSED_ONCE:
    // Serial.println("Short button press detected...");
    currentLEDState = static_cast<LEDState>((currentLEDState + 1) % 3);
    currentBTNState = NONE;
    break;
  case LONG_PRESS:
    // Serial.println("Button held for more than 3 seconds");
    vibratorActive = 1;
    digitalWrite(VIBRATOR_PIN, HIGH);
    if (vibartorTimer != NULL)
    {
      // Start the timer with a delay of 0 ticks
      xTimerStart(vibartorTimer, 0);
    }
    currentBTNState = NONE;
    break;
  }
}

/**
 * monitorTask - Task for device monitoring every 2 seconds.
 *
 * This function prints the status of the button, LED, and vibrator every 2 seconds.
 *
 * @param pvParameters: void pointer to task parameters.
 * @return: void.
 */
void monitorTask(void *pvParameters)
{
  while (true)
  {
    Serial.print("Button Status: ");
    Serial.println(buttonState ? "RELEASED" : "PRESSED");
    Serial.print("LED Status: ");
    Serial.println(ledStatus);
    Serial.print("Vibrator Status: ");
    Serial.println(vibratorActive ? "ON" : "OFF");
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

/**
 * vibratorRunning - Timer callback function for vibrator.
 *
 * This function is called when the vibrator timer expires.
 *
 * @param xTimer: Timer handle.
 * @return: void.
 */
void vibratorRunning(TimerHandle_t xTimer)
{
  // Serial.println("Timer expired!");
  digitalWrite(VIBRATOR_PIN, LOW);
  vibratorActive = 0;
}

/**
 * enterSleepIfIdle - Function to enter deep sleep if idle.
 *
 * This function checks if the system is idle and enters deep sleep after IDLE_TIMEOUT seconds.
 *
 * @return: void.
 */
void enterSleepIfIdle()
{
  // Check if the system is idle (LED is off and vibrator is off)
  if (currentLEDState == LED_OFF && vibratorActive == 0)
  {
    // If system just became idle, store the current time
    if (idleStartTime == 0)
    {
      idleStartTime = millis();
    }

    // If the system has been idle for more than the idle timeout (5 seconds)
    if (millis() - idleStartTime >= IDLE_TIMEOUT)
    {
      Serial.println("System idle for 5 seconds, entering deep sleep...");

      // Configure the button pin to wake the ESP32 from deep sleep
      esp_sleep_enable_ext0_wakeup(GPIO_NUM_15, LOW); // Wake on falling edge (LOW)

      // Enter deep sleep
      esp_deep_sleep_start();
    }
  }
  else
  {
    // Reset the idle timer if the system is not idle
    idleStartTime = 0;
  }
}

void setup()
{
  // Initialize serial communication
  Serial.begin(115200);

  // Check if the ESP32 woke up from deep sleep
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0)
  {
    Serial.println("Woke up from deep sleep");
  }

  // Initialize pins
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(VIBRATOR_PIN, OUTPUT);

  // Initialize PWM
  setupPWM();

  // Create monitoring task
  xTaskCreate(monitorTask, "Monitor Task", 1024, NULL, 1, &monitorTaskHandle);

  // Create vibrator timer
  vibartorTimer = xTimerCreate(
      "vibartorTimer",      // Human-readable name for debugging
      pdMS_TO_TICKS(10000), // Timer period in ticks (time duration)
      false,                // If pdTRUE, the timer will automatically reload itself (i.e., repeat periodically)
      NULL,                 // Optional user-defined ID associated with the timer
      vibratorRunning       // Function called when the timer expires
  );

  // Create led fade task and suspend it
  xTaskCreate(fadeTask, "LED Fade Task", 1024, NULL, 1, &fadeTaskHandle);
  // Suspend the task initially
  vTaskSuspend(fadeTaskHandle);
}

void loop()
{
  updateLEDState();
  debounceButton();
  updateButtonState();
  enterSleepIfIdle();
}