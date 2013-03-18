PROJECT1 = wireless-sensor

CROSS_COMPILE = msp430-
CC      = $(CROSS_COMPILE)gcc
AS      = $(CROSS_COMPILE)as
LD      = $(CROSS_COMPILE)gcc
CP      = $(CROSS_COMPILE)objcopy

LDFLAGS  = -mmcu=cc430f5137

SRC =   adc.c \
		adc.h \
		i2c.c \
		i2c.h \
		led.c \
		led.h \
		rf.c \
		rf.h \
		timer.c \
		timer.h \
		tmp275.c \
		tmp275.h \
		uart.c \
		uart.h \
		utils.c \
		utils.h \
		common.h \
		wireless-sensor.h \
		./HAL/RfRegSettings.c \
        ./HAL/RF1A.c \
        ./HAL/hal_pmm.c \
        ./HAL/hal_pmm.h \
        ./HAL/RF1A.h

#  C source files
CFILES = $(filter %.c, $(SRC))

# Object filse
COBJ = $(CFILES:.c=.o)
OBJ  = $(COBJ)

# This list is made with trial and error. Run make, find the missing header,
# add the path to the list.
INC = -I./HAL \
      -I/usr/msp430/include

# Compile with debug for cc430f5137
CFLAGS  = -Wall -g -O0 -mmcu=cc430f5137 -I./HAL

FEATURES += -DMHZ_433

all: $(SRC) $(PROJECT1).c $(PROJECT1).elf

$(PROJECT1).elf: $(OBJ) $(PROJECT1).o $(PROJECT1).c
	$(LD) $(LDFLAGS) $(OBJ) $(PROJECT1).o -o $@

$(COBJ): %.o: %.c
	$(CC) -c $(FEATURES) $(INC) $(CFLAGS) $< -o $@

clean:
	rm -f $(PROJECT1).elf $(PROJECT1).o $(OBJ)

.PHONY: clean
