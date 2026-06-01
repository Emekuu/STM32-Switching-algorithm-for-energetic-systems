################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/emecc/STM32Cube/Example/BLE_DataThroughput/STM32_WPAN/App/app_ble.c \
C:/Users/emecc/STM32Cube/Example/BLE_DataThroughput/STM32_WPAN/App/dt_client_app.c \
C:/Users/emecc/STM32Cube/Example/BLE_DataThroughput/STM32_WPAN/App/dt_server_app.c \
C:/Users/emecc/STM32Cube/Example/BLE_DataThroughput/STM32_WPAN/App/dts.c \
../Application/STM32_WPAN/app/eth_parser.c 

OBJS += \
./Application/STM32_WPAN/app/app_ble.o \
./Application/STM32_WPAN/app/dt_client_app.o \
./Application/STM32_WPAN/app/dt_server_app.o \
./Application/STM32_WPAN/app/dts.o \
./Application/STM32_WPAN/app/eth_parser.o 

C_DEPS += \
./Application/STM32_WPAN/app/app_ble.d \
./Application/STM32_WPAN/app/dt_client_app.d \
./Application/STM32_WPAN/app/dt_server_app.d \
./Application/STM32_WPAN/app/dts.d \
./Application/STM32_WPAN/app/eth_parser.d 


# Each subdirectory must supply rules for building sources it contributes
Application/STM32_WPAN/app/app_ble.o: C:/Users/emecc/STM32Cube/Example/BLE_DataThroughput/STM32_WPAN/App/app_ble.c Application/STM32_WPAN/app/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DUSE_STM32WBXX_NUCLEO -DCORE_CM4 -DDEBUG -DSTM32WB55xx -c -I../../Middlewares/ST/STM32_WPAN/ble/core/template -I../../Middlewares/ST/STM32_WPAN/ble/core/auto -I../../Middlewares/ST/STM32_WPAN -I../../Drivers/CMSIS/Device/ST/STM32WBxx/Include -I../../Drivers/STM32WBxx_HAL_Driver/Inc -I../../Drivers/BSP/P-NUCLEO-WB55.Nucleo -I../../Middlewares/ST/STM32_WPAN/ble/core -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/tl -I../../Middlewares/ST/STM32_WPAN/utilities -I../../Utilities/lpm/tiny_lpm -I../../Utilities/sequencer -I../../Drivers/CMSIS/Include -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/shci -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread -I../../Middlewares/ST/STM32_WPAN/ble -I../../Middlewares/ST/STM32_WPAN/ble/mesh/Inc -I/STM32_WPAN/App -I/Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Application/STM32_WPAN/app/dt_client_app.o: C:/Users/emecc/STM32Cube/Example/BLE_DataThroughput/STM32_WPAN/App/dt_client_app.c Application/STM32_WPAN/app/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DUSE_STM32WBXX_NUCLEO -DCORE_CM4 -DDEBUG -DSTM32WB55xx -c -I../../Middlewares/ST/STM32_WPAN/ble/core/template -I../../Middlewares/ST/STM32_WPAN/ble/core/auto -I../../Middlewares/ST/STM32_WPAN -I../../Drivers/CMSIS/Device/ST/STM32WBxx/Include -I../../Drivers/STM32WBxx_HAL_Driver/Inc -I../../Drivers/BSP/P-NUCLEO-WB55.Nucleo -I../../Middlewares/ST/STM32_WPAN/ble/core -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/tl -I../../Middlewares/ST/STM32_WPAN/utilities -I../../Utilities/lpm/tiny_lpm -I../../Utilities/sequencer -I../../Drivers/CMSIS/Include -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/shci -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread -I../../Middlewares/ST/STM32_WPAN/ble -I../../Middlewares/ST/STM32_WPAN/ble/mesh/Inc -I/STM32_WPAN/App -I/Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Application/STM32_WPAN/app/dt_server_app.o: C:/Users/emecc/STM32Cube/Example/BLE_DataThroughput/STM32_WPAN/App/dt_server_app.c Application/STM32_WPAN/app/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DUSE_STM32WBXX_NUCLEO -DCORE_CM4 -DDEBUG -DSTM32WB55xx -c -I../../Middlewares/ST/STM32_WPAN/ble/core/template -I../../Middlewares/ST/STM32_WPAN/ble/core/auto -I../../Middlewares/ST/STM32_WPAN -I../../Drivers/CMSIS/Device/ST/STM32WBxx/Include -I../../Drivers/STM32WBxx_HAL_Driver/Inc -I../../Drivers/BSP/P-NUCLEO-WB55.Nucleo -I../../Middlewares/ST/STM32_WPAN/ble/core -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/tl -I../../Middlewares/ST/STM32_WPAN/utilities -I../../Utilities/lpm/tiny_lpm -I../../Utilities/sequencer -I../../Drivers/CMSIS/Include -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/shci -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread -I../../Middlewares/ST/STM32_WPAN/ble -I../../Middlewares/ST/STM32_WPAN/ble/mesh/Inc -I/STM32_WPAN/App -I/Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Application/STM32_WPAN/app/dts.o: C:/Users/emecc/STM32Cube/Example/BLE_DataThroughput/STM32_WPAN/App/dts.c Application/STM32_WPAN/app/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DUSE_STM32WBXX_NUCLEO -DCORE_CM4 -DDEBUG -DSTM32WB55xx -c -I../../Middlewares/ST/STM32_WPAN/ble/core/template -I../../Middlewares/ST/STM32_WPAN/ble/core/auto -I../../Middlewares/ST/STM32_WPAN -I../../Drivers/CMSIS/Device/ST/STM32WBxx/Include -I../../Drivers/STM32WBxx_HAL_Driver/Inc -I../../Drivers/BSP/P-NUCLEO-WB55.Nucleo -I../../Middlewares/ST/STM32_WPAN/ble/core -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/tl -I../../Middlewares/ST/STM32_WPAN/utilities -I../../Utilities/lpm/tiny_lpm -I../../Utilities/sequencer -I../../Drivers/CMSIS/Include -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/shci -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread -I../../Middlewares/ST/STM32_WPAN/ble -I../../Middlewares/ST/STM32_WPAN/ble/mesh/Inc -I/STM32_WPAN/App -I/Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Application/STM32_WPAN/app/%.o Application/STM32_WPAN/app/%.su Application/STM32_WPAN/app/%.cyclo: ../Application/STM32_WPAN/app/%.c Application/STM32_WPAN/app/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DUSE_STM32WBXX_NUCLEO -DCORE_CM4 -DDEBUG -DSTM32WB55xx -c -I../../Middlewares/ST/STM32_WPAN/ble/core/template -I../../Middlewares/ST/STM32_WPAN/ble/core/auto -I../../Middlewares/ST/STM32_WPAN -I../../Drivers/CMSIS/Device/ST/STM32WBxx/Include -I../../Drivers/STM32WBxx_HAL_Driver/Inc -I../../Drivers/BSP/P-NUCLEO-WB55.Nucleo -I../../Middlewares/ST/STM32_WPAN/ble/core -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/tl -I../../Middlewares/ST/STM32_WPAN/utilities -I../../Utilities/lpm/tiny_lpm -I../../Utilities/sequencer -I../../Drivers/CMSIS/Include -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/shci -I../../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread -I../../Middlewares/ST/STM32_WPAN/ble -I../../Middlewares/ST/STM32_WPAN/ble/mesh/Inc -I/STM32_WPAN/App -I/Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Application-2f-STM32_WPAN-2f-app

clean-Application-2f-STM32_WPAN-2f-app:
	-$(RM) ./Application/STM32_WPAN/app/app_ble.cyclo ./Application/STM32_WPAN/app/app_ble.d ./Application/STM32_WPAN/app/app_ble.o ./Application/STM32_WPAN/app/app_ble.su ./Application/STM32_WPAN/app/dt_client_app.cyclo ./Application/STM32_WPAN/app/dt_client_app.d ./Application/STM32_WPAN/app/dt_client_app.o ./Application/STM32_WPAN/app/dt_client_app.su ./Application/STM32_WPAN/app/dt_server_app.cyclo ./Application/STM32_WPAN/app/dt_server_app.d ./Application/STM32_WPAN/app/dt_server_app.o ./Application/STM32_WPAN/app/dt_server_app.su ./Application/STM32_WPAN/app/dts.cyclo ./Application/STM32_WPAN/app/dts.d ./Application/STM32_WPAN/app/dts.o ./Application/STM32_WPAN/app/dts.su ./Application/STM32_WPAN/app/eth_parser.cyclo ./Application/STM32_WPAN/app/eth_parser.d ./Application/STM32_WPAN/app/eth_parser.o ./Application/STM32_WPAN/app/eth_parser.su

.PHONY: clean-Application-2f-STM32_WPAN-2f-app

