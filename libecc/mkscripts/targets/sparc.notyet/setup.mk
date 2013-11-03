TARGET := sparc
Arch.$(TARGET) := sparc
Triple.$(TARGET) := $(TARGET)-ellcc-$(OS)
TARGET.$(TARGET) := -target $(Triple.$(TARGET))
CFLAGS.$(TARGET) := $(TARGET.$(TARGET)) $(CFLAGS)
CXXFLAGS.$(TARGET) := $(TARGET.$(TARGET)) $(CXXFLAGS)
LDFLAGS := $(TARGET.$(TARGET))
