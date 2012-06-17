PROJECT = cc430-modem

CROSS_COMPILE = msp430-
CC      = $(CROSS_COMPILE)gcc
AS      = $(CROSS_COMPILE)as
LD      = $(CROSS_COMPILE)gcc
CP      = $(CROSS_COMPILE)objcopy

LDFLAGS  = -mmcu=cc430f5137

SRC =   ./HAL/RfRegSettings.c \
        ./HAL/RF1A.c \
        ./HAL/hal_pmm.c \
        ./HAL/hal_pmm.h \
        ./HAL/RF1A.h \
        ./$(PROJECT).c

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
CFLAGS  = -Wall -g -O0 -mmcu=cc430f5137

FEATURES += -DMHZ_433

all: $(SRC) $(PROJECT).elf

$(PROJECT).elf: $(OBJ)
	$(LD) $(LDFLAGS) $(OBJ) -o $@

$(COBJ): %.o: %.c
	$(CC) -c $(FEATURES) $(INC) $(CFLAGS) $< -o $@

clean:
	rm -f $(PROJECT).elf $(OBJ)

.PHONY: clean
