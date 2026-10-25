#include <Arduino.h>

HardwareSerial MyUART(2);

constexpr int UART_RX = 16;
constexpr int UART_TX = 17;

void setup()
{
    Serial.begin(115200);

    MyUART.begin(115200, SERIAL_8N1, UART_RX, UART_TX);

    Serial.println("ESP32 #2 - Bidirectional UART Started");
}

void loop()
{
    if (MyUART.available() > 0)
    {
        String message = MyUART.readStringUntil('\n');

        Serial.print("RX: ");
        Serial.println(message);

        // Send a response to ESP32 #1
        MyUART.println("Acknowledged by ESP32 #2");

        Serial.println("TX: Acknowledged by ESP32 #2");
    }
}
