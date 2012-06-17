cc430-modem
===========

cc430 modem software acts as a wireless uart (with some limitations)

Features:
* Converts \r -> \n
* Ignores extra newlines (e.g. \n\n)
* Sends characters after \n is received
* Max 32 character messages.

Currently adds also RSSI and LQI information as a separate line.
