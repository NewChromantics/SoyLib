################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/media/graham/Shared/SoyLib/src/smallsha1/sha1.cpp 

OBJS += \
./src/smallsha1/sha1.o 

CPP_DEPS += \
./src/smallsha1/sha1.d 


# Each subdirectory must supply rules for building sources it contributes
src/smallsha1/sha1.o: /media/graham/Shared/SoyLib/src/smallsha1/sha1.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	clang++-3.5 -std=c++11 -DTARGET_LINUX -D__cplusplus11=201103L -D__GXX_EXPERIMENTAL_CXX0X__ -O0 -g3 -c -fmessage-length=0 -fpermissive -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


