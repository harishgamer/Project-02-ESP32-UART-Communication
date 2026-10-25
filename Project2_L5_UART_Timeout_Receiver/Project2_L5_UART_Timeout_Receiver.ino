#include <Arduino.h>
#include <string.h>
#include <stdlib.h>

HardwareSerial MyUART(2);

constexpr int UART_RX = 16;
constexpr int UART_TX = 17;

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
      Serial.println("[ERROR] Incoming UART frame too long");
      return false;
    }
  }

  return false;
}

void processFrame(char *frame)
{
  Serial.printf("[RX] Frame: %s\n", frame);

  if (strncmp(frame, "MSG|", 4) != 0)
  {
    Serial.println("[ERROR] Invalid frame header");
    MyUART.println("ACK|ERROR");
    return;
  }

  // Find the last '|' separating payload and CRC.
  char *crcSeparator = strrchr(frame, '|');

  if (crcSeparator == nullptr || crcSeparator <= frame + 4)
  {
    Serial.println("[ERROR] Invalid frame format");
    MyUART.println("ACK|ERROR");
    return;
  }

  // Convert the received CRC from hexadecimal.
  char *crcText = crcSeparator + 1;

  if (strlen(crcText) != 2)
  {
    Serial.println("[ERROR] Invalid CRC field");
    MyUART.println("ACK|ERROR");
    return;
  }

  char *endPointer;
  unsigned long receivedCRC = strtoul(crcText, &endPointer, 16);

  if (*endPointer != '\0' || receivedCRC > 0xFF)
  {
    Serial.println("[ERROR] CRC field is not valid hexadecimal");
    MyUART.println("ACK|ERROR");
    return;
  }

  // Temporarily terminate the frame before the CRC field.
  *crcSeparator = '\0';

  // Skip the "MSG|" header to get the payload.
  const char *payload = frame + 4;

  uint8_t calculatedCRC = calculateCRC8(payload);

  Serial.printf("[RX] Payload: %s\n", payload);
  Serial.printf("[CRC] Received:   %02lX\n", receivedCRC);
  Serial.printf("[CRC] Calculated: %02X\n", calculatedCRC);

  if (calculatedCRC == static_cast<uint8_t>(receivedCRC))
  {
    Serial.println("[CRC] PASS - Message accepted");
    MyUART.println("ACK|OK");
  }
  else
  {
    Serial.println("[CRC] FAIL - Message rejected");
    MyUART.println("ACK|ERROR");
  }
}

void setup()
{
  Serial.begin(115200);
  MyUART.begin(115200, SERIAL_8N1, UART_RX, UART_TX);

  Serial.println();
  Serial.println("ESP32 #2 - Level 5 UART Fault Handling");
}

void loop()
{
  char frame[128];

  if (readUARTLine(frame, sizeof(frame)))
  {
    processFrame(frame);
    Serial.println("------------------------------");
  }
}