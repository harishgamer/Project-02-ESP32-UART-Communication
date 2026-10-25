#include <Arduino.h>

HardwareSerial MyUART(2);

constexpr int UART_RX = 16;
constexpr int UART_TX = 17;

void setup()
{
    Serial.begin(115200);

    MyUART.begin(115200, SERIAL_8N1, UART_RX, UART_TX);

    Serial.println("ESP32 #1 - Bidirectional UART Started");
}

void loop()
{
    // Send a message to ESP32 #2
    MyUART.println("Hello from ESP32 #1");

    Serial.println("TX: Hello from ESP32 #1");

    // Receive a response from ESP32 #2
    unsigned long startTime = millis();

    while (millis() - startTime < 500)
    {
        if (MyUART.available() > 0)
        {
            String message = MyUART.readStringUntil('\n');

            Serial.print("RX: ");
            Serial.println(message);

            break;
        }
    }

    delay(1000);
}
