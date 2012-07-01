cc430-modem
===========

cc430 modem software acts as a wireless uart (with some limitations)

Features:
* Converts \r -> \n
* Ignores extra newlines (e.g. \n\n)
* Sends characters after \n is received
* Max 32 character messages.
* Ignores 1 byte messages

Currently adds also RSSI and LQI information as a separate line.

TODO:
* Don't alter messages (when to send message without extra wait?)
** Would be straightforward with SPI interface instead of UART
* Send uart messages to RF after 10-20ms of idle (when no \n received)
* Increase the max message length to ~60 bytes (RF FIFO len is 64)
* Do something less intrusive with RSSI debug
** Would be straightforward with SPI interface instead of UART