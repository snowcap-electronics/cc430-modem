#ifndef RB_RF1A_H
#define RB_RF1A_H

void ResetRadioCore (void);
unsigned char Strobe(unsigned char strobe);

void WriteRfSettings(void);

void WriteSingleReg(unsigned char addr, unsigned char value);
void WriteBurstReg(unsigned char addr, unsigned char *buffer, unsigned char count);
unsigned char ReadSingleReg(unsigned char addr);
void ReadBurstReg(unsigned char addr, unsigned char *buffer, unsigned char count);
void WriteSinglePATable(unsigned char value);
void WriteBurstPATable(unsigned char *buffer, unsigned char count); 

#endif
