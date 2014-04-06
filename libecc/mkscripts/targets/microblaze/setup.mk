TARGET := microblaze
Arch.$(TARGET) := microblaze
Triple.$(TARGET) := $(TARGET)-ellcc-$(OS)
TARGET.$(TARGET) := -target $(Triple.$(TARGET))
ASFLAGS.$(TARGET) := $(TARGET.$(TARGET)) $(ASFLAGS)
CFLAGS.$(TARGET) := $(TARGET.$(TARGET)) $(CFLAGS)
CXXFLAGS.$(TARGET) := $(TARGET.$(TARGET)) $(CXXFLAGS)
LDFLAGS := $(TARGET.$(TARGET))
