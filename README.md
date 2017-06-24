# Building blocks for wired sensor network

## Background and motivation

I have developed a lot of IoT prototypes both at work and as my hobby, then I have observed that most of my IoT projects require open wired sensor networking technologies that satisfiy the requirements below:

- low power consumption
- bus topology rather than hub and spoke
- cheap and open
- compact and small footprint

There are a lot of such technologies for in-vehicle network, buidling management or factory automation, but none of them satisfies the requirements above.

This is a project to develop a light-weight protocol and building-blocks for wired sensor network.

![compact2](./doc/compact2.png)

Plug&Play protocol are supported for a master board to recognize capabilities of all its slaves in a plug&play manner.

## Interfaces among building blocks

All the blocks developed in this project support [Plug&Play protocol](./doc/PROTOCOL.md) that runs on UART.

```
                    USB hub
                     +---+
[block A]--UART/USB--|   |
[block B]--UART/USB--|   |--[mini PLC]
[block C]--UART/USB--|   |
                     +---+
                     
               hub&spoke topology
```

It also runs on I2C: [I2C backplane specification](./doc/I2C_BACKPLANE_SPEC.pptx).

```
        <- - - - I2C backplane - - - ->
[block A]---[block B]---[block C]---[Scheduler]--UART/USB--[mini PLC]

            bus topology (daisy-chain)
```

## 8bit MCU

I have concluded that [PIC16F1829](http://ww1.microchip.com/downloads/en/DeviceDoc/41440A.pdf) is the best choice for this project.

![pic16f1](./doc/starting_project.png)

## Base board prototype

![prototype3](./doc/prototype3.png)

![pico2](https://docs.google.com/drawings/d/1_WCC4vuPbIT2im9c337ibk5xEq9WKzrT9907IOWTCCA/pub?w=680&h=400)

##### Construct examples

One I2C master and three I2C slaves are connected with each other via backplane bus:

![compact](./doc/compact.png)

A similar construct to the above, but all the boards are connected with each other via daisy-chain:

![daisy_chain](./doc/daisy_chain.png)

To extend the distance of bus signal reachability, use CAN standalone controller (SPI): [MCP2525](http://ww1.microchip.com/downloads/en/DeviceDoc/21801e.pdf).

## Implementation

Note: I use [MPLAB Code Configurator (MCC)](http://www.microchip.com/mplab/mplab-code-configurator) to generate code for USART, I2C, PWM, Timer etc.

#### Plug&Play protocol

- [Plug&play protocol specification](./doc/PROTOCOL.md)
- [Implementation](./mini_plc/lib/protocol.X)

Including it as a library:
- [Step1: include the protocol library directory](./doc/mcc_eusart4.png)
- [Step2: include the protocol library in your project](./doc/mcc_eusart3.png)
- [Step3: exclude mcc generated eusart libraries from your project](./doc/mcc_eusart2.png)
- [Step4: enable eusart interrupts](./doc/mcc_eusart.png)

#### Blocks

- [5V: Character LCD actuator block (AQM1602XA-RN-GBW)](./mini_plc/i2c_slave_lcd.X), [pin assignment](./doc/lcd_pin.png)
- [5V: Acceleration sensor block （KXR94-2050)](./mini_plc/i2c_slave_accel.X), [pin assignment](./doc/acceleration_pin.png)
- [5V: Speed sensor block (A1324LUA-T)](./mini_plc/i2c_slave_speed.X), [pin assignment](./doc/rotation_pin.png)
- [5V: Temperature and humidity sensor block (HDC1000)](./mini_plc/i2c_temp.X)
