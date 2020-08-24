######################################
# target
######################################
TARGET = hover

ifneq "$(findstring AT32, $(MAKECMDGOALS))" ""
TARGET_MCU = AT32
else ifneq "$(findstring STM32, $(MAKECMDGOALS))" ""
TARGET_MCU = STM32
else
TARGET_MCU = AT32
endif

######################################
# building variables
######################################
# debug build?
DEBUG = 1
# optimization
OPT = -Og

# Build path
BUILD_DIR = build

######################################
# source
######################################
# C sources
C_SOURCES =  \
Drivers/Modified_HAL_Driver/Src/stm32f1xx_hal_flash.c \
Drivers/Modified_HAL_Driver/Src/stm32f1xx_hal_flash_ex.c \
Drivers/Modified_HAL_Driver/Src/stm32f1xx_hal_rcc.c \
Drivers/Modified_HAL_Driver/Src/stm32f1xx_hal_tim.c \
Drivers/Modified_HAL_Driver/Src/stm32f1xx_hal_tim_ex.c \
Drivers/Modified_HAL_Driver/Src/stm32f1xx_hal_gpio_ex.c \
Drivers/Modified_HAL_Driver/Src/stm32f1xx_hal_adc_ex.c \
Drivers/Modified_HAL_Driver/Src/stm32f1xx_hal_cortex.c \
Drivers/Modified_HAL_Driver/Src/stm32f1xx_hal_gpio.c \
Drivers/Modified_HAL_Driver/Src/stm32f1xx_hal_rcc_ex.c \
Drivers/Modified_HAL_Driver/Src/stm32f1xx_hal_pwr.c \
Drivers/Modified_HAL_Driver/Src/stm32f1xx_hal.c \
Drivers/Modified_HAL_Driver/Src/stm32f1xx_hal_adc.c \
Drivers/Modified_HAL_Driver/Src/stm32f1xx_hal_uart.c \
Drivers/Modified_HAL_Driver/Src/stm32f1xx_hal_i2c.c \
Drivers/Modified_HAL_Driver/Src/stm32f1xx_hal_dma.c \
Src/setup.c \
Src/control.c \
Src/main.c \
Src/bldc.c \
Src/comms.c \
Src/stm32f1xx_it.c \
Src/BLDC_controller_data.c \
Src/BLDC_controller.c \

ifeq ($(TARGET_MCU), AT32)
C_SOURCES := $(C_SOURCES) Src/system_at32f4xx.c
else
C_SOURCES := $(C_SOURCES) Src/system_stm32f1xx.c
endif

# ASM sources
ifeq ($(TARGET_MCU), AT32)
ASM_SOURCES =  \
startup_at32f403xe.s
else
ASM_SOURCES =  \
startup_stm32f103xe.s
endif

#######################################
# binaries
#######################################
PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
AR = $(PREFIX)ar
SZ = $(PREFIX)size
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S
TOUCH = echo "" > 
RM = del /q

#######################################
# CFLAGS
#######################################
# cpu
ifeq ($(TARGET_MCU),AT32)
	CPU = -mcpu=cortex-m4

	# fpu
	FPU=-mfpu=fpv4-sp-d16
	# float-abi
	FLOAT-ABI=-mfloat-abi=softfp
else
	CPU = -mcpu=cortex-m3
	# fpu
	# NONE for Cortex-M0/M0+/M3

	# float-abi
endif


# mcu
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# macros for gcc
# AS defines
AS_DEFS =

# C defines
ifeq ($(TARGET_MCU), AT32)
C_DEFS =  \
-DUSE_HAL_DRIVER \
-DAT32F403Rx_HD
else
C_DEFS =  \
-DUSE_HAL_DRIVER \
-DSTM32F103xE
endif

ifneq ("$(wildcard ./Inc/custom_config.h)","")
	CUSTOM_CONFIG = 1
	C_DEFS := $(C_DEFS) -DCUSTOM_CONFIG 
else
	CUSTOM_CONFIG = 0
	C_DEFS := $(C_DEFS)
endif

# AS includes
AS_INCLUDES =

# C includes
C_INCLUDES =  \
-IInc \
-IDrivers/Modified_HAL_Driver/Inc \
-IDrivers/CMSIS/CM4/DeviceSupport \
-IDrivers/CMSIS/CM4/CoreSupport \

#-IDrivers/AT32F4xx_StdPeriph_Driver/Inc \

# compile gcc flags
ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections -std=gnu11

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif


# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)"


#######################################
# LDFLAGS
#######################################
# link script
ifeq ($(TARGET_MCU), AT32)
LDSCRIPT = AT32F403RCTx_FLASH.ld
else
LDSCRIPT = STM32F103RCTx_FLASH.ld
endif

# libraries
LIBS = -lc -lm -lnosys
LIBDIR =
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

# default action: build all
all: post-build

pre-build: 
	@echo Build for target [$(TARGET_MCU)] 
	@echo Using custom configuration [$(CUSTOM_CONFIG)]
ifeq ($(TARGET_MCU), AT32)
ifneq ("$(wildcard ./build/target_stm32)","")
	@echo Found files from a previous compilation for STM32, clean them up now
	$(RM) build\*
	$(TOUCH) ./build/target_at32
endif
ifeq ("$(wildcard ./build/target_at32)","")
	@echo Create target lock file for AT32
	$(TOUCH) ./build/target_at32
endif
else
ifneq ("$(wildcard ./build/target_at32)","")
	@echo Found files from a previous compilation for AT32, clean them up now
	$(RM) build\*
	$(TOUCH) ./build/target_stm32
endif
ifeq ("$(wildcard ./build/target_stm32)","")
	@echo Create target lock file for STM32
	$(TOUCH) ./build/target_stm32
endif
endif		

post-build: main-build
	@echo POST

main-build: pre-build $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin
#@$(MAKE) --no-print-directory target

target: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

#######################################
# build the application
#######################################
# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Inc/config.h Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@ -include Inc/stm32f1xx_hal_conf.h

$(BUILD_DIR)/%.o: %.s Inc/config.h Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@

$(BUILD_DIR):
	mkdir -p $@

format:
	find Src/ Inc/ -iname '*.h' -o -iname '*.c' | xargs clang-format -i
#######################################
# clean up
#######################################
clean:
	-rm -fR .dep $(BUILD_DIR)

flash:
	st-flash --reset write $(BUILD_DIR)/$(TARGET).bin 0x8000000
flash-at32:
	openocd -f interface/stlink-v2.cfg -f target/stm32f3x.cfg -c init -c "reset halt" -c "stm32f1x unlock 0" -c "flash write_image erase $(BUILD_DIR)/$(TARGET).hex 0 ihex" -c "shutdown"

flash-jlink:
	 JLink.exe -if swd -device Cortex-M4 -speed 4000 -SettingsFile .\JLinkSettings.ini -CommanderScript jlink-command.jlink

unlock:
	openocd -f interface/stlink-v2.cfg -f target/stm32f1x.cfg -c init -c "reset halt" -c "stm32f1x unlock 0"

#empty target for MCU selection
AT32: all
	@echo Build for target [$(TARGET_MCU)] 
	
STM32: all
	@echo Build for target [$(TARGET_MCU)] 

#######################################
# dependencies
#######################################
-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)

# *** EOF ***
