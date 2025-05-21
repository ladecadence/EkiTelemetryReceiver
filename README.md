# EKI Telemetry Receiver

Receiver code for the EKI LoRa-based telemetry and SSDV packets. Designed to run on ESP32+LoRa+OLED boards like the Heltec ones.

Works with the EkiTelemetry desktop software (using the USB serial port) and also creates a Bluetooth-Serial device to send the telemetry data.

See EkiTelemetry repository for more info: https://github.com/ladecadence/EkiTelemetry 

## Compiling and uploading

Uses platform.io, so just run platformio run -t upload if using it from command line. If you are using the platform.io IDE, just upload it to the board.

You'll need to change the OLED pins depending on your board (heltec or TTGO), and also the Bluetooth device name on the #define section at the top of the main.c file.




