# Project 2 - ESP32 UART Communication

Hi, this is my **Project 2** using two ESP32 boards. I started with sending a simple message from one board to another, and then improved it level by level.

I wanted to understand not only how UART works, but also what happens when messages are incomplete, data gets corrupted, an acknowledgement is missed, or the receiver needs to process data without blocking. In the final level, I used FreeRTOS concepts.

I tested the levels using Arduino IDE and checked the output in the Serial Monitor. I have written down the problems I faced and the checks that helped me debug them, so I can refer to them later instead of repeating the same mistakes.

## What I used

- 2 ESP32 development boards (ESP32-WROOM-32)
- Jumper wires
- USB cables (a data-capable cable is needed for uploading and Serial Monitor)
- Arduino IDE 2.3.10
- ESP32 board package by Espressif Systems
- Board selected in Arduino IDE: **ESP32 Dev Module**

## Wiring between the two boards

I used UART2 on both ESP32 boards. The code uses GPIO17 as TX and GPIO16 as RX.

| Transmitter board (ESP32 #1) | Receiver board (ESP32 #2) |
|---|---|
| GPIO17 (TX) | GPIO16 (RX) |
| GPIO16 (RX) | GPIO17 (TX) |
| GND | GND |

**Important things I learned:**

- TX must connect to the other board's RX. Connecting TX to TX will not work.
- Both boards need a common GND so they share the same voltage reference.
- Each board can be powered through its own USB cable. Do not connect the 5 V pins together.
- I used GPIO16 and GPIO17 in the code, so I should not assume the board's USB serial pins are being used for this UART connection.
- ESP32 UART signals use 3.3 V logic.

## Levels I completed

### Level 1 - Basic UART

I sent a message from ESP32 #1 and checked that ESP32 #2 received it.

Example message:

`Hello from ESP32 #1`

### Level 2 - Bidirectional UART

I made the boards communicate in both directions. The receiver sends an acknowledgement back, so the transmitter can see that the other board responded.

### Level 3 - Message framing

I added a simple frame format so the receiver can identify the start and end of a message instead of treating every incoming byte as a complete message.

Example format used in the project:

`<MSG:Hello from ESP32 #1>`

The exact frame format depends on the sketch for that level.

### Level 4 - CRC-8 error detection

I added CRC-8 checking. The receiver calculates a CRC from the message and compares it with the CRC sent in the frame.

In my test, the receiver showed matching values such as:

```text
RECEIVED CRC: 71
CALCULATED CRC: 71
CRC CHECK: PASS
MESSAGE ACCEPTED
```

A matching CRC is evidence that the received payload passed this check. It is not a guarantee that every possible error will be detected.

### Level 5 - Timeout and retry

I added a timeout for waiting for an acknowledgement. If the transmitter does not receive a valid ACK before the timeout, it retries a limited number of times and then reports a failure.

I observed both successful delivery and a timeout. A timeout does not automatically mean the UART wiring is broken: the message may have reached the receiver while the ACK was delayed, missed, or not recognised by the transmitter.

### Level 6 - Buffered UART

I changed the receiver to process incoming bytes using a buffer rather than waiting for a whole line before doing anything. I also displayed message and error counters.

This level helped me understand why a receiver should keep processing incoming data and should handle buffer limits carefully.

### Level 7 - FreeRTOS

I used FreeRTOS concepts in the UART transmitter and receiver sketches, including tasks and a queue for passing messages between parts of the program. The receiver also displayed acknowledgements and diagnostic counters.

My captured output showed messages being accepted, `ERRORS=0`, and `RX_OVERFLOW=0` during that test. This confirms the observed run; it does not mean every possible stress or fault case has been tested.

## Problems I faced and how I checked them

I am keeping this section because these were useful debugging lessons while building the project.

### 1. The transmitter printed a message, but the receiver showed `No UART data received`

**What I saw:** The transmitter's Serial Monitor showed its send message, but the receiver did not display the incoming UART message.

**What this meant:** A `TX:` message printed using `Serial.println()` only confirms that the transmitter sketch reached that print statement. It does not prove that the second ESP32 received the data.

**Checks to do:**

1. Check the wire from transmitter GPIO17 (TX) to receiver GPIO16 (RX).
2. Check that the GND pins of the two boards are connected.
3. Check that both sketches use the same baud rate and the intended UART pins.
4. Check that the receiver sketch is uploaded to the receiver board and is running.
5. Open the Serial Monitor for the receiver's own USB COM port.

In my setup, the message was successfully received after I had the transmitter and receiver sketches running on the correct boards with the UART connections in place. The important lesson was to distinguish the USB Serial Monitor output from the separate UART2 link between the ESP32 boards.

### 2. The loopback test kept showing `No UART data received`

A loopback test sends data from a UART TX pin and connects that TX pin back to the same board's RX pin.

For the test using UART2 on GPIO17 and GPIO16:

- Disconnect the inter-board UART wires for the loopback test.
- Connect **GPIO17 to GPIO16 on the same ESP32**.
- Upload the loopback diagnostic sketch to that board.
- Open its USB Serial Monitor at the baud rate used by `Serial.begin()`.
- Confirm the sketch uses `MyUART.begin(115200, SERIAL_8N1, 16, 17)`.

The argument order is RX first, then TX. So `16, 17` means RX = GPIO16 and TX = GPIO17.

If loopback still fails, check that the jumper really connects GPIO17 to GPIO16, that the correct board and sketch are selected, and that no other wiring is interfering. A successful loopback test checks one board's UART pins and configuration; it does not by itself prove that two-board wiring is correct.

### 3. The transmitter showed data, but there was confusion about COM ports and USB cables

The USB connection to a board is separate from the UART2 connection between the boards.

- A **data-capable USB cable** lets the computer upload a sketch and open the board's USB serial port.
- A **charge-only cable** can power the board but cannot provide a COM port for uploading or Serial Monitor.
- If both boards need their own Serial Monitor windows at the same time, both need a data-capable USB connection to the computer.
- For a one-way UART test, the transmitter can be powered separately while the receiver is connected to the computer, as long as the UART wires and common GND are connected correctly.

I tested the boards individually and found that the white cable provided a COM port, while the black cable powered the board but did not provide a COM port. This pointed to a cable limitation rather than a UART-code problem.

### 4. Arduino IDE showed an upload error after flashing

An upload message such as `No more data to read from serial port` can occur even when the sketch was written, so check the actual result before assuming the code failed.

**What to do:**

1. Wait for the upload process to finish.
2. Check whether the board restarts and runs the new sketch.
3. Close the Serial Monitor if it is holding the port, then upload again if necessary.
4. Verify the selected board is **ESP32 Dev Module** and the selected COM port belongs to the board connected by the data cable.
5. If upload continues to fail, check the USB cable, USB-to-UART driver, and board connection.

In my Level 2 test, the upload tool showed an error, but the Serial Monitors subsequently showed messages and acknowledgements going in both directions. I therefore checked runtime behaviour instead of treating the upload message alone as proof that the project was not working.

### 5. CRC showed PASS

A `CRC CHECK: PASS` message means the received CRC matched the CRC calculated by that receiver using the implemented algorithm and payload.

If the check fails:

1. Confirm that transmitter and receiver use the same CRC algorithm, polynomial, initial value, and byte range.
2. Confirm that the receiver extracts the payload and received CRC from the frame correctly.
3. Check that the message was not truncated or parsed incorrectly.
4. Test with a known message and compare the sent and calculated CRC values.

CRC is an error-detection mechanism; it is not encryption or authentication.

### 6. The transmitter reported a timeout even though the receiver displayed the message

This happened during testing. The receiver could display a valid message while the transmitter still timed out waiting for the acknowledgement.

Possible checks:

1. Confirm the reverse connection is also present: receiver GPIO17 (TX) to transmitter GPIO16 (RX).
2. Check that the receiver actually sends an ACK after processing the message.
3. Confirm that the ACK text and frame format match what the transmitter is looking for.
4. Check whether the timeout is too short for the current processing and Serial Monitor output.
5. Check whether a previous or extra line is being read as the response.

A received message and a received acknowledgement are two different events. The transmitter should only report success when it recognises the expected ACK.

### 7. Level 6/7 buffer and overflow counters

A receive buffer has a limited size. If data arrives faster than the application processes it, the buffer can fill up.

Things to check when investigating buffer problems:

- Whether the code checks the buffer limit before writing another byte.
- Whether the receiver processes incoming bytes often enough.
- Whether long delays or slow printing are preventing timely processing.
- Whether counters are updated consistently when data is dropped or rejected.

`RX_OVERFLOW=0` means no overflow was counted in the displayed test. It does not prove that overflow can never happen under heavier traffic.

## How to test the project again

1. Check the wiring table above with the boards powered off.
2. Connect each board to a suitable USB power/data source.
3. Upload the matching transmitter and receiver sketches for the level being tested.
4. Open the Serial Monitor for each connected board at **115200 baud**.
5. Start with Level 1 before debugging a more complex level.
6. For Level 4 and above, check the frame, CRC, ACK, timeout, and diagnostic messages—not only the transmitter's `TX` line.
7. Change one thing at a time when debugging, so it is clear which change fixed the issue.

## What I learned

- How UART TX/RX wiring and common ground work between two ESP32 boards.
- Why USB Serial output is not the same as communication over UART2.
- How to frame messages and detect certain transmission errors with CRC.
- How acknowledgement, timeout, and retry logic can make communication more robust.
- Why buffered reception and careful buffer-limit checks matter.
- How FreeRTOS tasks and queues can help separate communication work.
- How to use Serial Monitor logs to narrow down hardware, configuration, and software problems.

## Next project

My next step is to work with I²C and the MPU6050 sensor, then continue with SPI display and CAN communication projects.

---
**Project:** ESP32 UART Communication  
**Author:** Harish
