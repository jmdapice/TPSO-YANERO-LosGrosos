################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../fsc/auxCliente.c \
../fsc/cacheInterface.c \
../fsc/fsc.c \
../fsc/serializar_y_deserializar.c 

OBJS += \
./fsc/auxCliente.o \
./fsc/cacheInterface.o \
./fsc/fsc.o \
./fsc/serializar_y_deserializar.o 

C_DEPS += \
./fsc/auxCliente.d \
./fsc/cacheInterface.d \
./fsc/fsc.d \
./fsc/serializar_y_deserializar.d 


# Each subdirectory must supply rules for building sources it contributes
fsc/%.o: ../fsc/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -O0 -g3 -Wall -DFUSE_USE_VERSION=27 -D_FILE_OFFSET_BITS=64 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


