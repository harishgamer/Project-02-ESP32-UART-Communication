#include <Arduino.h>
#include <string.h>

HardwareSerial MyUART(2);

constexpr int UART_RX = 16;
constexpr int UART_TX = 17;

constexpr unsigned long ACK_TIMEOUT_MS = 1000;
constexpr int MAX_ATTEMPTS = 3;

uint8_t calculateCRC8(const char *data)
{
  uint8_t crc = 0x00;

  while (*data != '\0')
  {
    crc ^= static_cast<uint8_t>(*data++);

    for (int bit = 0; bit < 8; bit++)
    {
      if (crc & 0x80)
        crc = (crc << 1) ^ 0x07;
      else
        crc <<= 1;
    }
  }

  return crc;
}

bool readUARTLine(char *buffer, size_t capacity)
{
  static size_t index = 0;

  while (MyUART.available() > 0)
  {
    char c = static_cast<char>(MyUART.read());

    if (c == '\r')
      continue;

    if (c == '\n')
    {
      buffer[index] = '\0';
      index = 0;
      return true;
    }

    if (index < capacity - 1)
    {
      buffer[index++] = c;
    }
    else
    {
      index = 0;
      buffer[0] = '\0';
      Serial.println("[ERROR] Incoming UART line too long");
      return false;
    }
  }

  return false;
}

bool sendMessageWithRetry(const char *payload)
{
  char frame[128];
  char response[64];

  uint8_t crc = calculateCRC8(payload);

  snprintf(frame, sizeof(frame), "MSG|%s|%02X", payload, crc);

  for (int attempt = 1; attempt <= MAX_ATTEMPTS; attempt++)
  {
    // Remove any stale response before a new attempt.
    while (MyUART.available() > 0)
      MyUART.read();

    Serial.printf("[TX] Attempt %d/%d\n", attempt, MAX_ATTEMPTS);
    Serial.printf("[TX] Frame: %s\n", frame);

    MyUART.println(frame);

    unsigned long startTime = millis();
    bool gotResponse = false;

    while (millis() - startTime < ACK_TIMEOUT_MS)
    {
      if (readUARTLine(response, sizeof(response)))
      {
        gotResponse = true;

        Serial.printf("[RX] Response: %s\n", response);

        if (strcmp(response, "ACK|OK") == 0)
        {
          Serial.println("[STATUS] Message delivered successfully");
          return true;
        }

        if (strcmp(response, "ACK|ERROR") == 0)
        {
          Serial.println("[ERROR] Receiver rejected message");
          break;
        }

        // Ignore unrelated responses and continue waiting.
        gotResponse = false;
      }

      delay(1);
    }

    if (!gotResponse)
      Serial.println("[TIMEOUT] No valid ACK received");

    if (attempt < MAX_ATTEMPTS)
    {
      Serial.println("[RETRY] Retrying transmission...");
    }
  }

  Serial.println("[FAULT] Transmission failed after 3 attempts");
  return false;
}

void setup()
{
  Serial.begin(115200);
  MyUART.begin(115200, SERIAL_8N1, UART_RX, UART_TX);

  Serial.println();
  Serial.println("ESP32 #1 - Level 5 UART Fault Handling");
}

void loop()
{
  sendMessageWithRetry("Hello from ESP32 #1");

  Serial.println("------------------------------");
  delay(2000);
}