TARGET=fuzz
ARCH:=$(shell uname -m)
OBJS_armv7l=BitBltArm.o BitBltArmLinux.o BitBltArmSimd.o BitBltArmSimdAlphaBlend.o BitBltArmSimdBitLogical.o BitBltArmSimdCompare.o BitBltArmSimdPixPaint.o BitBltArmSimdSourceWord.o
OBJS_aarch64=
BUILD_armv7l=../../../../../build.linux32ARMv6/squeak.stack.v3
BUILD_aarch64=../../../../../build.linux64ARMv8/squeak.cog.spur
OBJS=$(TARGET).o $(OBJS_$(ARCH)) BitBltDispatch.o BitBltGeneric.o BitBltPlugin.o
VPATH=../../../../../src/plugins/BitBltPlugin ../../../../Cross/plugins/BitBltPlugin
CFLAGS=-g -O2 -Wall -Wextra -std=c99 -DLSB_FIRST=1 -DENABLE_FAST_BLT \
  -I$(BUILD_$(ARCH))/build \
  -I$(BUILD_$(ARCH))/build.debug \
  -I../../../../unix/vm \
  -I../../../../Cross/vm \
  -I../../../../../spur64src/vm \
  -I../../../../Cross/plugins/BitBltPlugin \

all: $(TARGET)

%.o: %.s
	../../../../../build.linux32ARMv6/asasm -cpu 6 -I ../../../../Cross/plugins/BitBltPlugin -o $@ $^

$(TARGET): $(OBJS)
	$(CC) -o $@ $^

clean:
	rm -rf $(TARGET) $(OBJS)
