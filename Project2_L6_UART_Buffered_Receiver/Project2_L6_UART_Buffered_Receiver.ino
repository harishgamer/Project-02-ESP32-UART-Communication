
#include <Arduino.h>
#include <string.h>

// UART2 for communication between ESP32 boards
HardwareSerial MyUART(2);

constexpr int UART_RX = 16;
constexpr int UART_TX = 17;

constexpr size_t BUFFER_SIZE = 128;

char rxBuffer[BUFFER_SIZE];
size_t rxIndex = 0;

unsigned long receivedCount = 0;
unsigned long errorCount = 0;

void processReceivedLine(const char *message)
{
  Serial.print("[UART RX] ");
  Serial.println(message);

  if (strncmp(message, "MSG|", 4) == 0)
  {
    receivedCount++;

    Serial.println("[STATUS] Valid message received");

    // Send acknowledgement back to ESP32 #1.
    MyUART.print("ACK|");
    MyUART.println(receivedCount);

    Serial.print("[UART TX] ACK|");
    Serial.println(receivedCount);
  }
  else
  {
    errorCount++;
    Serial.println("[ERROR] Unknown message format");
  }

  Serial.print("[STATS] Messages received: ");
  Serial.println(receivedCount);

  Serial.print("[STATS] Errors: ");
  Serial.println(errorCount);

  Serial.println("------------------------------");
}

void receiveUART()
{
  // Process bytes already available in the UART receive buffer.
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
        errorCount++;

        Serial.println("[ERROR] RX buffer overflow");

        // Discard the oversized message and start again.
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
  Serial.println("ESP32 #2 - UART Receiver");
  Serial.println("Level 6: Buffered UART");
  Serial.println("================================");
}

void loop()
{
  // Continuously process received bytes without waiting for a line.
  receiveUART();
}
