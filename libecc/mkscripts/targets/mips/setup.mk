TARGET := mips
Arch.$(TARGET) := mips
Triple.$(TARGET) := $(TARGET)-ellcc-$(OS)
TARGET.$(TARGET) := -target $(Triple.$(TARGET))
CFLAGS.$(TARGET) := $(TARGET.$(TARGET)) -mcpu=mips32r2 $(CFLAGS)
CXXFLAGS.$(TARGET) := $(TARGET.$(TARGET)) -mcpu=mips32r2 $(CXXFLAGS)
LDFLAGS := $(TARGET.$(TARGET))
