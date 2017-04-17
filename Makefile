rwildcard = $(foreach d, $(wildcard $1*), $(filter $(subst *, %, $2), $d) $(call rwildcard, $d/, $2))

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/base_tools

name := SafeA9LHInstaller
revision := $(shell git describe --tags --match v[0-9]* --abbrev=8 | sed 's/-[0-9]*-g/-/i')

dir_source := source
dir_cakehax := CakeHax
dir_cakebrah := CakeBrah
dir_build := build
dir_out := out

ASFLAGS := -mcpu=arm946e-s
CFLAGS := -Wall -Wextra -MMD -MP -marm $(ASFLAGS) -fno-builtin -fshort-wchar -std=c11 -Wno-main -O2 -flto -ffast-math
LDFLAGS := -nostartfiles
FLAGS := name=$(name).dat dir_out=$(abspath $(dir_out)) ICON=$(abspath icon.png) APP_DESCRIPTION="Noob-proof ARM9LoaderHax installer/updater." APP_AUTHOR="Aurora Wright" --no-print-directory

objects= $(patsubst $(dir_source)/%.s, $(dir_build)/%.o, \
         $(patsubst $(dir_source)/%.c, $(dir_build)/%.o, \
	 $(call rwildcard, $(dir_source), *.s *.c)))

.PHONY: all
all: launcher a9lh cakebrah

.PHONY: launcher
launcher: $(dir_out)/$(name).dat

.PHONY: a9lh
a9lh: $(dir_out)/arm9loaderhax.bin

.PHONY: cakebrah
cakebrah: $(dir_out)/3ds/$(name)

.PHONY: release
release: $(dir_out)/$(name)$(revision).7z

.PHONY: clean
clean:
	@$(MAKE) $(FLAGS) -C $(dir_cakehax) clean
	@$(MAKE) $(FLAGS) -C $(dir_cakebrah) clean
	@rm -rf $(dir_out) $(dir_build)

$(dir_out) $(dir_build):
	@mkdir -p "$@"

$(dir_out)/$(name).dat: $(dir_build)/main.bin $(dir_out)
	@$(MAKE) $(FLAGS) -C $(dir_cakehax) launcher
	dd if=$(dir_build)/main.bin of=$@ bs=512 seek=144

$(dir_out)/arm9loaderhax.bin: $(dir_build)/main.bin $(dir_out)
	@cp -av $(dir_build)/main.bin $@

$(dir_out)/3ds/$(name): $(dir_out)
	@mkdir -p "$@"
	@$(MAKE) $(FLAGS) -C $(dir_cakebrah)
	@mv $(dir_out)/$(name).3dsx $(dir_out)/$(name).smdh $@

$(dir_out)/$(name)$(revision).7z: all
	@7z a -mx $@ ./$(@D)/*

$(dir_build)/main.bin: $(dir_build)/main.elf
	$(OBJCOPY) -S -O binary $< $@

$(dir_build)/main.elf: $(objects)
	$(LINK.o) -T linker.ld $(OUTPUT_OPTION) $^

$(dir_build)/memory.o $(dir_build)/strings.o: CFLAGS += -O3
$(dir_build)/installer.o: CFLAGS += -DTITLE="\"$(name) $(revision)\""

$(dir_build)/%.o: $(dir_source)/%.c
	@mkdir -p "$(@D)"
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(dir_build)/%.o: $(dir_source)/%.s
	@mkdir -p "$(@D)"
	$(COMPILE.s) $(OUTPUT_OPTION) $<
