##############################################################################
# Build global options
# NOTE: Can be overridden externally.
#

# Compiler options here.
# -Wdouble-promotion -fno-omit-frame-pointer

GCCVERSIONGTEQ10 := $(shell expr `arm-none-eabi-gcc -dumpversion | cut -f1 -d.` \>= 10)
GCC_DIAG =  -Werror -Wno-error=unused-variable -Wno-error=format \
            -Wno-error=cpp  \
            -Wno-error=unused-function \
            -Wunused -Wpointer-arith \
            -Werror=sign-compare \
            -Wshadow -Wparentheses -fmax-errors=5 \
            -ftrack-macro-expansion=2 -Wno-error=strict-overflow -Wstrict-overflow=2 \
	    -Wvla-larger-than=128 -Wduplicated-branches -Wdangling-else \
            -Wformat-overflow=2

ifeq "$(GCCVERSIONGTEQ10)" "1"
    USE_CPPOPT = -Wno-volatile -Wno-error=deprecated-declarations
endif

ifeq ($(USE_OPT),)
  USE_OPT =  -Og  -ggdb3 -Wall -Wextra \
         -falign-functions=16 -fomit-frame-pointer \
          $(GCC_DIAG) -DTRACE
endif

ifeq ($(USE_OPT),)
  USE_OPT =  -Os  -flto  -Wall -Wextra \
	    -falign-functions=16 -fomit-frame-pointer \
	     $(GCC_DIAG)   \
            --specs=nano.specs \
            -DCH_DBG_STATISTICS=0 \
            -DCH_DBG_SYSTEM_STATE_CHECK=0 -DCH_DBG_ENABLE_CHECKS=0 \
            -DCH_DBG_ENABLE_ASSERTS=0 -DCH_DBG_ENABLE_STACK_CHECK=0 \
            -DCH_DBG_FILL_THREADS=0 \
            -DNOSHELL
endif


# C specific options here (added to USE_OPT).
ifeq ($(USE_COPT),)
  USE_COPT = -std=gnu11  -Wunsuffixed-float-constants -Wstrict-prototypes
endif

# C++ specific options here (added to USE_OPT).

USE_CPPOPT += -std=c++20 -fno-rtti -fno-exceptions -fno-threadsafe-statics


# Enable this if you want the linker to remove unused code and data
ifeq ($(USE_LINK_GC),)
  USE_LINK_GC = yes
endif

# Linker extra options here.
ifeq ($(USE_LDOPT),)
  USE_LDOPT = 
endif

# Enable this if you want link time optimizations (LTO)
ifeq ($(USE_LTO),)
  USE_LTO = yes
endif


# Enable this if you want to see the full log while compiling.
ifeq ($(USE_VERBOSE_COMPILE),)
  USE_VERBOSE_COMPILE = no
endif

# If enabled, this option makes the build process faster by not compiling
# modules not used in the current configuration.
ifeq ($(USE_SMART_BUILD),)
  USE_SMART_BUILD = yes
endif

#
# Build global options
##############################################################################

##############################################################################
# Architecture or project specific options
#

# Stack size to be allocated to the Cortex-M process stack. This stack is
# the stack used by the main() thread.
ifeq ($(USE_PROCESS_STACKSIZE),)
  USE_PROCESS_STACKSIZE = 0x400
endif

# Stack size to the allocated to the Cortex-M main/exceptions stack. This
# stack is used for processing interrupts and exceptions.
ifeq ($(USE_EXCEPTIONS_STACKSIZE),)
  USE_EXCEPTIONS_STACKSIZE = 0x400
endif

# Enables the use of FPU (no, softfp, hard).
ifeq ($(USE_FPU),)
  USE_FPU = no
endif

# FPU-related options.
ifeq ($(USE_FPU_OPT),)
  USE_FPU_OPT = -mfloat-abi=$(USE_FPU) -mfpu=fpv4-sp-d16
endif

#
# Architecture or project specific options
##############################################################################

##############################################################################
# Project, sources and paths
#

# Define project name here
PROJECT = ch
BOARD = NUCLEO432

# Target settings.
MCU  = cortex-m4

# Imported source files and paths.
MY_DIRNAME=../../../ChibiOS_21.11_stable
ifneq "$(wildcard $(MY_DIRNAME) )" ""
   RELATIVE=../../..
else
  RELATIVE=../..
endif

CHIBIOS  := $(RELATIVE)/$(notdir $(MY_DIRNAME))
CONFDIR  := ./cfg
BUILDDIR := ./build
DEPDIR   := ./.dep
DRIVER_SRC := ./CubeMx/Drivers/STM32L4xx_HAL_Driver/Src
VARIOUS = $(RELATIVE)/COMMON/various
USBD_LIB = $(VARIOUS)/Chibios-USB-Devices
COMMUNITY = $(VARIOUS)
ETL_LIB = := ../../../../etl/include
SSD1306DIR := $(CONFDIR)/GFXfonts
# Licensing files.
include $(CHIBIOS)/os/license/license.mk
# Startup files.
include $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/mk/startup_stm32l4xx.mk
# HAL-OSAL files (optional).
include $(CHIBIOS)/os/hal/hal.mk
include $(CHIBIOS)/os/hal/ports/STM32/STM32L4xx/platform_l432.mk
include cfg/board.mk
include $(CHIBIOS)/os/hal/osal/rt-nil/osal.mk
# RTOS files (optional).
include $(CHIBIOS)/os/rt/rt.mk
include $(CHIBIOS)/os/common/ports/ARMv7-M/compilers/GCC/mk/port.mk
# Auto-build files in ./source recursively.
include $(CHIBIOS)/tools/mk/autobuild.mk
# HAL contrib
include  $(COMMUNITY)/hal/ports/STM32/STM32L4x2/platform.mk
include  $(COMMUNITY)/hal/hal_community.mk
# Other files (optional).


# Define linker script file here
LDSCRIPT= $(STARTUPLD)/STM32L432xC.ld

# C sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CSRC = $(ALLCSRC) \
       $(CHIBIOS)/os/various/syscalls.c \
       $(VARIOUS)/stdutil.c \
       $(VARIOUS)/rtcAccess.c \
       $(VARIOUS)/printf.c \
       $(VARIOUS)/microrl/microrlShell.c \
       $(VARIOUS)/microrl/microrl.c  \
       $(VARIOUS)/ssd1306.c 


# C++ sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CPPSRC = $(ALLCPPSRC)

# List ASM source files here.
ASMSRC = $(ALLASMSRC)

# List ASM with preprocessor source files here.
ASMXSRC = $(ALLXASMSRC)

# Inclusion directories.
INCDIR = $(CONFDIR) $(ALLINC) $(VARIOUS) \
         $(USBD_LIB) $(ETL_LIB) $(SSD1306DIR)

# Define C warning options here.
CWARN = -Wall -Wextra -Wundef -Wstrict-prototypes

# Define C++ warning options here
CPPWARN = -Wall -Wextra -Wundef

#
# Compiler settings
##############################################################################
LD   = $(TRGT)g++
##############################################################################
# Start of user section
#

# List all user C define here, like -D_DEBUG=1
UDEFS = -DGIT_BRANCH="$(GIT_BRANCH)" -DGIT_TAG="$(GIT_TAG)" -DGIT_SHA="$(GIT_SHA)" \
        -DBUILD="$(BUILD)" -DDIO2_DIRECT

# Define ASM defines here
UADEFS =

# List all user directories here
UINCDIR =

# List the user directory to look for the libraries here
ULIBDIR =

# List all user libraries here
ULIBS = -lm -lstdc++

#
# End of user defines
##############################################################################

##############################################################################
# Common rules
#

RULESPATH = $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/mk
include $(RULESPATH)/arm-none-eabi.mk
include $(RULESPATH)/rules.mk
$(OBJS): $(CONFDIR)/board.h


$(CONFDIR)/board.h: $(CONFDIR)/board.cfg
	boardGen.pl --no-pp-line --no-adcp-in $<  $@


stflash: all
	@echo write $(BUILDDIR)/$(PROJECT).bin to flash memory
	/usr/bin/st-flash write  $(BUILDDIR)/$(PROJECT).bin 0x08000000
	@echo Done

flash: all
	@echo write $(BUILDDIR)/$(PROJECT).bin to flash memory
	bmpflash  $(BUILDDIR)/$(PROJECT).elf
	@echo Done
