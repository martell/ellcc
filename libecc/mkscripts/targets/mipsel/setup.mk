TARGET := mipsel
Arch.$(TARGET) := mipsel
Triple.$(TARGET) := $(TARGET)-ellcc-$(OS)
TARGET.$(TARGET) := -target $(Triple.$(TARGET))
ASFLAGS.$(TARGET) := $(TARGET.$(TARGET)) -mcpu=mips32r2 $(ASFLAGS)
CFLAGS.$(TARGET) := $(TARGET.$(TARGET)) -mcpu=mips32r2 $(CFLAGS)
CXXFLAGS.$(TARGET) := $(TARGET.$(TARGET)) -mcpu=mips32r2 $(CXXFLAGS)
LDFLAGS := $(TARGET.$(TARGET))
