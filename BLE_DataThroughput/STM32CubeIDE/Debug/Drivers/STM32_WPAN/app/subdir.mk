################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/emecc/STM32Cube/Example/BLE_DataThroughput/STM32_WPAN/App/app_ble.c \
C:/Users/emecc/STM32Cube/Example/BLE_DataThroughput/STM32_WPAN/App/dt_client_app.c \
C:/Users/emecc/STM32Cube/Example/BLE_DataThroughput/STM32_WPAN/App/dt_server_app.c \
C:/Users/emecc/STM32Cube/Example/BLE_DataThroughput/STM32_WPAN/App/dts.c 

OBJS += \
./Drivers/STM32_WPAN/app/app_ble.o \
./Drivers/STM32_WPAN/app/dt_client_app.o \
./Drivers/STM32_WPAN/app/dt_server_app.o \
./Drivers/STM32_WPAN/app/dts.o 

C_DEPS += \
./Drivers/STM32_WPAN/app/app_ble.d \
./Drivers/STM32_WPAN/app/dt_client_app.d \
./Drivers/STM32_WPAN/app/dt_server_app.d \
./Drivers/STM32_WPAN/app/dts.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/STM32_WPAN/app/app_ble.o: C:/Users/emecc/STM32Cube/Example/BLE_DataThroughput/STM32_WPAN/App/app_ble.c Drivers/STM32_WPAN/app/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DUSE_STM32WBXX_NUCLEO -DCORE_CM4 -DDEBUG -DSTM32WB55xx -c -I../../Middlewares/ST/STM32_WPAN/ble/core/template -I../../Middlewares/ST/STM32_WPAN/ble/core/auto -I../../Middlewares/ST/STM32_WPAN -I../../Drivers/CMSIS/Device/ST/STM32WBxx/Include -I../../Drivers/STM32WBxx_HAL_Driver/Inc -I../../Drivers/BSP/P-NUCLEO-WB55.Nucleo -I../../Middlewares/ST/STM32_WPAN/ble/core -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/tl -I../../Middlewares/ST/STM32_WPAN/utilities -I../../Utilities/lpm/tiny_lpm -I../../Utilities/sequencer -I../../Drivers/CMSIS/Include -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/shci -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread -I../../Middlewares/ST/STM32_WPAN/ble -I../../Core/Inc -I../../STM32_WPAN/App -I../../Middlewares/ST/STM32_WPAN/ble/mesh/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Drivers/STM32_WPAN/app/dt_client_app.o: C:/Users/emecc/STM32Cube/Example/BLE_DataThroughput/STM32_WPAN/App/dt_client_app.c Drivers/STM32_WPAN/app/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DUSE_STM32WBXX_NUCLEO -DCORE_CM4 -DDEBUG -DSTM32WB55xx -c -I../../Middlewares/ST/STM32_WPAN/ble/core/template -I../../Middlewares/ST/STM32_WPAN/ble/core/auto -I../../Middlewares/ST/STM32_WPAN -I../../Drivers/CMSIS/Device/ST/STM32WBxx/Include -I../../Drivers/STM32WBxx_HAL_Driver/Inc -I../../Drivers/BSP/P-NUCLEO-WB55.Nucleo -I../../Middlewares/ST/STM32_WPAN/ble/core -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/tl -I../../Middlewares/ST/STM32_WPAN/utilities -I../../Utilities/lpm/tiny_lpm -I../../Utilities/sequencer -I../../Drivers/CMSIS/Include -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/shci -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread -I../../Middlewares/ST/STM32_WPAN/ble -I../../Core/Inc -I../../STM32_WPAN/App -I../../Middlewares/ST/STM32_WPAN/ble/mesh/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Drivers/STM32_WPAN/app/dt_server_app.o: C:/Users/emecc/STM32Cube/Example/BLE_DataThroughput/STM32_WPAN/App/dt_server_app.c Drivers/STM32_WPAN/app/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DUSE_STM32WBXX_NUCLEO -DCORE_CM4 -DDEBUG -DSTM32WB55xx -c -I../../Middlewares/ST/STM32_WPAN/ble/core/template -I../../Middlewares/ST/STM32_WPAN/ble/core/auto -I../../Middlewares/ST/STM32_WPAN -I../../Drivers/CMSIS/Device/ST/STM32WBxx/Include -I../../Drivers/STM32WBxx_HAL_Driver/Inc -I../../Drivers/BSP/P-NUCLEO-WB55.Nucleo -I../../Middlewares/ST/STM32_WPAN/ble/core -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/tl -I../../Middlewares/ST/STM32_WPAN/utilities -I../../Utilities/lpm/tiny_lpm -I../../Utilities/sequencer -I../../Drivers/CMSIS/Include -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/shci -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread -I../../Middlewares/ST/STM32_WPAN/ble -I../../Core/Inc -I../../STM32_WPAN/App -I../../Middlewares/ST/STM32_WPAN/ble/mesh/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Drivers/STM32_WPAN/app/dts.o: C:/Users/emecc/STM32Cube/Example/BLE_DataThroughput/STM32_WPAN/App/dts.c Drivers/STM32_WPAN/app/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DUSE_STM32WBXX_NUCLEO -DCORE_CM4 -DDEBUG -DSTM32WB55xx -c -I../../Middlewares/ST/STM32_WPAN/ble/core/template -I../../Middlewares/ST/STM32_WPAN/ble/core/auto -I../../Middlewares/ST/STM32_WPAN -I../../Drivers/CMSIS/Device/ST/STM32WBxx/Include -I../../Drivers/STM32WBxx_HAL_Driver/Inc -I../../Drivers/BSP/P-NUCLEO-WB55.Nucleo -I../../Middlewares/ST/STM32_WPAN/ble/core -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/tl -I../../Middlewares/ST/STM32_WPAN/utilities -I../../Utilities/lpm/tiny_lpm -I../../Utilities/sequencer -I../../Drivers/CMSIS/Include -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/shci -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread -I../../Middlewares/ST/STM32_WPAN/ble -I../../Core/Inc -I../../STM32_WPAN/App -I../../Middlewares/ST/STM32_WPAN/ble/mesh/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-STM32_WPAN-2f-app

clean-Drivers-2f-STM32_WPAN-2f-app:
	-$(RM) ./Drivers/STM32_WPAN/app/app_ble.cyclo ./Drivers/STM32_WPAN/app/app_ble.d ./Drivers/STM32_WPAN/app/app_ble.o ./Drivers/STM32_WPAN/app/app_ble.su ./Drivers/STM32_WPAN/app/dt_client_app.cyclo ./Drivers/STM32_WPAN/app/dt_client_app.d ./Drivers/STM32_WPAN/app/dt_client_app.o ./Drivers/STM32_WPAN/app/dt_client_app.su ./Drivers/STM32_WPAN/app/dt_server_app.cyclo ./Drivers/STM32_WPAN/app/dt_server_app.d ./Drivers/STM32_WPAN/app/dt_server_app.o ./Drivers/STM32_WPAN/app/dt_server_app.su ./Drivers/STM32_WPAN/app/dts.cyclo ./Drivers/STM32_WPAN/app/dts.d ./Drivers/STM32_WPAN/app/dts.o ./Drivers/STM32_WPAN/app/dts.su

.PHONY: clean-Drivers-2f-STM32_WPAN-2f-app

