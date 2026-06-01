################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/emecc/STM32Cube/Example/BLE_DataThroughput/STM32_WPAN/Target/hw_ipcc.c 

OBJS += \
./Application/STM32_WPAN/target/hw_ipcc.o 

C_DEPS += \
./Application/STM32_WPAN/target/hw_ipcc.d 


# Each subdirectory must supply rules for building sources it contributes
Application/STM32_WPAN/target/hw_ipcc.o: C:/Users/emecc/STM32Cube/Example/BLE_DataThroughput/STM32_WPAN/Target/hw_ipcc.c Application/STM32_WPAN/target/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DUSE_STM32WBXX_NUCLEO -DCORE_CM4 -DDEBUG -DSTM32WB55xx -c -I../../Middlewares/ST/STM32_WPAN/ble/core/template -I../../Middlewares/ST/STM32_WPAN/ble/core/auto -I../../Middlewares/ST/STM32_WPAN -I../../Drivers/CMSIS/Device/ST/STM32WBxx/Include -I../../Drivers/STM32WBxx_HAL_Driver/Inc -I../../Drivers/BSP/P-NUCLEO-WB55.Nucleo -I../../Middlewares/ST/STM32_WPAN/ble/core -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/tl -I../../Middlewares/ST/STM32_WPAN/utilities -I../../Utilities/lpm/tiny_lpm -I../../Utilities/sequencer -I../../Drivers/CMSIS/Include -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/shci -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread -I../../Middlewares/ST/STM32_WPAN/ble -I../../Middlewares/ST/STM32_WPAN/ble/mesh/Inc -I/STM32_WPAN/App -I/Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Application-2f-STM32_WPAN-2f-target

clean-Application-2f-STM32_WPAN-2f-target:
	-$(RM) ./Application/STM32_WPAN/target/hw_ipcc.cyclo ./Application/STM32_WPAN/target/hw_ipcc.d ./Application/STM32_WPAN/target/hw_ipcc.o ./Application/STM32_WPAN/target/hw_ipcc.su

.PHONY: clean-Application-2f-STM32_WPAN-2f-target

