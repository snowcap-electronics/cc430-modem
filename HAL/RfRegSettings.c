#include "RF1A.h"

// Rf settings for CC430
RF_SETTINGS rfSettings = {
    0x08,  // FSCTRL1          Frequency Synthesizer Control
    0x00,  // FSCTRL0          Frequency Synthesizer Control
    0x10,  // FREQ2            Frequency Control Word, High Byte
    0xB1,  // FREQ1            Frequency Control Word, Middle Byte
    0x3B,  // FREQ0            Frequency Control Word, Low Byte
    0xCA,  // MDMCFG4          Modem Configuration
    0x83,  // MDMCFG3          Modem Configuration
    0x93,  // MDMCFG2          Modem Configuration
    0x22,  // MDMCFG1          Modem Configuration
    0xF8,  // MDMCFG0          Modem Configuration
    0x00,  // CHANNR           Channel Number
    0x35,  // DEVIATN          Modem Deviation Setting
    0x56,  // FREND1           Front End RX Configuration
    0x10,  // FREND0           Front End TX Configuration
    0x10,  // MCSM0            Main Radio Control State Machine Configuration
    0x16,  // FOCCFG           Frequency Offset Compensation Configuration
    0x6C,  // BSCFG            Bit Synchronization Configuration
    0x43,  // AGCCTRL2         AGC Control
    0x40,  // AGCCTRL1         AGC Control
    0x91,  // AGCCTRL0         AGC Control
    0xE9,  // FSCAL3           Frequency Synthesizer Calibration
    0x2A,  // FSCAL2           Frequency Synthesizer Calibration
    0x00,  // FSCAL1           Frequency Synthesizer Calibration
    0x1F,  // FSCAL0           Frequency Synthesizer Calibration
    0x59,  // FSTEST           Frequency Synthesizer Calibration Control
    0x81,  // TEST2            Various Test Settings
    0x35,  // TEST1            Various Test Settings
    0x09,  // TEST0            Various Test Settings
    0x47,  // FIFOTHR          RX FIFO and TX FIFO Thresholds
    0x29,  // IOCFG2           GDO2 Output Configuration
    0x06,  // IOCFG0           GDO0 output pin configuration.
    0x04,  // PKTCTRL1         Packet Automation Control
    0x05,  // PKTCTRL0         Packet Automation Control
    0x00,  // ADDR             Device Address
    0x32,  // PKTLEN           Packet Length
};

