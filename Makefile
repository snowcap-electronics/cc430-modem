PROJECT1 = wireless-uart
PROJECT2 = wireless-sensor
PROJECT3 = main-fps
PROJECT4 = wireless-power-led
PROJECT5 = wireless-soil-solar

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
		fps.h \
		fps.c \
		comp.h \
		comp.c \
		common.h \
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
CFLAGS  = -Wall -g -O0 -mmcu=cc430f5137 -I./HAL -Werror -Wno-error=unused-but-set-variable -Wno-error=unused-variable

FEATURES += -DMHZ_433

all: $(SRC) $(PROJECT1).c $(PROJECT1).elf $(PROJECT2).c $(PROJECT2).elf $(PROJECT3).c $(PROJECT3).elf $(PROJECT4).c $(PROJECT4).elf $(PROJECT5).c $(PROJECT5).elf

$(PROJECT1).elf: $(OBJ) $(PROJECT1).o $(PROJECT1).c
	$(LD) $(LDFLAGS) $(OBJ) $(PROJECT1).o -o $@

$(PROJECT2).elf: $(OBJ) $(PROJECT2).o $(PROJECT2).c
	$(LD) $(LDFLAGS) $(OBJ) $(PROJECT2).o -o $@

$(PROJECT3).elf: $(OBJ) $(PROJECT3).o $(PROJECT3).c
	$(LD) $(LDFLAGS) $(OBJ) $(PROJECT3).o -o $@

$(PROJECT4).elf: $(OBJ) $(PROJECT4).o $(PROJECT4).c
	$(LD) $(LDFLAGS) $(OBJ) $(PROJECT4).o -o $@

$(PROJECT5).elf: $(OBJ) $(PROJECT5).o $(PROJECT5).c
	$(LD) $(LDFLAGS) $(OBJ) $(PROJECT5).o -o $@

$(COBJ): %.o: %.c
	$(CC) -c $(FEATURES) $(INC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o HAL/*.o *.elf

.PHONY: clean
