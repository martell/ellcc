TARGET := arm
Arch.$(TARGET) := arm
Triple.$(TARGET) := $(TARGET)-ellcc-$(OS)-eabi
TARGET.$(TARGET) := -target $(Triple.$(TARGET))
CFLAGS.$(TARGET) := $(TARGET.$(TARGET)) -mcpu=armv6z -mfpu=vfp -mfloat-abi=softfp $(CFLAGS)
CXXFLAGS.$(TARGET) := $(TARGET.$(TARGET)) -mcpu=armv6z -mfpu=vfp -mfloat-abi=softfp $(CXXFLAGS)
LDFLAGS := $(TARGET.$(TARGET))
