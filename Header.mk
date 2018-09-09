ifndef HEADER_MK
HEADER_MK = 1

include $(BASE_DIR)/Define.mk

CC=gcc
CXX=g++
AR=ar
COMMON_INC := -I$(BASE_DIR) -I$(BASE_DIR)/include -DHAVE_PTHREADS -fpermissive


MODULE  := 
SRC_DIR := 
DEFINE  :=
INCLUDE :=
LDFLAGS := 
LIBS    := 


endif


