TARGET := thumb
Arch.$(TARGET) := thumb
Triple.$(TARGET) := thumb-linux-engeabi
TARGET.$(TARGET) := -target $(Triple.$(TARGET))
ASFLAGS.$(TARGET) := $(TARGET.$(TARGET)) $(ASFLAGS)
CFLAGS.$(TARGET) := $(TARGET.$(TARGET)) $(CFLAGS)
CXXFLAGS.$(TARGET) := $(TARGET.$(TARGET)) $(CXXFLAGS)
LDFLAGS := $(TARGET.$(TARGET))
