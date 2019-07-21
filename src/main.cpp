#include <Arduino.h>
#include <FastLED.h>
#include <Ticker.h>
#include "ESP8266WiFi.h"

// mathemagical values
// To play with these, see the accompanying Processing sketch, lantern_toy
// TIME_PERIOD defines the base frequency of the flickering. Larger values (in ms)
// define a longer base frequency.
// FLICKER_RATIO defines the amount of the light brightness that can be dedicated
// to flickering (when there is no flicker, the flicker function returns 1.0)
#define TIME_PERIOD 666.0
#define FLICKER_RATIO 0.16
// This choses the divisor within the logrithmic sine wave.
// A lower value will cause a higher frequency pertubation when more networks
// are around (2 causes bad flickering around 20 networks or so)
// A higher value will cause smaller pertubations when more networks are around
// but is generally calmer when fewer (10ish) networks are around.
#define FLICKER_DIVISOR 16

#define NEOPIXEL_PIN 14
#define NUM_LEDS 12
#define FRAMERATE 30

#define FRAME_TIME (int)( 1000/(float)(FRAMERATE) )

CRGB leds[NUM_LEDS];

uint8_t seen_ssids = 0;        // Number of SSIDs that have been seen in the last scan
int32_t average_rssi = -100;     // Average RSSI of the last scan.

// Pallette from colorbrewer "Spectral" palette:
// from ColorBrewer2.org -- A small spectral-ish color palette that
// almost approxomates the color spectrum. 
// We use the 9-value spectrum (since 0 is a value we use)
// this will be mapped onto the "heat" (rssi) of the networks we see. 
DEFINE_GRADIENT_PALETTE(CB_SPECTRAL) {
0, 213,62,79,
31, 244,109,67,
63, 253,174,97,
95, 254,224,139,
127, 255,255,191,
159, 230,245,152,
191, 171,221,164,
223, 102,194,165,
255, 50,136,189
};

CRGBPalette16 spectral_palette = CB_SPECTRAL;

void update_leds();
void check_scan_wifi();
void print_current_state();

// These timer-tickers kee us handling things like scanning and such. 
Ticker led_timer(update_leds, FRAME_TIME, 0, MILLIS);
Ticker wifi_scan_timer(check_scan_wifi, 30000, 0, MILLIS);
Ticker state_timer(print_current_state, 1000,0,MILLIS);

void setup() {
    Serial.begin(9600);
    Serial.println("Startup: Lantern booting.");
    // initialize the initial variables. 
    Serial.println("Setup LED array");
    FastLED.addLeds<NEOPIXEL,NEOPIXEL_PIN>(leds,NUM_LEDS);
    // this is all for now. 
    Serial.println("Bake some RGB cupcakes.");
    //fill_rainbow(leds,NUM_LEDS,0);
    
    FastLED.setTemperature(Tungsten100W);
    FastLED.setBrightness(255);

    FastLED.show();

    Serial.println("Turn on ESP8266 WiFi");
    // wiFi stuff
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(2000);
    // This makes sure that there's a scan ready when the
    // first roll-over of the wifi scan timer kicks. 
    Serial.println("Start network scan...");
    WiFi.scanNetworks(true,false);

    Serial.println("Start timers...");
    led_timer.start();
    wifi_scan_timer.start();

//    state_timer.start();
    Serial.println("setup() finished");
}

void loop() { 
    led_timer.update();
    wifi_scan_timer.update();
    state_timer.update();
}

void print_current_state() {
    Serial.printf("[ %d networks rssi %ddBm / scan status %d | FastLED %d fps ]\r",
        seen_ssids,
        average_rssi,
        WiFi.scanComplete(),
        FastLED.getFPS()
    );
}

void check_scan_wifi() {
    //Serial.println("[ SCAN TICK ]");
    int8_t scan_state = WiFi.scanComplete();
    switch (scan_state)
    {
    case -2:
        // Kick off a scan
        
        WiFi.scanNetworks(true,false);
        return;
    case -1:
        // no scan data yet...
        return;
    default:
        // We want to get the average RSSI of the set of networks. 
        // this is a value between 0 and -inf, but we're going to clamp it
        // around -100 or so
        // If for some reason there's a different value we haven't caught in the error
        // set, we should return now. Our scan data will come back later with new
        // data. 
        if(scan_state < 0 ) return; 
        long long rssi_tmp = 0;
        for(int i=0; i<scan_state; i++)
        {
            //Serial.printf("Network %s with strength %idBm \n", WiFi.SSID(i).c_str(), WiFi.RSSI(i) );
            rssi_tmp += WiFi.RSSI(i);
        }
        seen_ssids = scan_state;
        average_rssi = (int)(rssi_tmp / (float)seen_ssids);
        // Remove our scanned data, since it's currently irrelevant.
        WiFi.scanDelete();
        break;
    }
}

/**
 * Flicker: takes n (the number of APs, t (the time base) and o (the angular offset))
 * For a toy version of this, see this Desmos graph: https://www.desmos.com/calculator/8wzbhj5oaa
 */
inline float flicker_fn(int n,float t,float o) {
    return FLICKER_RATIO - ( FLICKER_RATIO * cos( ( log( ( (n-1)/((float)FLICKER_DIVISOR) ) + 1) * t ) + o) );
}

CRGB inline get_rgb_color(int led_num, int32_t rssi, uint8_t num_networks )
{
    // calculate the RGB color for a specific LED given the particular avg. RSSI and network count
    float rssi_ratio = abs(rssi)/100.0;
    float time = millis() / TIME_PERIOD ;
    float angle_offset = exp(led_num) + (TWO_PI/NUM_LEDS);
    float flicker = flicker_fn(num_networks, time, angle_offset);
     //FLICKER_RATIO - ( FLICKER_RATIO * cos( log(((num_networks-1)*0.5)+1) * time + angle_offset) );
    uint8_t color_entry = (int)(  ( ( rssi_ratio - FLICKER_RATIO ) + flicker ) * 255 );
    return ColorFromPalette(spectral_palette, color_entry, 96*flicker);
}

uint8_t rainbow_hue = 0;

void update_leds() {
    if(seen_ssids == 0)
    {
        // an unlikely occourance, but maybe at startup
        for(int i = 0; i < NUM_LEDS; i++)
        {
            //fill_rainbow(leds,NUM_LEDS,(uint8_t)(millis()/10000 & 0xFF));
            leds[i] = CHSV(
                (255/NUM_LEDS)*i +(  ++rainbow_hue),
                128+(128* cos( TWO_PI * (millis()/2500.0 )) ),
                128+32+(96* sin( TWO_PI * (millis()/5000.0 )) )
                );
        }
        
    }
    else {

        for(int i=0;i<NUM_LEDS;i++){
            leds[i] = get_rgb_color(i, average_rssi, seen_ssids);
        }
    }
    FastLED.show();
}