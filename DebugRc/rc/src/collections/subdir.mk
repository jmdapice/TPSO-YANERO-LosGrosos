################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../rc/src/collections/algoritmo_reemplazo.c \
../rc/src/collections/buddy.c \
../rc/src/collections/compactacion.c \
../rc/src/collections/logRc.c \
../rc/src/collections/miDiccionario.c 

OBJS += \
./rc/src/collections/algoritmo_reemplazo.o \
./rc/src/collections/buddy.o \
./rc/src/collections/compactacion.o \
./rc/src/collections/logRc.o \
./rc/src/collections/miDiccionario.o 

C_DEPS += \
./rc/src/collections/algoritmo_reemplazo.d \
./rc/src/collections/buddy.d \
./rc/src/collections/compactacion.d \
./rc/src/collections/logRc.d \
./rc/src/collections/miDiccionario.d 


# Each subdirectory must supply rules for building sources it contributes
rc/src/collections/%.o: ../rc/src/collections/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/home/utnso/memcached-1.6/include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


