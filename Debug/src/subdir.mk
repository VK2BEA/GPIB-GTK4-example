################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/GPIB_GTK4-demo.c \
../src/GPIB_source.c \
../src/instrument.c \
../src/utility.c 

C_DEPS += \
./src/GPIB_GTK4-demo.d \
./src/GPIB_source.d \
./src/instrument.d \
./src/utility.d 

OBJS += \
./src/GPIB_GTK4-demo.o \
./src/GPIB_source.o \
./src/instrument.o \
./src/utility.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/usr/include/gtk-4.0 -I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include -O0 -g3 -Wall -c -fmessage-length=0 `pkg-config --cflags gtk4` -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/GPIB_GTK4-demo.d ./src/GPIB_GTK4-demo.o ./src/GPIB_source.d ./src/GPIB_source.o ./src/instrument.d ./src/instrument.o ./src/utility.d ./src/utility.o

.PHONY: clean-src

