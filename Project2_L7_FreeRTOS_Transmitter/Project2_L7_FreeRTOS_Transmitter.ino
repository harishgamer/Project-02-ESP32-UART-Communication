#include <Arduino.h>
#include <string.h>

HardwareSerial MyUART(2);

constexpr int UART_RX = 16;
constexpr int UART_TX = 17;

constexpr size_t BUFFER_SIZE = 128;
constexpr size_t QUEUE_LENGTH = 10;

struct Message
{
  char text[BUFFER_SIZE];
};

QueueHandle_t rxQueue;

volatile uint32_t txCount = 0;
volatile uint32_t ackCount = 0;
volatile uint32_t rxOverflowCount = 0;

SemaphoreHandle_t serialMutex;

void safePrint(const char *text)
{
  if (xSemaphoreTake(serialMutex, portMAX_DELAY) == pdTRUE)
  {
    Serial.print(text);
    xSemaphoreGive(serialMutex);
  }
}

// Task 1: Transmit numbered messages periodically.
void transmitTask(void *parameter)
{
  uint32_t messageNumber = 0;
  char message[BUFFER_SIZE];

  for (;;)
  {
    messageNumber++;

    snprintf(
      message,
      sizeof(message),
      "MSG|Hello from ESP32 #1|%lu",
      static_cast<unsigned long>(messageNumber)
    );

    MyUART.println(message);
    txCount++;

    char logMessage[BUFFER_SIZE + 32];

    snprintf(
      logMessage,
      sizeof(logMessage),
      "[TX] %s\n",
      message
    );

    safePrint(logMessage);

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

// Task 2: Receive UART bytes and queue complete lines.
void receiveTask(void *parameter)
{
  char buffer[BUFFER_SIZE];
  size_t index = 0;

  for (;;)
  {
    while (MyUART.available() > 0)
    {
      char c = static_cast<char>(MyUART.read());

      if (c == '\n')
      {
        buffer[index] = '\0';

        if (index > 0)
        {
          Message message = {};
          strncpy(message.text, buffer, BUFFER_SIZE - 1);

          if (xQueueSend(rxQueue, &message, 0) != pdPASS)
          {
            safePrint("[ERROR] RX queue full; ACK discarded\n");
          }
        }

        index = 0;
      }
      else if (c != '\r')
      {
        if (index < BUFFER_SIZE - 1)
        {
          buffer[index++] = c;
        }
        else
        {
          rxOverflowCount++;
          index = 0;
          safePrint("[ERROR] RX buffer overflow\n");
        }
      }
    }

    // Yield so other FreeRTOS tasks can execute.
    vTaskDelay(pdMS_TO_TICKS(2));
  }
}

// Task 3: Process acknowledgements from the queue.
void processTask(void *parameter)
{
  Message message;

  for (;;)
  {
    if (xQueueReceive(rxQueue, &message, portMAX_DELAY) == pdPASS)
    {
      char logMessage[BUFFER_SIZE + 32];

      snprintf(
        logMessage,
        sizeof(logMessage),
        "[RX ACK] %s\n",
        message.text
      );

      safePrint(logMessage);

      if (strncmp(message.text, "ACK|", 4) == 0)
      {
        ackCount++;
        safePrint("[STATUS] Acknowledgement received\n");
      }
    }
  }
}

// Task 4: Report system counters periodically.
void diagnosticsTask(void *parameter)
{
  for (;;)
  {
    char message[160];

    snprintf(
      message,
      sizeof(message),
      "[DIAGNOSTICS] TX=%lu ACK=%lu RX_OVERFLOW=%lu\n",
      static_cast<unsigned long>(txCount),
      static_cast<unsigned long>(ackCount),
      static_cast<unsigned long>(rxOverflowCount)
    );

    safePrint(message);

    vTaskDelay(pdMS_TO_TICKS(10000));
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

  serialMutex = xSemaphoreCreateMutex();
  rxQueue = xQueueCreate(QUEUE_LENGTH, sizeof(Message));

  if (serialMutex == nullptr || rxQueue == nullptr)
  {
    Serial.println("[FATAL] Failed to create FreeRTOS resources");

    while (true)
    {
      delay(1000);
    }
  }

  Serial.println("ESP32 #1 - FreeRTOS UART Transmitter");

  BaseType_t result1 = xTaskCreate(
    transmitTask, "UART_TX", 4096, nullptr, 2, nullptr
  );

  BaseType_t result2 = xTaskCreate(
    receiveTask, "UART_RX", 4096, nullptr, 3, nullptr
  );

  BaseType_t result3 = xTaskCreate(
    processTask, "PROCESS", 4096, nullptr, 2, nullptr
  );

  BaseType_t result4 = xTaskCreate(
    diagnosticsTask, "DIAGNOSTICS", 3072, nullptr, 1, nullptr
  );

  if (result1 != pdPASS || result2 != pdPASS ||
      result3 != pdPASS || result4 != pdPASS)
  {
    Serial.println("[FATAL] Failed to create one or more tasks");
  }
}

void loop()
{
  // FreeRTOS tasks perform the application work.
  vTaskDelay(pdMS_TO_TICKS(1000));
}