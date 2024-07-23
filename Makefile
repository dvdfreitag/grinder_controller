TARGET := grinder_controller

CC := avr-gcc
OBJCOPY := avr-objcopy
OBJDUMP := avr-objdump
AVRDUDE := avrdude

SRC_DIR := src
INC_DIR := include
BUILD_DIR := build

CPUFLAGS := \
	-mmcu=atmega328p

DEFINES := \
	-DF_CPU=16000000UL

CFLAGS := \
	$(CPUFLAGS) \
	$(DEFINES) \
	-O2 \
	-Wall \
	-Wextra \
	-Wstrict-prototypes \
	-fdata-sections \
	-ffunction-sections \
	-ffat-lto-objects \
	-flto \
	-fsigned-char \
	-g3 \
	-std=gnu11

LDFLAGS := \
	$(CPUFLAGS) \
	-Wall \
	-Wextra \
	-g3 \
	-Wl,--gc-sections \
	-Wl,--print-memory-usage

SRCS := \
	$(SRC_DIR)/main.c \
	$(SRC_DIR)/gpio.c \
	$(SRC_DIR)/encoder.c \
	$(SRC_DIR)/clock.c \
	$(SRC_DIR)/uart.c \
	$(SRC_DIR)/spi.c \
	$(SRC_DIR)/display.c

OBJS := \
	$(addprefix $(BUILD_DIR)/,$(notdir $(SRCS:.c=.o)))

DEPFLAGS = -MT "$@" -MMD -MP -MF "$(BUILD_DIR)/$*.d"
DEPFILES := $(OBJS:.o=.d)

.PHONY: all flash clean
all: $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).lss

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo [ CC ] $@
	@$(CC) -x c $(CFLAGS) -I$(INC_DIR) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf
	@echo [ HEX ] $@
	@$(OBJCOPY) -O ihex -R .eeprom $< $@

$(BUILD_DIR)/%.lss: $(BUILD_DIR)/%.elf
	@echo [ LSS ] $@
	@$(OBJDUMP) -h -S $< > $@

$(BUILD_DIR)/%.eep: $(BUILD_DIR)/%.elf
	@echo [ EEP ] $@
	@$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" --change-section-lma .eeprom=0 -O ihex $< $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	@echo [ LD ] $@
	@$(CC) $(LDFLAGS) $^ -o $@

$(BUILD_DIR):
	@mkdir -p $@

flash: $(BUILD_DIR)/$(TARGET).hex
	@$(AVRDUDE) -p atmega328p -P /dev/ttyUSB0 -c arduino -b 57600 -DV -U flash:w:$(BUILD_DIR)/$(TARGET).hex:i

clean:
	@rm -rf $(BUILD_DIR)

$(DEPFILES): ;
include $(wildcard $(DEPFILES))