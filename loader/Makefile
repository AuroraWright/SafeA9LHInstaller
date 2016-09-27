rwildcard = $(foreach d, $(wildcard $1*), $(filter $(subst *, %, $2), $d) $(call rwildcard, $d/, $2))

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/3ds_rules

CC := arm-none-eabi-gcc
AS := arm-none-eabi-as
LD := arm-none-eabi-ld
OC := arm-none-eabi-objcopy

name := $(shell basename $(CURDIR))

dir_source := source
dir_arm11 := arm11
dir_build := build
dir_out := ../$(dir_build)

ASFLAGS := -mcpu=arm946e-s
CFLAGS  := -Wall -Wextra -MMD -MP -mthumb -mthumb-interwork $(ASFLAGS) -fno-builtin -std=c11 -Wno-main -O2 -flto -ffast-math
LDFLAGS := -nostdlib

objects = $(patsubst $(dir_source)/%.s, $(dir_build)/%.o, \
          $(patsubst $(dir_source)/%.c, $(dir_build)/%.o, \
          $(call rwildcard, $(dir_source), *.s *.c)))

bundled = $(dir_build)/arm11.bin.o

define bin2o
	bin2s $< | $(AS) -o $(@)
	echo "extern const u8" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"[];" >> $(dir_build)/bundled.h
	echo "extern const u32" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`_size";" >> $(dir_build)/bundled.h
endef

.PHONY: all
all: $(dir_out)/$(name).bin

.PHONY: clean
clean:
	@$(MAKE) -C $(dir_arm11) clean
	@rm -rf $(dir_build)

$(dir_out)/$(name).bin: $(dir_build)/$(name).elf
	$(OC) -S -O binary $< $@

$(dir_build)/$(name).elf: $(bundled) $(objects)
	$(LINK.o) -T linker.ld $(OUTPUT_OPTION) $^

$(dir_build)/%.bin.o: $(dir_build)/%.bin
	@$(bin2o)

$(dir_build)/arm11.bin: $(dir_arm11)
	@mkdir -p "$(@D)"
	@$(MAKE) -C $<

$(dir_build)/memory.o: CFLAGS += -O3

$(dir_build)/%.o: $(dir_source)/%.c
	@mkdir -p "$(@D)"
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(dir_build)/%.o: $(dir_source)/%.s
	@mkdir -p "$(@D)"
	$(COMPILE.s) $(OUTPUT_OPTION) $<
include $(call rwildcard, $(dir_build), *.d)
