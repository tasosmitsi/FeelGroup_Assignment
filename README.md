# FeelRobotics assignment

## Approaching the problem

- Development board: ESP32 WROOM 32 DevKit
- RGB LED was simulated using 3 LEDs (Red, Green, Blue)
- Button was simulated using a wire connected to a GPIO pin
- The code was written in C using the Arduino framework and PlatformIO
- The project is based on freeRTOS to handle the tasks

After the wiring on the bread board was done, I started by writing the code to control the LED. I used PWM to control the brightness of the LEDs. The color of the LED goes as follows: RED -> GREEN -> BLUE -> RED -> ... by fading the current color and increasing the next color at the same time. This can be changed to a different pattern if needed, depending on the need to show all the intermediate colors or just the primary colors.

**Task 1**: Debounce the button and implement a normal and a long press feature. The normal press should toggle the LED betweem the three states and it is triggered by the release of the button. The long press activates the vibration motor and it is triggered by the press of the button for more than 3 seconds without necessarily releasing it.

**Task 2**: Implement a simple state machine to control the LED. The LED should be off by default. When the button is pressed, the LED should turn on and cycle through the three stated.

**Task 3**: Implement a simple state machine regarding the button. The button should be in the NONE state by default. When the button is pressed normally, it should go to the PRESSED_ONCE state. When the button is pressed for more than 3 seconds, it should go to the LONG_PRESS state. When the button is released, it should go back to the NONE state.

**Task 4**: Regarding the button state, if the button is in the PRESSED_ONCE state, I update the LED state circularly. If the button is in the LONG_PRESS state, I activate the vibration motor and a timer.

**Task 5**: The vibration motor timer will expire after 2 second and the vibration motor will be turned off. If the vibration duration was more than 3 seconds and the user managed to long press the button while the vibration motor was on, the vibration motor would run for another durration time after the first period. In other words, the time would be cumulative.

**Task 6**: As for monitoring of the system, I decided to use the serial monitor to print the current state of the button, the LED, the vibration motor, and some other logging information. This can be changed to a more sophisticated monitoring system in the future. For exammple an sd memory card / spi flash to store the logs or mqtt protocol to send the logs to a server.

## Challenges and solutions

**Chalenge 1**: 
    Contact bounce (also called chatter) is a common problem with mechanical switches, relays and battery contacts, which arises as the result of electrical contact resistance (ECR) phenomena at interfaces. Switch and relay contacts are usually made of springy metals. When the contacts strike together, their momentum and elasticity act together to cause them to bounce apart one or more times before making steady contact. The result is a rapidly pulsed electric current instead of a clean transition from zero to full current. The effect is usually unimportant in power circuits, but causes problems in some analogue and logic circuits that respond fast enough to misinterpret the on‑off pulses as a data stream. (Wikipedia)

**Theoretical Solution 1**:
    Contact circuit voltages can be low-pass filtered to reduce or eliminate multiple pulses from appearing.

**Theoretical Solution 2**:
    In digital systems, multiple samples of the contact state can be taken at a low rate and examined for a steady sequence, so that contacts can settle before the contact level is considered reliable and acted upon.

**Practical Solution**:
    I used a simple debouncing algorithm that waits for a certain amount of time before reading the button state again. This way, the button state is only read once the button has been stable for a certain amount of time. This solution also allowed me to count the time of a button press so that I could implement the long press feature.

**Chalenge 2**: While in deep sleep mode, the ESP32 can only wake up from a button press or a timer. The button press can be used to wake up the system but that would require one more single press to wake up the system. One to wake up the system and one to change the LED state.

**Ideal Solution**:
    A capacitor can be used to slow down the button press. This way, the system will wake up and the button press will be registered as a normal press.

**Practical Solution**:
    I made sure the code is fast enough to wake up the system and register a normal press if the button is pressed NOT super fast. This is not ideal but it is a good compromise.

## Power efficiency

The system should be power efficient. The ESP32 has a deep sleep mode that can be used to save power. After 10 seconds of inactivity (LED off, vibration motor off, button not pressed), the system will go to deep sleep mode. The system will wake up when the button is pressed. 

If the button is pressed long enough while in sleep, the system will wake up and the led state will change. Long enough means that the button is pressed normally and not super fast. Here to ensure that the system does wake up and at the same time registers a normal press, a button capacitor can be used to slow down the button press.

If the button is long pressed while the system is in deep sleep, the system will wake up and the vibration motor will be activated. The system will stay awake until the vibration motor is turned off and in inactivity for 10 seconds.

## Error handling

The code has some error handling. For example:
1. If the vibration motor is on and the button is long pressed again, the vibration motor will be on for another 2 seconds after the first period comulatively.
2. Debouncing the button to avoid contact bounce.
3. I used if statements to check if the initialization of the pwm channels was successful.

Note: A lot more error handling can be added to the code but I decided that this would be beyond the scope of this assignment.

## Testing

The code was tested on the ESP32 WROOM 32 DevKit. The code was tested using the serial monitor to print the current state of the button, the LED, the vibration motor, and some other logging information. The code was tested for all the above-mentioned features and usage senarios. In a production environment, the code should be tested on the actual hardware and the power consumption should be measured. The code should be tested for all possible senarios and edge cases. I would also recommend writing unit tests for all the functions and functionalities.