TARGET := arm32v7eb
Arch.$(TARGET) := armeb
Triple.$(TARGET) := $(TARGET)-$(OS)
TARGET.$(TARGET) := -target $(Triple.$(TARGET))
ASFLAGS.$(TARGET) := $(TARGET.$(TARGET)) -march=armv7 -mfpu=vfp -mfloat-abi=softfp \
		    $(ASFLAGS)
CFLAGS.$(TARGET) := $(TARGET.$(TARGET)) -march=armv7 -mfpu=vfp -mfloat-abi=softfp \
		    $(CFLAGS)
CXXFLAGS.$(TARGET) := $(TARGET.$(TARGET)) -march=armv7 -mfpu=vfp -mfloat-abi=softfp \
		      $(CXXFLAGS)
LDFLAGS := $(TARGET.$(TARGET))
