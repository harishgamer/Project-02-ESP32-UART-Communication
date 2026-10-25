#include <Arduino.h>

HardwareSerial MyUART(2);

constexpr int UART_RX = 16;
constexpr int UART_TX = 17;

constexpr size_t BUFFER_SIZE = 100;

char rxBuffer[BUFFER_SIZE];
size_t rxIndex = 0;
bool receivingFrame = false;

unsigned long lastSendTime = 0;

// Send a framed message
void sendFrame(const char *type, const char *payload)
{
    MyUART.print('<');
    MyUART.print(type);
    MyUART.print(':');
    MyUART.print(payload);
    MyUART.println('>');

    Serial.print("TX: <");
    Serial.print(type);
    Serial.print(':');
    Serial.print(payload);
    Serial.println('>');
}

// Read and validate incoming frames
void receiveFrames()
{
    while (MyUART.available() > 0)
    {
        char c = MyUART.read();

        // Start of a new frame
        if (c == '<')
        {
            receivingFrame = true;
            rxIndex = 0;
            continue;
        }

        // Ignore data outside a frame
        if (!receivingFrame)
        {
            continue;
        }

        // End of the frame
        if (c == '>')
        {
            rxBuffer[rxIndex] = '\0';

            char *separator = strchr(rxBuffer, ':');

            if (separator != nullptr)
            {
                *separator = '\0';

                const char *type = rxBuffer;
                const char *payload = separator + 1;

                if (strcmp(type, "ACK") == 0)
                {
                    Serial.print("RX ACK: ");
                    Serial.println(payload);
                }
                else
                {
                    Serial.println("RX: Unexpected message type");
                }
            }
            else
            {
                Serial.println("RX ERROR: Invalid frame format");
            }

            receivingFrame = false;
            rxIndex = 0;
            continue;
        }

        // Prevent buffer overflow
        if (rxIndex < BUFFER_SIZE - 1)
        {
            rxBuffer[rxIndex++] = c;
        }
        else
        {
            Serial.println("RX ERROR: Frame too long");

            receivingFrame = false;
            rxIndex = 0;
        }
    }
}

void setup()
{
    Serial.begin(115200);

    MyUART.begin(
        115200,
        SERIAL_8N1,
        UART_RX,
        UART_TX
    );

    Serial.println("ESP32 #1 - UART Framing Started");
}

void loop()
{
    // Send one message every second
    if (millis() - lastSendTime >= 1000)
    {
        lastSendTime = millis();

        sendFrame("MSG", "Hello from ESP32 #1");
    }

    // Process incoming acknowledgements
    receiveFrames();
}