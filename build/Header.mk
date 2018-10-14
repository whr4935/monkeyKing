ifndef HEADER_MK
HEADER_MK = 1

include $(BUILD_DIR)/Define.mk

CC=gcc
CXX=g++
AR=ar
COMMON_INC := -I$(BASE_DIR)/android -I$(BASE_DIR)/android/include -I$(BASE_DIR)/include -DHAVE_PTHREADS -fpermissive


MODULE  := 
SRC_DIR := 
DEFINE  :=
INCLUDE :=
LDFLAGS := 
LIBS    := 
EXECUTABLE_LDFLAGS :=
EXECUTABLE_LIBS :=


endif


