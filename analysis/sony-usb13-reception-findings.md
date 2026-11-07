# USB13: shared CS1 boundaries recovered, SPI2 bytes still absent

`captures/sony-usb13.csv` contains 291 records from each bus, sequences 160–450.
Every SPI1 record has 16 bytes and flags 00. Every SPI2 record is rejected for
length (flags 02):

| SPI2 received bytes | Records |
|---|---:|
| 0 | 95 |
| 1 | 131 |
| 2 | 51 |
| 3 | 11 |
| 4 | 3 |

All received SPI2 bytes are zero. No fifteen-byte text packet is present.
The first statistics record is at 5949 ms: SPI1 valid=159, SPI2 valid=0,
SPI2 rejected=160. The last is at 14967 ms: SPI1 valid=442, SPI2 valid=0,
SPI2 rejected=443. Capture and USB queue-drop counters remain zero.

Compared with USB12, the thousands of empty SPI2 select interrupts per second
are gone. Both buses now have the same CS1-driven sequence and frame rate.
However, matching boundary counts do not prove reception of SPI2 clocks/data.
The new trace cannot supply a date, title, or text command to the decoder.

The configured bypass uses PA4/CS1 for both frame ends, PB13/SCK2 and PB14/SDI2;
PB12 is disconnected and analog. SPI2 is a software-selected receive-only slave
(SSM=1, SSI=0, BIDIMODE=1, BIDIOE=0). ST's STM32F411 reference manual RM0383
specifies that SSM replaces the NSS input with SSI:
https://www.st.com/resource/en/reference_manual/rm0383-stm32f411xce-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

The trace alone cannot distinguish an absent/incorrect SCK2 connection, a low or
incorrect data connection, peripheral/pin damage, or an unobserved receiver-state
problem. Do not call this successful reception or invent date data. Clarify
whether the damaged connection is on the STM32 or Sony side, and verify PB13 and
PB14 wiring before selecting a hardware workaround or further receiver changes.
