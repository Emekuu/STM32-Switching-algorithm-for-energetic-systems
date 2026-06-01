################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/emecc/STM32Cube/Example/BLE_DataThroughput/Drivers/BSP/P-NUCLEO-WB55.Nucleo/stm32wbxx_nucleo.c 

OBJS += \
./Drivers/BSP/P-NUCLEO-WB55.Nucleo/stm32wbxx_nucleo.o 

C_DEPS += \
./Drivers/BSP/P-NUCLEO-WB55.Nucleo/stm32wbxx_nucleo.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/BSP/P-NUCLEO-WB55.Nucleo/stm32wbxx_nucleo.o: C:/Users/emecc/STM32Cube/Example/BLE_DataThroughput/Drivers/BSP/P-NUCLEO-WB55.Nucleo/stm32wbxx_nucleo.c Drivers/BSP/P-NUCLEO-WB55.Nucleo/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DUSE_STM32WBXX_NUCLEO -DCORE_CM4 -DDEBUG -DSTM32WB55xx -c -I../../Middlewares/ST/STM32_WPAN/ble/core/template -I../../Middlewares/ST/STM32_WPAN/ble/core/auto -I../../Middlewares/ST/STM32_WPAN -I../../Drivers/CMSIS/Device/ST/STM32WBxx/Include -I../../Drivers/STM32WBxx_HAL_Driver/Inc -I../../Drivers/BSP/P-NUCLEO-WB55.Nucleo -I../../Middlewares/ST/STM32_WPAN/ble/core -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/tl -I../../Middlewares/ST/STM32_WPAN/utilities -I../../Utilities/lpm/tiny_lpm -I../../Utilities/sequencer -I../../Drivers/CMSIS/Include -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/shci -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread -I../../Middlewares/ST/STM32_WPAN/ble -I../../Core/Inc -I../../STM32_WPAN/App -I../../Middlewares/ST/STM32_WPAN/ble/mesh/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-BSP-2f-P-2d-NUCLEO-2d-WB55-2e-Nucleo

clean-Drivers-2f-BSP-2f-P-2d-NUCLEO-2d-WB55-2e-Nucleo:
	-$(RM) ./Drivers/BSP/P-NUCLEO-WB55.Nucleo/stm32wbxx_nucleo.cyclo ./Drivers/BSP/P-NUCLEO-WB55.Nucleo/stm32wbxx_nucleo.d ./Drivers/BSP/P-NUCLEO-WB55.Nucleo/stm32wbxx_nucleo.o ./Drivers/BSP/P-NUCLEO-WB55.Nucleo/stm32wbxx_nucleo.su

.PHONY: clean-Drivers-2f-BSP-2f-P-2d-NUCLEO-2d-WB55-2e-Nucleo

