# Project Name
TARGET = DaisySeedPaulstretch

CMSIS_DSP = ./lib/CMSIS-DSP
CFLAGS += -Ofast -ffast-math

C_SOURCES += $(CMSIS_DSP)/Source/BasicMathFunctions/BasicMathFunctions.c \
 $(CMSIS_DSP)/Source/ControllerFunctions/ControllerFunctions.c \
 $(CMSIS_DSP)/Source/FastMathFunctions/FastMathFunctions.c \
 $(CMSIS_DSP)/Source/TransformFunctions/TransformFunctions.c
# $(CMSIS_DSP)/Source/CommonTables/CommonTables.c
# $(CMSIS_DSP)/Source/InterpolationFunctions/InterpolationFunctions.c
# $(CMSIS_DSP)/Source/BayesFunctions/BayesFunctions.c
# $(CMSIS_DSP)/Source/MatrixFunctions/MatrixFunctions.c
# $(CMSIS_DSP)/Source/ComplexMathFunctions/ComplexMathFunctions.c
# $(CMSIS_DSP)/Source/QuaternionMathFunctions/QuaternionMathFunctions.c
# $(CMSIS_DSP)/Source/SVMFunctions/SVMFunctions.c
# $(CMSIS_DSP)/Source/DistanceFunctions/DistanceFunctions.c
# $(CMSIS_DSP)/Source/StatisticsFunctions/StatisticsFunctions.c
# $(CMSIS_DSP)/Source/SupportFunctions/SupportFunctions.c
# $(CMSIS_DSP)/Source/FilteringFunctions/FilteringFunctions.c
# $(CMSIS_DSP)/Source/WindowFunctions/WindowFunctions.c

C_INCLUDES += -I$(CMSIS_DSP)/Include \
  -I$(CMSIS_DSP)/PrivateInclude

# Sources
CPP_SOURCES = DaisySeedPaulstretch.cpp $(wildcard */*.cpp) $(wildcard */*.c)

# Library Locations
LIBDAISY_DIR = ./lib/libDaisy
DAISYSP_DIR = ./lib/DaisySP

CPP_STANDARD = -std=c++17

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
