TARGET := armeb
Arch.$(TARGET) := armeb
Triple.$(TARGET) := $(TARGET)-ellcc-$(OS)-eabi
TARGET.$(TARGET) := -target $(Triple.$(TARGET))
CFLAGS.$(TARGET) := $(TARGET.$(TARGET)) -march=armv7 -mfpu=vfp -mfloat-abi=softfp \
		    -no-integrated-as $(CFLAGS)
CXXFLAGS.$(TARGET) := $(TARGET.$(TARGET)) -march=armv7 -mfpu=vfp -mfloat-abi=softfp \
		      -no-integrated-as $(CXXFLAGS)
LDFLAGS := $(TARGET.$(TARGET))
