################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/subdir.mk
-include Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F/subdir.mk
-include Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/subdir.mk
-include Middlewares/Third_Party/FreeRTOS/Source/subdir.mk
-include Middlewares/Third_Party/FatFs/src/option/subdir.mk
-include Middlewares/Third_Party/FatFs/src/subdir.mk
-include FATFS/Target/subdir.mk
-include FATFS/App/subdir.mk
-include Drivers/STM32F4xx_HAL_Driver/Src/subdir.mk
-include Core/lvgl/src/widgets/label/subdir.mk
-include Core/lvgl/src/tick/subdir.mk
-include Core/lvgl/src/themes/subdir.mk
-include Core/lvgl/src/stdlib/builtin/subdir.mk
-include Core/lvgl/src/stdlib/subdir.mk
-include Core/lvgl/src/osal/subdir.mk
-include Core/lvgl/src/misc/cache/instance/subdir.mk
-include Core/lvgl/src/misc/cache/class/subdir.mk
-include Core/lvgl/src/misc/cache/subdir.mk
-include Core/lvgl/src/misc/subdir.mk
-include Core/lvgl/src/libs/bin_decoder/subdir.mk
-include Core/lvgl/src/libs/FT800-FT813/subdir.mk
-include Core/lvgl/src/layouts/grid/subdir.mk
-include Core/lvgl/src/layouts/flex/subdir.mk
-include Core/lvgl/src/layouts/subdir.mk
-include Core/lvgl/src/indev/subdir.mk
-include Core/lvgl/src/font/fmt_txt/subdir.mk
-include Core/lvgl/src/font/subdir.mk
-include Core/lvgl/src/draw/sw/blend/riscv_v/subdir.mk
-include Core/lvgl/src/draw/sw/blend/neon/subdir.mk
-include Core/lvgl/src/draw/sw/blend/subdir.mk
-include Core/lvgl/src/draw/sw/subdir.mk
-include Core/lvgl/src/draw/snapshot/subdir.mk
-include Core/lvgl/src/draw/sdl/subdir.mk
-include Core/lvgl/src/draw/renesas/dave2d/subdir.mk
-include Core/lvgl/src/draw/opengles/subdir.mk
-include Core/lvgl/src/draw/nxp/pxp/subdir.mk
-include Core/lvgl/src/draw/nxp/g2d/subdir.mk
-include Core/lvgl/src/draw/nema_gfx/subdir.mk
-include Core/lvgl/src/draw/nanovg/subdir.mk
-include Core/lvgl/src/draw/espressif/ppa/subdir.mk
-include Core/lvgl/src/draw/dma2d/subdir.mk
-include Core/lvgl/src/draw/convert/neon/subdir.mk
-include Core/lvgl/src/draw/convert/helium/subdir.mk
-include Core/lvgl/src/draw/convert/subdir.mk
-include Core/lvgl/src/draw/subdir.mk
-include Core/lvgl/src/display/subdir.mk
-include Core/lvgl/src/debugging/vg_lite_tvg/subdir.mk
-include Core/lvgl/src/debugging/test/subdir.mk
-include Core/lvgl/src/debugging/sysmon/subdir.mk
-include Core/lvgl/src/debugging/monkey/subdir.mk
-include Core/lvgl/src/core/subdir.mk
-include Core/lvgl/src/subdir.mk
-include Core/lvgl/porting/subdir.mk
-include Core/Startup/subdir.mk
-include Core/Src/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := DATA_COLLECTION_INTEGRAL
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
DATA_COLLECTION_INTEGRAL.elf \

MAP_FILES += \
DATA_COLLECTION_INTEGRAL.map \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
DATA_COLLECTION_INTEGRAL.list \


# All Target
all: main-build

# Main-build Target
main-build: DATA_COLLECTION_INTEGRAL.elf secondary-outputs

# Tool invocations
DATA_COLLECTION_INTEGRAL.elf DATA_COLLECTION_INTEGRAL.map: $(OBJS) $(USER_OBJS) E:\lvgl\lv_port_pc_vscode-master\stm32_port\STM32F407ZETX_FLASH.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-gcc -o "DATA_COLLECTION_INTEGRAL.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m4 -T"E:\lvgl\lv_port_pc_vscode-master\stm32_port\STM32F407ZETX_FLASH.ld" --specs=nosys.specs -Wl,-Map="DATA_COLLECTION_INTEGRAL.map" -Wl,--gc-sections -static --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

DATA_COLLECTION_INTEGRAL.list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "DATA_COLLECTION_INTEGRAL.list"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) DATA_COLLECTION_INTEGRAL.elf DATA_COLLECTION_INTEGRAL.list DATA_COLLECTION_INTEGRAL.map default.size.stdout
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents main-build fail-specified-linker-script-missing warn-no-linker-script-specified

-include ../makefile.targets
