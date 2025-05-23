#define SERIAL_RX_BUFFER_SIZE 255

#include "Arduino.h"
#include <inttypes.h>
#define U8G2_USE_LARGE_FONTS
#include <U8g2lib.h>
#include <SPI.h>
#include "BluetoothSerial.h"
#include <RH_RF95.h>
#include "logo.xbm"

// Configs
#define DEBUG_SERIAL Serial

// Bluetooth
#define BT_NAME		"EKI021"

// LoRa config
#define LORA_LEN    255
#define SERIAL_LEN  LORA_LEN + 8 // 255 max LoRa PKT + start + end
#define LORA_FREQ   868.7

// GPIOs
#define LED     25

// OLED GPIOs
// Depending on boards can be CLK 5 or 15 and RST 23 or 16
// Depending on boards can be CLK 22 or 21 and RST 23
// for Heltec Lora, CLK DAT RST = 15, 4, 16
// for TTGO Lora32 V1.6, CLK DAT RST = 22, 21, 23
#define OLED_CLK    15
#define OLED_DAT    4
#define OLED_RST    16

// LoRa GPIOs
#define LORA_SS     18
#define LORA_INT    26

// INTERFACE
#define LINE8_0 8+1
#define LINE8_1 16+1
#define LINE8_2 24+1
#define LINE8_3 32+1
#define LINE8_4 40+1
#define LINE8_5 48+1
#define LINE8_6 56+1
#define LINE8_7 64+1

#define FONT_7t u8g2_font_artossans8_8r
#define FONT_16n u8g2_font_logisoso16_tn


// Objects
// U8G2_SSD1306_128X64_NONAME_F_HW_I2C (rotation, reset, clock, date)?
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, OLED_RST, OLED_CLK,  OLED_DAT);
RH_RF95 rf95(LORA_SS, LORA_INT);
BluetoothSerial SerialBT;


// variables
uint8_t pkt_start[4] = {0xAA, 0x55, 0xAA, 0x55};
uint8_t pkt_end[4] = {0x33, 0xCC, 0x33, 0xCC};

bool telem_detected;
bool ssdv_detected;


// lora receive buffer
uint8_t lora_buf[LORA_LEN];
uint8_t lora_len = sizeof(lora_buf);
// Serial buffer
uint8_t serial_buf[SERIAL_LEN];

// telemetry buffer (wifi)
uint8_t last_telem_buf[LORA_LEN];
uint8_t telem_len = 0;

// packet counter
uint16_t pkt_counter = 0;

void setup() {

    // initialize LED digital pin as an output.
    pinMode(LED, OUTPUT);

    // prepare telemetry
    memset(last_telem_buf, 0, sizeof(last_telem_buf));
    memcpy(last_telem_buf, "<li>NO TELEMETRY YET</li>", 25);
    telem_len = 25;

    // init SPI (ESP32)
    pinMode(18, OUTPUT);
    SPI.begin(5, 19, 27, 18);
    SPI.setFrequency(40000000);

    // init Display and show splash screen
    u8g2.begin();
    u8g2.drawXBM((128-logo_width)/2, (64-logo_height)/2, logo_width, logo_height, logo_bits);
    u8g2.sendBuffer();
    delay(3000);

    // simple interface
    u8g2.clearBuffer();
    u8g2.setFont(FONT_7t);
    u8g2.drawStr(2, LINE8_0, "EKI Receiver");

    u8g2.drawStr(2, LINE8_6, "Receiving...");
    u8g2.sendBuffer();

    // Serial port 
    DEBUG_SERIAL.begin(115200);
    //DEBUG_SERIAL.buffer(255);

    // Init LoRa
    if (!rf95.init()) {
        DEBUG_SERIAL.println("#RF95 Init failed");
        u8g2.drawStr(2, LINE8_1,"LoRa not found");
        u8g2.sendBuffer();
    } else        {
        DEBUG_SERIAL.println("#RF95 Init OK");  
        u8g2.drawStr(2, LINE8_1,"LoRa OK");
        u8g2.sendBuffer();
    }
    
    // Init BT
    SerialBT.begin(BT_NAME); //Bluetooth device name
    u8g2.drawStr(2, LINE8_2, "BT OK:");
    u8g2.drawStr(54, LINE8_2, BT_NAME);
    u8g2.sendBuffer();

    // receiver, low power  
    rf95.setFrequency(LORA_FREQ);
    rf95.setTxPower(5,false);
    memset(lora_buf, 0, sizeof(lora_buf));
    memset(serial_buf, 0, sizeof(serial_buf));

}

void loop() {
        lora_len = sizeof(lora_buf);
        // Should be a message for us now   
        if (rf95.recv(lora_buf, &lora_len)) {
            // packet received
            digitalWrite(LED, HIGH);
            telem_detected = false;
            ssdv_detected = false;

            // check for telemetry
            if (lora_buf[0] == '$' && lora_buf[1] == '$') {
                // clear
                memset(last_telem_buf, 0, sizeof(last_telem_buf));
                memcpy(last_telem_buf, lora_buf, lora_len);
                telem_len = lora_len;
                telem_detected = true;
            }

            // check for ssdv
            if (lora_buf[0] == 0x66 || lora_buf[0] == 0x67 ) {
                ssdv_detected = true;
            }

            // create serial packet
            memcpy(serial_buf, pkt_start, 4);
            memcpy(serial_buf+4, lora_buf, lora_len);
            memcpy(serial_buf+4+lora_len, pkt_end, 4);

            // send it through serial port (only if recognized)
            if (telem_detected || ssdv_detected) {
                DEBUG_SERIAL.write(serial_buf, lora_len+8);
            
                // clear buffer
                memset(serial_buf, 0, sizeof(serial_buf));

                // send BT
                SerialBT.print((char*)last_telem_buf);
            }
            digitalWrite(LED, LOW);
        }
        // } else {
        //     // failed packet?
        //     DEBUG_SERIAL.println("#Recv failed");
        // }
    

}

