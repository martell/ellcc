TARGET := arm
Arch.$(TARGET) := arm
Triple.$(TARGET) := $(TARGET)-ellcc-$(OS)-eabi5
TARGET.$(TARGET) := -target $(Triple.$(TARGET))
CFLAGS.$(TARGET) := $(TARGET.$(TARGET)) -march=armv7 -mfpu=vfp -mfloat-abi=softfp \
		    $(CFLAGS)
CXXFLAGS.$(TARGET) := $(TARGET.$(TARGET)) -march=armv7 -mfpu=vfp -mfloat-abi=softfp \
		     $(CXXFLAGS)
LDFLAGS := $(TARGET.$(TARGET))
