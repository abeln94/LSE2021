# V4 Interact force feedback wheel HID adapter (SV-283)

This proyect contains all the files used in the making of a teensy 2.0 (arduino) HID adapter to use a windows 95 wheel joystick on modern computers via USB.

## Summary

The wheel corresponds to a V4 interact force feedback model (SV-283)

![wheel](images/wheel.jpg?raw=true "Wheel")

This wheel was designed to be used in a Windows 95/98 machine via gameport. The driver no longer works on newer windows versions (it does on virtual machines) and the gameport connector is no longer present in practically all modern computers, (and it isn't supported on virtual machines either, at least the most common ones).

This proyect's objective was to create an adapter to be able to use this wheel in newer machines, via USB. Several steps were performed in order to achieve it and some tools were developed in the process too. In the end an adapter was created with a teensy (arduino) and even 'packed' to be able to use it as a joystick (sadly without force feedback).

![adapter](images/adapter.jpg?raw=true "Adapter")

## Steps

The steps performed were:

1) Obtain a working windows 95 machine to install the drivers in order to have a working environment.

1) Measure the pins of the gameport connection in order to detect and understand the protocol.

1) Replicate the protocol on an Arduino to simulate the windows 95 driver and connect the wheel with a modern computer.

### Working environment

In order to analyze the protocol a working windows 95 computer was used. The wheel's drivers were searched and found on the internet (unfortunately the original CD was lost) and then installed.

The drivers used are on folder `drivers`. Unfortunately the original pages where they were downloaded were not recorded. All rights correspond to the respective owners (the files are here for documentation).
- `sv-283_1.00.930.exe` correspond to the driver itself.
- `dx61eng.exe` is the installer for direct x 6.1, which is needed for the driver.
- `V120_B906_SV283Drv.zip` is a packed version which I think didn't work? I don't remember.

### Protocol decoding

Notes for the protocol are in `notes.txt`.

Once a working environment was available, the driver had a software to test the buttons and some force feedback effects. (samples were recorded in the `measurements` folder). It was discovered that the computer send the command via the MIDI TXD pin (gameport#12) using a MIDI protocol (uart with baud-rate 31250) in the format `A5 XX ...` where `XX` is a command id, and then optional parameters. All the commands discovered are on `commands.txt`.

The wheel answers via the BTN (1,2,3,4) pins (gameport#2,7,10,14), where the BTN3 pin is used for a clock signal and the others for data, to be read on clock's rising edge. Some commands return only 3 bits (a single clock signal) and others return multiple. Note that MIDI RXD pin is not used (nothing was transmitted in any of the tests).

![protocol](images/protocol.png?raw=true "Protocol") From top to bottom: BTN3, BTN1, BTN2, BTN4, _, _, _, MIDI TXD

Check the files `notes.txt` and `commands.txt` for the different pins and the responses meaning.



[PulseView](https://sigrok.org/wiki/PulseView) was used for analyzing the signals (with a USBee adapter). This software supports protocols to decode the signals automatically. A decoder for this project was developed (uses uart as input). It can be found on the `PulseView` folder. To install and use it simply copy the folder contents (the `share` folder) in the PulseView installation (`C:\Program Files\sigrok\PulseView` for windows). Then choose it from the program list of decoders, with the name `ForceFeedback`.

### Adapter Development

For the adapter a [Teensy® USB Development Board](https://www.pjrc.com/teensy/) was used, because it contains libraries for HID devices (so you can simply say `Jostick.X(128);` and forget about the horrible HID development). The project is inside the `arduino` folder, called `send`. To install it on a teensy (assuming you already have the environment prepared) open on Arduino, choose `USB type: serial + keyboard + mouse + joystick` and flash. A hex file is also provided for simplicity. 

The connections between the gameport and the teensy pinout are: 
- Gameport pin 12 to teensy pin D3: the midi TXD, which connects to the serial library. Can't be changed.
- Gameport pin 10 to teensy D0: the clock signal, by editing the source code can be changed to one of D0-3 because those are the only ones that fire interruptions.
- Gameport pin 2 to teensy B0: one of the data input. By editing the source code can be changed to almost any other pin, but preferably in the same PIN block (letter) as the other data, for simultaneous read.
- Gameport pin 7 to teensy B1: one of the data input. By editing the source code can be changed to almost any other pin, but preferably in the same PIN block (letter) as the other data, for simultaneous read.
- Gameport pin 14 to teensy B2: one of the data input. By editing the source code can be changed to almost any other pin, but preferably in the same PIN block (letter) as the other data, for simultaneous read.


The adapter should be ready to use (either if the wheel is connected or not) and can be configured via the serial port (you can send command via letters, send 'h' for a list of them).

Some features are:
- Send '+' and '-' to increase/decrease polling time (milliseconds to wait between loops). It's 10ms by default (the original was 100ms, but it felt choppy).
- Send '?' to toggle debug mode (the received data is displayed to quickly test which bits changed between commands). Disabled by default.
- Send '*' to toggle feedback test mode. When enabled pressing each button performs a force feedback effect, mimicking the original driver. Remember to connect the wheel with the power adapter!. You can also press Set+Force and Set+O buttons in the wheel to enable/disable without using serial commands. Disabled by default.
- Send 'z' to toggle joined axis mode. When enabled the accelerator and break pedals (if available) are sent as axis Y (positive if only the accelerator is pressed, negative if only the break, the sum if both). This was how the driver originally displayed them. If disabled the accelerator is sent as axis Y and the break as axis Z. Enabled by default.
- Send 'k' to toggle mouse+keyboard mode. This was sort of an easter egg (due to how easy is to program) to be able to control the mouse by turning the wheel, and send some keyboard keys by pressing buttons. Which button correspond to which key can be seen in the code (line 254). Can be enabled/disabled by pressing Set+leftTrigger and Set+rightTrigger respectively.
- Send 'm' and 's' to toggle main and secondary command polling. Useful when testing other commands (so nothing is sent automatically). Remember that when the main polling is disabled the joystick won't work (because buttons/axes won't be detected). Main is enabled by default, secondary is disabled by default (does nothing).
- Send any hexadecimal string and then enter to send it as a command via uart to the wheel. This was mainly used to discover extra commands and the significance of the parameters (for example send 'A501' then enter to receive a mysterious ID).
- Other chars are ignored (so you can also send 'A5 01' in the previous example).


**Important!**
For some reason the teensy interruptions are not fast enough to read the input on the clock raising edge. The delay was measured to be half the clock period approximately. This meant that signals were read on the falling edge instead. This introduced a lot of 'ghost' inputs due to incorrect reading. Faster interruptions were tried but impossible to develop, so instead the software is configured to interrupt on the falling edge, which makes it read on the rising without any mismatch. This has the disadvantage that the first 3 bits are lost (unless the response contains only 3 bits, because the signals are kept so they can be read later).

### Packed adapter

For academic purposes (and because it looks better) a PCB + 'casing' was created. The related files are in the `3DObject` folder. The PCB layout was developed with [EasyEda](https://easyeda.com/) and the casing using windows [3D Builder](https://www.microsoft.com/en-us/p/3d-builder/9wzdncrfj3t6) (I tried blender but...buff).

## Future work

A working adapter was developed to be able to use the wheel as a joystick, but there were some aspects that could be improved:

- Fix the interrupts. As explained above, for some reason the teensy interruptions are not fast enough to be able to read the data on the clock rising edge. The solution found was to delay the read, which misses the first 3 bits. Is it possible to make them even faster?

- From the original driver software some commands were recorded, but they are not all the protocol is able to understand. By trial and error other commands were discovered, but specially the force feedback commands require a lot of parameters which are hard to understand by trying. Being able to play some older software, specially games, that uses force feedback to send different commands could help in having more real examples. Note: some games were tried, and 3 more commands were discovered with Revolt, unfortunately in the process the driver stopped working (supposedly by installing an invalid direct x version) and could not be restored.

- In order to use the force feedback from modern games a custom HID descriptor is needed for the teensy. The default joystick library don't provide this functionality, and even though other libraries were tried it was not possible to use a custom HID descriptor to have force feedback capabilities.

## Resources

The following links were bookmarked due to its importance (even if they were not used).
- https://www.pjrc.com/store/teensy.html
- https://github.com/tloimu/adapt-ffb-joy
- https://github.com/YukMingLaw/ArduinoJoystickWithFFBLibrary
- https://forum.pjrc.com/threads/33246-USB-Joystick-Rumble-Force-Feedback-on-Teensy-3-1
- https://blog.hamaluik.ca/posts/making-a-custom-teensy3-hid-joystick/

## Contact

Even though I'm no longer involved in this project, don't hesitate to use it for other joysticks or to improve this existing one (if possible mention me). Also, if you need some help, feel free to open an issue or send me an email (keep in mind that depending on when you send it I may have forget the details and won't be able to answer, but you can try anyway!)
