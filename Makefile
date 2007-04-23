CFLAGS := -O2 -pipe -Wall

OBJS := configfile.o event.o logging.o
OBJS += context.o serial.o statemachine.o xmodem.o

all: zyxel-revert

zyxel-revert: $(OBJS) zyxel-revert.o
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.d: %.c
	$(CC) $(CFLAGS) -MM -c $< -o $@

clean:
	rm -f zyxel-revert *.d *.o *.log

DEPS := $(wildcard *.c)
-include $(DEPS:.c=.d)
