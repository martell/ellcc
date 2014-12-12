TARGET := armeb
Arch.$(TARGET) := armeb
Triple.$(TARGET) := $(TARGET)-ellcc-$(OS)-eabi
TARGET.$(TARGET) := -target $(Triple.$(TARGET))
ASFLAGS.$(TARGET) := $(TARGET.$(TARGET)) -march=armv7 -mfpu=vfp -mfloat-abi=softfp \
		    $(ASFLAGS)
CFLAGS.$(TARGET) := $(TARGET.$(TARGET)) -march=armv7 -mfpu=vfp -mfloat-abi=softfp \
		    $(CFLAGS)
CXXFLAGS.$(TARGET) := $(TARGET.$(TARGET)) -march=armv7 -mfpu=vfp -mfloat-abi=softfp \
		      $(CXXFLAGS)
LDFLAGS := $(TARGET.$(TARGET))
