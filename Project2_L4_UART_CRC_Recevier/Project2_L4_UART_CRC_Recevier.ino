#include <Arduino.h>

HardwareSerial MyUART(2);

constexpr int UART_RX = 16;
constexpr int UART_TX = 17;

// CRC-8: polynomial 0x07, initial value 0x00
uint8_t calculateCRC8(const String &data)
{
  uint8_t crc = 0x00;

  for (size_t i = 0; i < data.length(); i++)
  {
    crc ^= static_cast<uint8_t>(data[i]);

    for (int bit = 0; bit < 8; bit++)
    {
      if (crc & 0x80)
      {
        crc = (crc << 1) ^ 0x07;
      }
      else
      {
        crc <<= 1;
      }
    }
  }

  return crc;
}

String formatCRC(uint8_t crc)
{
  char buffer[3];
  snprintf(buffer, sizeof(buffer), "%02X", crc);
  return String(buffer);
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

  Serial.println("ESP32 #2 - UART CRC Receiver Started");
}

void loop()
{
  if (MyUART.available() > 0)
  {
    String frame = MyUART.readStringUntil('\n');
    frame.trim();

    Serial.print("RX FRAME: ");
    Serial.println(frame);

    // Expected format: MSG|payload|CRC
    if (!frame.startsWith("MSG|"))
    {
      Serial.println("ERROR: Invalid frame format");
      MyUART.println("ACK|INVALID_FRAME");
      return;
    }

    int separator = frame.lastIndexOf('|');

    if (separator <= 4 || separator == frame.length() - 1)
    {
      Serial.println("ERROR: Missing payload or CRC");
      MyUART.println("ACK|INVALID_FRAME");
      return;
    }

    String payload = frame.substring(4, separator);
    String receivedCRC = frame.substring(separator + 1);

    // CRC must contain exactly two hexadecimal characters.
    if (receivedCRC.length() != 2)
    {
      Serial.println("ERROR: Invalid CRC format");
      MyUART.println("ACK|INVALID_CRC_FORMAT");
      return;
    }

    uint8_t calculatedCRC = calculateCRC8(payload);
    String expectedCRC = formatCRC(calculatedCRC);

    Serial.print("PAYLOAD: ");
    Serial.println(payload);

    Serial.print("RECEIVED CRC: ");
    Serial.println(receivedCRC);

    Serial.print("CALCULATED CRC: ");
    Serial.println(expectedCRC);

    if (receivedCRC.equalsIgnoreCase(expectedCRC))
    {
      Serial.println("CRC CHECK: PASS");
      Serial.println("MESSAGE ACCEPTED");

      MyUART.println("ACK|OK");
    }
    else
    {
      Serial.println("CRC CHECK: FAIL");
      Serial.println("MESSAGE REJECTED");

      MyUART.println("ACK|CRC_ERROR");
    }

    Serial.println("-------------------------");
  }
}