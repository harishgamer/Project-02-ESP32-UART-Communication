#include <Arduino.h>
#include <string.h>

HardwareSerial MyUART(2);

constexpr int UART_RX = 16;
constexpr int UART_TX = 17;

constexpr size_t BUFFER_SIZE = 100;

char rxBuffer[BUFFER_SIZE];
size_t rxIndex = 0;
bool receivingFrame = false;

// Send a framed acknowledgement
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

        // Start marker
        if (c == '<')
        {
            receivingFrame = true;
            rxIndex = 0;
            continue;
        }

        // Ignore bytes outside a frame
        if (!receivingFrame)
        {
            continue;
        }

        // End marker
        if (c == '>')
        {
            rxBuffer[rxIndex] = '\0';

            char *separator = strchr(rxBuffer, ':');

            if (separator != nullptr)
            {
                *separator = '\0';

                const char *type = rxBuffer;
                const char *payload = separator + 1;

                if (strcmp(type, "MSG") == 0)
                {
                    Serial.print("RX MSG: ");
                    Serial.println(payload);

                    // Reply only to valid MSG frames
                    sendFrame("ACK", "Message received");
                }
                else
                {
                    Serial.println("RX ERROR: Unexpected message type");
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

    Serial.println("ESP32 #2 - UART Framing Started");
}

void loop()
{
    receiveFrames();
}