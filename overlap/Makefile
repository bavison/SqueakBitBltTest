TARGET=overlap
OBJS=$(TARGET).o BitBltArm.o BitBltArmLinux.o BitBltArmSimd.o BitBltArmSimdAlphaBlend.o BitBltArmSimdBitLogical.o BitBltArmSimdCompare.o BitBltArmSimdPixPaint.o BitBltArmSimdSourceWord.o BitBltDispatch.o BitBltGeneric.o BitBltPlugin.o
VPATH=../../../../../src/plugins/BitBltPlugin ../../../../Cross/plugins/BitBltPlugin
CFLAGS=-g -O2 -Wall -Wextra -std=c99 -DLSB_FIRST=1 -DENABLE_FAST_BLT -I../../../../Cross/vm -I../../../../Cross/plugins/BitBltPlugin -I../../../../../build.linux32ARM/squeak.stack.v3/build -I../../../../../build.linux32ARM/squeak.stack.v3/build.debug -I../../../../../src/vm -I../../../vm

all: $(TARGET)

%.o: %.s
	../../../../../build.linux32ARM/asasm -cpu 6 -I ../../../../Cross/plugins/BitBltPlugin -o $@ $^

$(TARGET): $(OBJS)
	$(CC) -o $@ $^

clean:
	rm -rf $(TARGET) $(OBJS)
