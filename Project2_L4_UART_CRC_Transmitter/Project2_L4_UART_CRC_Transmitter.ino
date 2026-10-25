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

  Serial.println("ESP32 #1 - UART CRC Transmitter Started");
}

void loop()
{
  String payload = "Hello from ESP32 #1";

  uint8_t crc = calculateCRC8(payload);

  // Send message and CRC in one framed line.
  String frame = "MSG|" + payload + "|" + formatCRC(crc);

  MyUART.println(frame);

  Serial.print("TX FRAME: ");
  Serial.println(frame);

  // Wait for the receiver's response.
  unsigned long startTime = millis();
  bool responseReceived = false;

  while (millis() - startTime < 1000)
  {
    if (MyUART.available() > 0)
    {
      String response = MyUART.readStringUntil('\n');
      response.trim();

      Serial.print("RX RESPONSE: ");
      Serial.println(response);

      responseReceived = true;
      break;
    }

    delay(1);
  }

  if (!responseReceived)
  {
    Serial.println("ERROR: ACK timeout");
  }

  Serial.println("-------------------------");

  delay(1000);
}