# the Wifi Lantern

Find it at DEF CON 27. More deeets to come. 


The TL;DR is that it's an IKEA lantern for kids with an ESP8266 in it. Take
one of [these things](https://www.ikea.com/us/en/catalog/products/10422090/)
from your local Big Blue Break-up Box, smush some [lights]
(https://www.adafruit.com/product/1643) in it and add some [brains]
(https://www.adafruit.com/product/3405), add some [juice](https://www.adafruit.com/product/328).

You should hijack the button on the IKEA lantern. Here's how:

* Trim out the resistor on the button board.
* On the SMD side, remove the diode on the trace directly connected to the switch
* liberally re-use the wires from the board to make the button press the EN line
  on the ESP8266 Huzzah board

Pin 14 on the Huzzah goes to data IN on the neopixel ring. Don't want to use Neopixels?
Awesome, fix it in the FastLED setup. APA102s are a good suggestion. I used the
neopixels since they were cheap and I could get them on Amazon Prime. 

so yeah. More networks, more noise. 

MO NOISE MO PROBLEMS.
