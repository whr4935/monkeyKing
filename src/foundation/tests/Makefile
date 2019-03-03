include $(BUILD_DIR)/Header.mk

MODULE  := foundation_test
SRC_DIR :=
DEFINE  :=
INCLUDE := -I$(BASE_DIR)/src/include/foundation
LDFLAGS :=
LIBS    := -lgtest
EXECUTABLE_LDFLAGS := -Wl,-rpath="out/lib"
EXECUTABLE_LIBS := -lgtest_main


$(call build-target, static_library executable)


include $(BUILD_DIR)/Footer.mk
