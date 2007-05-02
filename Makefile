CFLAGS := -O2 -pipe -Wall

all: zyxel-revert compress decompress cfgpatch

zyxel-revert: event.o filedata.o linebuffer.o logging.o context.o serial.o statemachine.o xmodem.o zyxel-revert.o
	$(CC) $(CFLAGS) $^ -o $@

compress: filedata.o lzsc.o romfile.o compress.o
	$(CC) $(CFLAGS) $^ -o $@

decompress: filedata.o lzsd.o romfile.o decompress.o
	$(CC) $(CFLAGS) $^ -o $@

cfgpatch: configdata.o filedata.o cfgpatch.o
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.d: %.c
	$(CC) $(CFLAGS) -MM -c $< -o $@

clean:
	rm -f zyxel-revert compress decompress cfgpatch *.d *.o *.log

DEPS := $(wildcard *.c)
-include $(DEPS:.c=.d)
