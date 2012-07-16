################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../rfs/Dir.c \
../rfs/Grupos.c \
../rfs/Inode.c \
../rfs/Inotify.c \
../rfs/Main.c \
../rfs/Rfs.c \
../rfs/Superblock.c \
../rfs/cacheInterface.c \
../rfs/serial_rfs.c \
../rfs/sincro.c 

OBJS += \
./rfs/Dir.o \
./rfs/Grupos.o \
./rfs/Inode.o \
./rfs/Inotify.o \
./rfs/Main.o \
./rfs/Rfs.o \
./rfs/Superblock.o \
./rfs/cacheInterface.o \
./rfs/serial_rfs.o \
./rfs/sincro.o 

C_DEPS += \
./rfs/Dir.d \
./rfs/Grupos.d \
./rfs/Inode.d \
./rfs/Inotify.d \
./rfs/Main.d \
./rfs/Rfs.d \
./rfs/Superblock.d \
./rfs/cacheInterface.d \
./rfs/serial_rfs.d \
./rfs/sincro.d 


# Each subdirectory must supply rules for building sources it contributes
rfs/%.o: ../rfs/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


