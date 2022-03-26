# ESP8266_PowerMeter
A simple but effective power meter based on INA219 and ESP8266 (Wifi and MQTT)

## What is this?

It's a simple power meter that i've build to find the best place to install a solar panel. Using a small solar panel and a solar LiPo charger controller, i was able to measure how many power I 
can have with different expositions and places.

Data were sent via MQTT to a broker, to be stored and analyzed.

## Scheme

I've just build a quick prototype, using a WeMos D1 mini module, a INA219 power meter and a 4 per 7-segment LED display.

INA219 is connected via I2C bus, using D1 (SDA) and D2 (SCL) pin. 

TM1637 7-segment LED display is connected via D5 (CLK) and D6 (DIO) pin.

![ESP8266_PowerMeter](https://raw.githubusercontent.com/michelep/ESP8266_PowerMeter/master/assets/asset_1.jpg)
