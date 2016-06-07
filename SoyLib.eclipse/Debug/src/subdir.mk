################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/media/graham/Shared/SoyLib/src/SoyTypes.cpp \
/media/graham/Shared/SoyLib/src/SoyVector.cpp 

OBJS += \
./src/SoyTypes.o \
./src/SoyVector.o 

CPP_DEPS += \
./src/SoyTypes.d \
./src/SoyVector.d 


# Each subdirectory must supply rules for building sources it contributes
src/SoyTypes.o: /media/graham/Shared/SoyLib/src/SoyTypes.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	clang++-3.5 -std=c++11 -DTARGET_LINUX -D__cplusplus11=201103L -D__GXX_EXPERIMENTAL_CXX0X__ -O0 -g3 -c -fmessage-length=0 -fpermissive -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/SoyVector.o: /media/graham/Shared/SoyLib/src/SoyVector.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	clang++-3.5 -std=c++11 -DTARGET_LINUX -D__cplusplus11=201103L -D__GXX_EXPERIMENTAL_CXX0X__ -O0 -g3 -c -fmessage-length=0 -fpermissive -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


