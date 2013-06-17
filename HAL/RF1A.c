#include "RF1A.h"
#include "cc430x513x.h"
#include <stdint.h>

// *****************************************************************************
// @fn          Strobe
// @brief       Send a command strobe to the radio. Includes workaround for RF1A7
// @param       unsigned char strobe        The strobe command to be sent
// @return      unsigned char statusByte    The status byte that follows the strobe
// *****************************************************************************
unsigned char Strobe(unsigned char strobe)
{
  unsigned char statusByte = 0;
  unsigned int  gdo_state;
  
  // Check for valid strobe command 
  if((strobe == 0xBD) || ((strobe >= RF_SRES) && (strobe <= RF_SNOP)))
  {
    // Clear the Status read flag 
    RF1AIFCTL1 &= ~(RFSTATIFG);    
    
    // Wait for radio to be ready for next instruction
    while( !(RF1AIFCTL1 & RFINSTRIFG));
    
    // Write the strobe instruction
    if ((strobe > RF_SRES) && (strobe < RF_SNOP))
    {
      gdo_state = ReadSingleReg(IOCFG2);    // buffer IOCFG2 state
      WriteSingleReg(IOCFG2, 0x29);         // chip-ready to GDO2
      
      RF1AINSTRB = strobe; 
      if ( (RF1AIN&0x04)== 0x04 )           // chip at sleep mode
      {
        if ( (strobe == RF_SXOFF) || (strobe == RF_SPWD) || (strobe == RF_SWOR) ) { }
        else  	
        {
          while ((RF1AIN&0x04)== 0x04);     // chip-ready ?
          // Delay for ~810usec at 1.05MHz CPU clock, see erratum RF1A7
          __delay_cycles(850);	            
        }
      }
      WriteSingleReg(IOCFG2, gdo_state);    // restore IOCFG2 setting
    
      while( !(RF1AIFCTL1 & RFSTATIFG) );
    }
    else		                    // chip active mode (SRES)
    {	
      RF1AINSTRB = strobe; 	   
    }
    statusByte = RF1ASTATB;
  }
  return statusByte;
}

// *****************************************************************************
// @fn          ReadSingleReg
// @brief       Read a single byte from the radio register
// @param       unsigned char addr      Target radio register address
// @return      unsigned char data_out  Value of byte that was read
// *****************************************************************************
unsigned char ReadSingleReg(unsigned char addr)
{
  unsigned char data_out;
  
  // Check for valid configuration register address, 0x3E refers to PATABLE 
  if ((addr <= 0x2E) || (addr == 0x3E))
    // Send address + Instruction + 1 dummy byte (auto-read)
    RF1AINSTR1B = (addr | RF_SNGLREGRD);    
  else
    // Send address + Instruction + 1 dummy byte (auto-read)
    RF1AINSTR1B = (addr | RF_STATREGRD);    
  
  while (!(RF1AIFCTL1 & RFDOUTIFG) );
  data_out = RF1ADOUTB;                    // Read data and clears the RFDOUTIFG

  return data_out;
}

// *****************************************************************************
// @fn          WriteSingleReg
// @brief       Write a single byte to a radio register
// @param       unsigned char addr      Target radio register address
// @param       unsigned char value     Value to be written
// @return      none
// *****************************************************************************
void WriteSingleReg(unsigned char addr, unsigned char value)
{   
  while (!(RF1AIFCTL1 & RFINSTRIFG));       // Wait for the Radio to be ready for next instruction
  RF1AINSTRB = (addr | RF_SNGLREGWR);	    // Send address + Instruction

  RF1ADINB = value; 			    // Write data in 

  __no_operation(); 
}
        
// *****************************************************************************
// @fn          ReadBurstReg
// @brief       Read multiple bytes to the radio registers
// @param       unsigned char addr      Beginning address of burst read
// @param       unsigned char *buffer   Pointer to data table
// @param       unsigned char count     Number of bytes to be read
// @return      none
// *****************************************************************************
void ReadBurstReg(unsigned char addr, unsigned char *buffer, unsigned char count)
{
  unsigned int i;
  if(count > 0)
  {
    while (!(RF1AIFCTL1 & RFINSTRIFG));       // Wait for INSTRIFG
    RF1AINSTR1B = (addr | RF_REGRD);          // Send addr of first conf. reg. to be read 
                                              // ... and the burst-register read instruction
    for (i = 0; i < (count-1); i++)
    {
      while (!(RFDOUTIFG&RF1AIFCTL1));        // Wait for the Radio Core to update the RF1ADOUTB reg
      buffer[i] = RF1ADOUT1B;                 // Read DOUT from Radio Core + clears RFDOUTIFG
                                              // Also initiates auo-read for next DOUT byte
    }
    buffer[count-1] = RF1ADOUT0B;             // Store the last DOUT from Radio Core  
  }
}  

// *****************************************************************************
// @fn          WriteBurstReg
// @brief       Write multiple bytes to the radio registers
// @param       unsigned char addr      Beginning address of burst write
// @param       unsigned char *buffer   Pointer to data table
// @param       unsigned char count     Number of bytes to be written
// @return      none
// *****************************************************************************
void WriteBurstReg(unsigned char addr, unsigned char *buffer, unsigned char count)
{  
  unsigned char i;

  if(count > 0)
  {
    while (!(RF1AIFCTL1 & RFINSTRIFG));       // Wait for the Radio to be ready for next instruction
    RF1AINSTRW = ((addr | RF_REGWR)<<8 ) + buffer[0]; // Send address + Instruction
  
    for (i = 1; i < count; i++)
    {
      RF1ADINB = buffer[i];                   // Send data
      while (!(RFDINIFG & RF1AIFCTL1));       // Wait for TX to finish
    } 
    i = RF1ADOUTB;                            // Reset RFDOUTIFG flag which contains status byte  
  }
}

// *****************************************************************************
// @fn          ResetRadioCore
// @brief       Reset the radio core using RF_SRES command
// @param       none
// @return      none
// *****************************************************************************
void ResetRadioCore (void)
{
  Strobe(RF_SRES);                          // Reset the Radio Core
  Strobe(RF_SNOP);                          // Reset Radio Pointer
}

// *****************************************************************************
// @fn          WriteRfSettings
// @brief       Write the minimum set of RF configuration register settings
// @param       RF_SETTINGS *pRfSettings  Pointer to the structure that holds the rf settings
// @return      none
// *****************************************************************************
void WriteRfSettings(void) {

//#define RF_MODE_OPTIMISED_CONSUMPTION 1
#define RF_MODE_OPTIMISED_SENSITIVITY 1

#ifdef RF_MODE_OPTIMISED_CONSUMPTION
/* Sync word qualifier mode = 30/32 sync word bits detected */
/* CRC autoflush = false */
/* Channel spacing = 199.951172 */
/* Data format = Normal mode */
/* Data rate = 38.3835 */
/* RX filter BW = 101.562500 */
/* PA ramping = false */
/* Preamble count = 4 */
/* Whitening = false */
/* Address config = No address check */
/* Carrier frequency = 433.999969 */
/* Device address = 0 */
/* TX power = 0 */
/* Manchester enable = false */
/* CRC enable = true */
/* Deviation = 20.629883 */
/* Packet length mode = Variable packet length mode. Packet length configured by the first byte after sync word */
/* Packet length = 255 */
/* Modulation format = 2-GFSK */
/* Base frequency = 433.999969 */
/* Modulated = true */
/* Channel number = 0 */
/* RF settings SoC: CC430 */
    WriteSingleReg(IOCFG2      , 0x29); // gdo2 output configuration
    WriteSingleReg(IOCFG1      , 0x2E); // gdo1 output configuration
    WriteSingleReg(IOCFG0      , 0x06); // gdo0 output configuration
    WriteSingleReg(FIFOTHR     , 0x47); // rx fifo and tx fifo thresholds
    WriteSingleReg(SYNC1       , 0xD3); // sync word, high byte
    WriteSingleReg(SYNC0       , 0x91); // sync word, low byte
    WriteSingleReg(PKTLEN      , 0x32); // packet length
    WriteSingleReg(PKTCTRL1    , 0x04); // packet automation control
    WriteSingleReg(PKTCTRL0    , 0x05); // packet automation control
    WriteSingleReg(ADDR        , 0x00); // device address
    WriteSingleReg(CHANNR      , 0x00); // channel number
    WriteSingleReg(FSCTRL1     , 0x08); // frequency synthesizer control
    WriteSingleReg(FSCTRL0     , 0x00); // frequency synthesizer control
    WriteSingleReg(FREQ2       , 0x10); // frequency control word, high byte
    WriteSingleReg(FREQ1       , 0xB1); // frequency control word, middle byte
    WriteSingleReg(FREQ0       , 0x3B); // frequency control word, low byte
    WriteSingleReg(MDMCFG4     , 0xCA); // modem configuration
    WriteSingleReg(MDMCFG3     , 0x83); // modem configuration
    WriteSingleReg(MDMCFG2     , 0x93); // modem configuration
    WriteSingleReg(MDMCFG1     , 0x22); // modem configuration
    WriteSingleReg(MDMCFG0     , 0xF8); // modem configuration
    WriteSingleReg(DEVIATN     , 0x35); // modem deviation setting
    WriteSingleReg(MCSM2       , 0x07); // main radio control state machine configuration
    WriteSingleReg(MCSM1       , 0x30); // main radio control state machine configuration
    WriteSingleReg(MCSM0       , 0x10); // main radio control state machine configuration
    WriteSingleReg(FOCCFG      , 0x16); // frequency offset compensation configuration
    WriteSingleReg(BSCFG       , 0x6C); // bit synchronization configuration
    WriteSingleReg(AGCCTRL2    , 0x43); // agc control
    WriteSingleReg(AGCCTRL1    , 0x40); // agc control
    WriteSingleReg(AGCCTRL0    , 0x91); // agc control
    WriteSingleReg(WOREVT1     , 0x80); // high byte event0 timeout
    WriteSingleReg(WOREVT0     , 0x00); // low byte event0 timeout
    WriteSingleReg(WORCTRL     , 0xFB); // wake on radio control
    WriteSingleReg(FREND1      , 0x56); // front end rx configuration
    WriteSingleReg(FREND0      , 0x10); // front end tx configuration
    WriteSingleReg(FSCAL3      , 0xE9); // frequency synthesizer calibration
    WriteSingleReg(FSCAL2      , 0x2A); // frequency synthesizer calibration
    WriteSingleReg(FSCAL1      , 0x00); // frequency synthesizer calibration
    WriteSingleReg(FSCAL0      , 0x1F); // frequency synthesizer calibration
    WriteSingleReg(FSTEST      , 0x59); // frequency synthesizer calibration control
    WriteSingleReg(PTEST       , 0x7F); // production test
    WriteSingleReg(AGCTEST     , 0x3F); // agc test
    WriteSingleReg(TEST2       , 0x81); // various test settings
    WriteSingleReg(TEST1       , 0x35); // various test settings
    WriteSingleReg(TEST0       , 0x09); // various test settings
#endif

#ifdef RF_MODE_OPTIMISED_SENSITIVITY
/* Sync word qualifier mode = 30/32 sync word bits detected */
/* CRC autoflush = false */
/* Channel spacing = 199.951172 */
/* Data format = Normal mode */
/* Data rate = 38.3835 */
/* RX filter BW = 101.562500 */
/* PA ramping = false */
/* Preamble count = 4 */
/* Whitening = false */
/* Address config = No address check */
/* Carrier frequency = 433.999969 */
/* Device address = 0 */
/* TX power = 0 */
/* Manchester enable = false */
/* CRC enable = true */
/* Deviation = 20.629883 */
/* Packet length mode = Variable packet length mode. Packet length configured by the first byte after sync word */
/* Packet length = 255 */
/* Modulation format = 2-GFSK */
/* Base frequency = 433.999969 */
/* Modulated = true */
/* Channel number = 0 */
/* RF settings SoC: CC430 */
    WriteSingleReg(IOCFG2      , 0x29); // gdo2 output configuration
    WriteSingleReg(IOCFG1      , 0x2E); // gdo1 output configuration
    WriteSingleReg(IOCFG0      , 0x06); // gdo0 output configuration
    WriteSingleReg(FIFOTHR     , 0x47); // rx fifo and tx fifo thresholds
    WriteSingleReg(SYNC1       , 0xD3); // sync word, high byte
    WriteSingleReg(SYNC0       , 0x91); // sync word, low byte
    WriteSingleReg(PKTLEN      , 0xFF); // packet length
    WriteSingleReg(PKTCTRL1    , 0x04); // packet automation control
    WriteSingleReg(PKTCTRL0    , 0x05); // packet automation control
    WriteSingleReg(ADDR        , 0x00); // device address
    WriteSingleReg(CHANNR      , 0x00); // channel number
    WriteSingleReg(FSCTRL1     , 0x06); // frequency synthesizer control
    WriteSingleReg(FSCTRL0     , 0x00); // frequency synthesizer control
    WriteSingleReg(FREQ2       , 0x10); // frequency control word, high byte
    WriteSingleReg(FREQ1       , 0xB1); // frequency control word, middle byte
    WriteSingleReg(FREQ0       , 0x3B); // frequency control word, low byte
    WriteSingleReg(MDMCFG4     , 0xCA); // modem configuration
    WriteSingleReg(MDMCFG3     , 0x83); // modem configuration
    WriteSingleReg(MDMCFG2     , 0x13); // modem configuration
    WriteSingleReg(MDMCFG1     , 0x22); // modem configuration
    WriteSingleReg(MDMCFG0     , 0xF8); // modem configuration
    WriteSingleReg(DEVIATN     , 0x35); // modem deviation setting
    WriteSingleReg(MCSM2       , 0x07); // main radio control state machine configuration
    WriteSingleReg(MCSM1       , 0x30); // main radio control state machine configuration
    WriteSingleReg(MCSM0       , 0x10); // main radio control state machine configuration
    WriteSingleReg(FOCCFG      , 0x16); // frequency offset compensation configuration
    WriteSingleReg(BSCFG       , 0x6C); // bit synchronization configuration
    WriteSingleReg(AGCCTRL2    , 0x43); // agc control
    WriteSingleReg(AGCCTRL1    , 0x40); // agc control
    WriteSingleReg(AGCCTRL0    , 0x91); // agc control
    WriteSingleReg(WOREVT1     , 0x80); // high byte event0 timeout
    WriteSingleReg(WOREVT0     , 0x00); // low byte event0 timeout
    WriteSingleReg(WORCTRL     , 0xFB); // wake on radio control
    WriteSingleReg(FREND1      , 0x56); // front end rx configuration
    WriteSingleReg(FREND0      , 0x10); // front end tx configuration
    WriteSingleReg(FSCAL3      , 0xE9); // frequency synthesizer calibration
    WriteSingleReg(FSCAL2      , 0x2A); // frequency synthesizer calibration
    WriteSingleReg(FSCAL1      , 0x00); // frequency synthesizer calibration
    WriteSingleReg(FSCAL0      , 0x1F); // frequency synthesizer calibration
    WriteSingleReg(FSTEST      , 0x59); // frequency synthesizer calibration control
    WriteSingleReg(PTEST       , 0x7F); // production test
    WriteSingleReg(AGCTEST     , 0x3F); // agc test
    WriteSingleReg(TEST2       , 0x81); // various test settings
    WriteSingleReg(TEST1       , 0x35); // various test settings
    WriteSingleReg(TEST0       , 0x09); // various test settings
#endif

}

// *****************************************************************************
// @fn          WritePATable
// @brief       Write data to power table
// @param       unsigned char value		Value to write
// @return      none
// *****************************************************************************
void WriteSinglePATable(unsigned char value)
{
  while( !(RF1AIFCTL1 & RFINSTRIFG));
  RF1AINSTRW = 0x3E00 + value;              // PA Table single write
  
  while( !(RF1AIFCTL1 & RFINSTRIFG));
  RF1AINSTRB = RF_SNOP;                     // reset PA_Table pointer
}

// *****************************************************************************
// @fn          WritePATable
// @brief       Write to multiple locations in power table 
// @param       unsigned char *buffer	Pointer to the table of values to be written 
// @param       unsigned char count	Number of values to be written
// @return      none
// *****************************************************************************
void WriteBurstPATable(unsigned char *buffer, unsigned char count)
{
  volatile char i = 0; 
  
  while( !(RF1AIFCTL1 & RFINSTRIFG));
  RF1AINSTRW = 0x7E00 + buffer[(uint8_t)i];          // PA Table burst write   

  for (i = 1; i < count; i++)
  {
    RF1ADINB = buffer[(uint8_t)i];                   // Send data
    while (!(RFDINIFG & RF1AIFCTL1));       // Wait for TX to finish
  } 
  i = RF1ADOUTB;                            // Reset RFDOUTIFG flag which contains status byte

  while( !(RF1AIFCTL1 & RFINSTRIFG));
  RF1AINSTRB = RF_SNOP;                     // reset PA Table pointer
}
