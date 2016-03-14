TARGET := arm32v7
Arch.$(TARGET) := arm
Triple.$(TARGET) := $(TARGET)-$(OS)
TARGET.$(TARGET) := -target $(Triple.$(TARGET))
ASFLAGS.$(TARGET) := $(TARGET.$(TARGET)) -march=armv7 -mfpu=none -mfloat-abi=softfp \
		    $(ASFLAGS)
CFLAGS.$(TARGET) := $(TARGET.$(TARGET)) -march=armv7a -mfpu=none -mfloat-abi=softfp \
		    $(CFLAGS)
CXXFLAGS.$(TARGET) := $(TARGET.$(TARGET)) -march=armv7 -mfpu=none -mfloat-abi=softfp \
		     $(CXXFLAGS)
LDFLAGS := $(TARGET.$(TARGET))
