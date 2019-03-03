include $(BUILD_DIR)/Header.mk

MODULE  := monkeyKingTest
SRC_DIR :=
DEFINE  :=
INCLUDE := -I$(BASE_DIR)/src/include
LDFLAGS :=
LIBS    := -L$(LIBS_DIR) -lmonkeyKing -pthread -lrt
EXECUTABLE_LDFLAGS := -Wl,-rpath="out/lib"
EXECUTABLE_LIBS := -lgtest -Wl,--whole-archive -lfoundation_test -Wl,--no-whole-archive


$(call build-target, executable)


include $(BUILD_DIR)/Footer.mk
