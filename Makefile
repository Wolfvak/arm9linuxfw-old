ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM>")
endif

include $(DEVKITARM)/base_tools

TARGET  := $(shell basename $(CURDIR))

SOURCE  := src
BUILD   := bin
SUBARCH := -mcpu=arm946e-s -mfloat-abi=soft \
           -marm -mno-thumb-interwork -ggdb

INCLUDE := -I$(SOURCE) -Iinc
ASFLAGS := $(SUBARCH) $(INCLUDE) -x assembler-with-cpp
CFLAGS  := $(SUBARCH) $(INCLUDE) -O2 -std=c99 -pipe -fomit-frame-pointer \
           -ffunction-sections -ffast-math -Wall -Wextra \
           -Wno-unused-variable -Wno-unused-parameter

LDFLAGS := -Tlink.ld -Wl,--gc-sections,--nmagic,-z,max-page-size=4 \
           -nostartfiles $(SUBARCH)

SOURCE_OUTPUT := $(patsubst $(SOURCE)/%.c, $(BUILD)/%.c.o, \
                 $(patsubst $(SOURCE)/%.s, $(BUILD)/%.s.o, \
                 $(shell find "$(SOURCE)" -name '*.c' -o -name '*.s')))


.PHONY: all
all: $(TARGET).bin

.PHONY: clean
clean:
	@rm -rf $(BUILD) $(TARGET).elf $(TARGET).bin

$(TARGET).bin: $(TARGET).elf
	@$(OBJCOPY) -O binary $^ $@

$(TARGET).elf: $(SOURCE_OUTPUT)
	@mkdir -p "$(@D)"
	@$(CC) $(LDFLAGS) $^ -o $@

$(BUILD)/%.c.o: $(SOURCE)/%.c
	@mkdir -p "$(@D)"
	@echo "\t $<"
	@$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD)/%.s.o: $(SOURCE)/%.s
	@mkdir -p "$(@D)"
	@echo "\t $<"
	@$(CC) -c $(ASFLAGS) -o $@ $<
