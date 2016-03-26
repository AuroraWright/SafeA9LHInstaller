rwildcard = $(foreach d, $(wildcard $1*), $(filter $(subst *, %, $2), $d) $(call rwildcard, $d/, $2))

CC := arm-none-eabi-gcc
AS := arm-none-eabi-as
LD := arm-none-eabi-ld
OC := arm-none-eabi-objcopy

name := SafeA9LHInstaller

dir_source := source
dir_mset := CakeHax
dir_ninjhax := CakeBrah
dir_build := build
dir_out := out

ASFLAGS := -mlittle-endian -mcpu=arm946e-s -march=armv5te
CFLAGS := -Wall -Wextra -MMD -MP -marm $(ASFLAGS) -fno-builtin -fshort-wchar -std=c11 -Wno-main -O2 -ffast-math
FLAGS := name=$(name).dat dir_out=$(abspath $(dir_out)) ICON=$(abspath icon.png) APP_DESCRIPTION="Noob-proof ARM9LoaderHax installer/updater." APP_AUTHOR="Aurora Wright" --no-print-directory

objects= $(patsubst $(dir_source)/%.s, $(dir_build)/%.o, \
			  $(patsubst $(dir_source)/%.c, $(dir_build)/%.o, \
			  $(call rwildcard, $(dir_source), *.s *.c)))


.PHONY: all
all: launcher a9lh ninjhax

.PHONY: launcher
launcher: $(dir_out)/$(name).dat 

.PHONY: a9lh
a9lh: $(dir_out)/arm9loaderhax.bin

.PHONY: ninjhax
ninjhax: $(dir_out)/3ds/$(name)

.PHONY: release
release: $(dir_out)/$(name).zip

.PHONY: clean
clean:
	@$(MAKE) $(FLAGS) -C $(dir_mset) clean
	@$(MAKE) $(FLAGS) -C $(dir_ninjhax) clean
	@rm -rf $(dir_out) $(dir_build)

$(dir_out):
	@mkdir -p "$(dir_out)"

$(dir_out)/$(name).dat: $(dir_build)/main.bin $(dir_out)
	@mkdir -p $(dir_out)
	@$(MAKE) $(FLAGS) -C $(dir_mset) launcher
	dd if=$(dir_build)/main.bin of=$@ bs=512 seek=144

$(dir_out)/arm9loaderhax.bin: $(dir_build)/main.bin $(dir_out)
	@cp -av $(dir_build)/main.bin $@

$(dir_out)/3ds/$(name): $(dir_out)
	@mkdir -p $(dir_out)/3ds/$(name)
	@$(MAKE) $(FLAGS) -C $(dir_ninjhax)
	@mv $(dir_out)/$(name).3dsx $@
	@mv $(dir_out)/$(name).smdh $@

$(dir_out)/$(name).zip: launcher a9lh ninjhax
	@cd $(dir_out) && zip -9 -r $(name) *

$(dir_build)/main.bin: $(dir_build)/main.elf
	$(OC) -S -O binary $< $@

$(dir_build)/main.elf: $(objects)
	# FatFs requires libgcc for __aeabi_uidiv
	$(CC) -nostartfiles $(LDFLAGS) -T linker.ld $(OUTPUT_OPTION) $^

$(dir_build)/%.o: $(dir_source)/%.c
	@mkdir -p "$(@D)"
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(dir_build)/%.o: $(dir_source)/%.s
	@mkdir -p "$(@D)"
	$(COMPILE.s) $(OUTPUT_OPTION) $<

$(dir_build)/fatfs/%.o: $(dir_source)/fatfs/%.c
	@mkdir -p "$(@D)"
	$(COMPILE.c) -mthumb -mthumb-interwork -Wno-unused-function $(OUTPUT_OPTION) $<

$(dir_build)/fatfs/%.o: $(dir_source)/fatfs/%.s
	@mkdir -p "$(@D)"
	$(COMPILE.s) -mthumb -mthumb-interwork $(OUTPUT_OPTION) $<
include $(call rwildcard, $(dir_build), *.d)
