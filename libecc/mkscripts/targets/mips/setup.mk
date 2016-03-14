TARGET := mips32r2
Arch.$(TARGET) := mips
Triple.$(TARGET) := $(TARGET)-$(OS)
TARGET.$(TARGET) := -target $(Triple.$(TARGET))
ASFLAGS.$(TARGET) := $(TARGET.$(TARGET)) -mcpu=mips32r2 -msoft-float $(ASFLAGS)
CFLAGS.$(TARGET) := $(TARGET.$(TARGET)) -mcpu=mips32r2 -msoft-float $(CFLAGS)
CXXFLAGS.$(TARGET) := $(TARGET.$(TARGET)) -mcpu=mips32r2 -msoft-float $(CXXFLAGS)
LDFLAGS := $(TARGET.$(TARGET))
