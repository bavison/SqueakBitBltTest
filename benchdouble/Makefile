TARGET=benchdouble
OBJS=$(TARGET).o PixelDouble.o BitBltArmSimdPixelDouble.o
VPATH=../../../../Cross/plugins/BitBltPlugin
CFLAGS=-g -O2 -Wall -Wextra -std=c99 -I../../../../Cross/plugins/BitBltPlugin

all: $(TARGET)

%.o: %.s
	../../../../../build.linux32ARM/asasm -cpu 6 -I ../../../../Cross/plugins/BitBltPlugin -o $@ $^

$(TARGET): $(OBJS)
	$(CC) -o $@ $^

clean:
	rm -rf $(TARGET) $(OBJS)
