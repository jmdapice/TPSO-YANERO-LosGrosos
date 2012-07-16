################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../rc/src/my_engine.c 

OBJS += \
./rc/src/my_engine.o 

C_DEPS += \
./rc/src/my_engine.d 


# Each subdirectory must supply rules for building sources it contributes
rc/src/%.o: ../rc/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/home/utnso/memcached-1.6/include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


