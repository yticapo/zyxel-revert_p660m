CFLAGS := -O2 -pipe -Wall

all: zyxel-revert compress decompress

zyxel-revert: configfile.o event.o filedata.o logging.o context.o serial.o statemachine.o xmodem.o zyxel-revert.o
	$(CC) $(CFLAGS) $^ -o $@

compress: lzsc.o filedata.o romfile.o compress.o
	$(CC) $(CFLAGS) $^ -o $@

decompress: lzsd.o filedata.o romfile.o decompress.o
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.d: %.c
	$(CC) $(CFLAGS) -MM -c $< -o $@

clean:
	rm -f zyxel-revert compress decompress *.d *.o *.log

DEPS := $(wildcard *.c)
-include $(DEPS:.c=.d)
