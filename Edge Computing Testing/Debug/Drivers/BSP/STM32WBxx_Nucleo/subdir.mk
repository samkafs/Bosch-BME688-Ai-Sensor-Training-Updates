################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/Samkafs/STM32Cube/Repository/STM32Cube_FW_WB_V1.23.0/Drivers/BSP/P-NUCLEO-WB55.USBDongle/stm32wbxx_usb_dongle.c 

OBJS += \
./Drivers/BSP/STM32WBxx_Nucleo/stm32wbxx_usb_dongle.o 

C_DEPS += \
./Drivers/BSP/STM32WBxx_Nucleo/stm32wbxx_usb_dongle.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/BSP/STM32WBxx_Nucleo/stm32wbxx_usb_dongle.o: C:/Users/Samkafs/STM32Cube/Repository/STM32Cube_FW_WB_V1.23.0/Drivers/BSP/P-NUCLEO-WB55.USBDongle/stm32wbxx_usb_dongle.c Drivers/BSP/STM32WBxx_Nucleo/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_NUCLEO_32 -DUSE_HAL_DRIVER -DSTM32WB55xx -c -I../Core/Inc -IC:/Users/Samkafs/STM32Cube/Repository/STM32Cube_FW_WB_V1.23.0/Drivers/STM32WBxx_HAL_Driver/Inc -IC:/Users/Samkafs/STM32Cube/Repository/STM32Cube_FW_WB_V1.23.0/Drivers/STM32WBxx_HAL_Driver/Inc/Legacy -IC:/Users/Samkafs/STM32Cube/Repository/STM32Cube_FW_WB_V1.23.0/Drivers/BSP/P-NUCLEO-WB55.USBDongle -IC:/Users/Samkafs/STM32Cube/Repository/STM32Cube_FW_WB_V1.23.0/Drivers/CMSIS/Device/ST/STM32WBxx/Include -IC:/Users/Samkafs/STM32Cube/Repository/STM32Cube_FW_WB_V1.23.0/Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"Drivers/BSP/STM32WBxx_Nucleo/stm32wbxx_usb_dongle.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-BSP-2f-STM32WBxx_Nucleo

clean-Drivers-2f-BSP-2f-STM32WBxx_Nucleo:
	-$(RM) ./Drivers/BSP/STM32WBxx_Nucleo/stm32wbxx_usb_dongle.cyclo ./Drivers/BSP/STM32WBxx_Nucleo/stm32wbxx_usb_dongle.d ./Drivers/BSP/STM32WBxx_Nucleo/stm32wbxx_usb_dongle.o ./Drivers/BSP/STM32WBxx_Nucleo/stm32wbxx_usb_dongle.su

.PHONY: clean-Drivers-2f-BSP-2f-STM32WBxx_Nucleo

