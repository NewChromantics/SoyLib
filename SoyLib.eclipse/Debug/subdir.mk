################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/media/graham/Shared/SoyLib/src/SoyTypes.cpp 

OBJS += \
./SoyTypes.o 

CPP_DEPS += \
./SoyTypes.d 


# Each subdirectory must supply rules for building sources it contributes
SoyTypes.o: /media/graham/Shared/SoyLib/src/SoyTypes.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


