include $(BUILD_DIR)/Header.mk

MODULE  := monkeyKingTest
SRC_DIR :=
DEFINE  :=
INCLUDE :=
LDFLAGS :=
LIBS    := -L$(LIBS_DIR) -landroid_utils  -landroid_foundation -pthread -lrt
EXECUTABLE_LDFLAGS := -Wl,-rpath="out/lib"
EXECUTABLE_LIBS := -lgtest -Wl,--whole-archive -landroid_foundation_test -Wl,--no-whole-archive


$(call build-target, executable)


include $(BUILD_DIR)/Footer.mk
