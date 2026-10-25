
#include <Arduino.h>
#include <string.h>

// UART2 for communication between ESP32 boards
HardwareSerial MyUART(2);

constexpr int UART_RX = 16;
constexpr int UART_TX = 17;

constexpr unsigned long SEND_INTERVAL_MS = 2000;
constexpr size_t BUFFER_SIZE = 128;

char rxBuffer[BUFFER_SIZE];
size_t rxIndex = 0;

unsigned long lastSendTime = 0;
unsigned long messageCounter = 0;

void processReceivedLine(const char *message)
{
  Serial.print("[UART RX] ");
  Serial.println(message);

  if (strncmp(message, "ACK|", 4) == 0)
  {
    Serial.println("[STATUS] Receiver acknowledged the message");
  }
}

void receiveUART()
{
  // Read available bytes without waiting for a complete message.
  while (MyUART.available() > 0)
  {
    char c = static_cast<char>(MyUART.read());

    if (c == '\n')
    {
      rxBuffer[rxIndex] = '\0';

      if (rxIndex > 0)
      {
        processReceivedLine(rxBuffer);
      }

      rxIndex = 0;
    }
    else if (c != '\r')
    {
      if (rxIndex < BUFFER_SIZE - 1)
      {
        rxBuffer[rxIndex++] = c;
      }
      else
      {
        Serial.println("[ERROR] RX buffer overflow");
        rxIndex = 0;
      }
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

  delay(500);

  Serial.println();
  Serial.println("================================");
  Serial.println("ESP32 #1 - UART Transmitter");
  Serial.println("Level 6: Buffered UART");
  Serial.println("================================");
}

void loop()
{
  // Always process incoming UART bytes.
  receiveUART();

  // Send a message periodically without blocking the loop.
  unsigned long now = millis();

  if (now - lastSendTime >= SEND_INTERVAL_MS)
  {
    lastSendTime = now;
    messageCounter++;

    char message[BUFFER_SIZE];

    snprintf(
      message,
      sizeof(message),
      "MSG|Hello from ESP32 #1|%lu",
      messageCounter
    );

    MyUART.println(message);

    Serial.print("[UART TX] ");
    Serial.println(message);
  }
}
