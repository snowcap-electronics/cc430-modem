cc430-modem
===========

cc430 modem software acts as a wireless uart

Features:
* Buffers incoming uart and sends when
  - \n is received
  - buffer (32 bytes) is full
  - no new data has been received in 4 milliseconds

Currently adds also RSSI and LQI information as a separate line for
debugging purposes.

TODO:
* Increase the max message length to ~60 bytes (RF FIFO len is 64)
* Do something less intrusive with RSSI debug
    * Would be straightforward with SPI interface instead of UART