TARGET_EXEC ?= yunolex

Q ?= @
ECHO := $(Q)echo
ECXX := $(ECHO) "\033[34m CXX \033[0m" 
EBIN := $(ECHO) "\033[32m BIN \033[0m"

CXX ?= clang++
CXXFLAGS = -std=c++20 -Wall
DBG_CXXFLAGS = $(CXXFLAGS) -DDEBUG -DVERBOSE -g

BUILD_DIR ?= ./build
DBG_BUILD_DIR ?= ./build_dbg
SRC_DIRS ?= ./src

SRCS := $(shell find $(SRC_DIRS) -name *.cpp -or -name *.c -or -name *.s)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)
DBG_OBJS := $(SRCS:%=$(DBG_BUILD_DIR)/%.o)
DBG_DEPS := $(DBG_OBJS:.o=.d)

$(TARGET_EXEC): $(OBJS)
	$(EBIN) $(TARGET_EXEC)
	$(Q)$(CXX) $(OBJS) -o yunolex

$(BUILD_DIR)/%.cpp.o: %.cpp
	$(Q)mkdir -p $(dir $@)
	$(ECXX) $<
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: debug
debug: $(DBG_OBJS)
	$(EBIN) $(TARGET_EXEC)_dbg
	$(Q)$(CXX) -DDEBUG $(DBG_OBJS) -o $(TARGET_EXEC)_dbg $(DBG_CXXFLAGS)

$(DBG_BUILD_DIR)/%.cpp.o: %.cpp
	$(Q)mkdir -p $(dir $@)
	$(ECXX) $<
	$(Q)$(CXX) $(DBG_CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(Q)rm -rf $(TARGET_EXEC) $(TARGET_EXEC)_dbg $(BUILD_DIR) $(DBG_BUILD_DIR) vgcore.*

-include $(DEPS)