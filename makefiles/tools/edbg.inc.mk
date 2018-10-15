RIOT_EDBG = $(RIOTBASE)/dist/tools/edbg/edbg
EDBG ?= $(RIOT_EDBG)
FLASHER ?= $(EDBG)
#MZTODO COMPILE
#<<<<<<< HEAD
HEXFILE = $(BINFILE)
# Use USB serial number to select device when more than one is connected
# Use /dist/tools/usb-serial/list-ttys.sh to find out serial number.
#   Usage:
# export DEBUG_ADAPTER_ID="ATML..."
# BOARD=<board> make flash
ifneq (,$(DEBUG_ADAPTER_ID))
  EDBG_ARGS += --serial $(DEBUG_ADAPTER_ID)
endif

# Set offset according to IMAGE_OFFSET if it's defined
EDBG_ARGS += $(if $(IMAGE_OFFSET),--offset $(IMAGE_OFFSET))

FFLAGS ?= $(EDBG_ARGS) -t $(EDBG_DEVICE_TYPE) -b -v -p -f $(HEXFILE)
#=======
#OFLAGS ?= -O binary
#HEXFILE = $(ELFFILE:.elf=.bin)
#FFLAGS ?= $(EDBG_ARGS) -t $(EDBG_DEVICE_TYPE) -b -e -v -p -f $(HEXFILE)
#>>>>>>> Fix dependencies, make javascript example run

ifeq ($(RIOT_EDBG),$(FLASHER))
  FLASHDEPS += $(RIOT_EDBG)
endif
