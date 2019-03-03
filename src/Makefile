include $(BUILD_DIR)/Define.mk


MODULE  := monkeyKing
SRC_DIR :=
DEFINE  :=
INCLUDE := 
LDFLAGS := 
LIBS    :=  -Wl,-whole-archive $(LIBS_DIR)/libfoundation.a $(LIBS_DIR)/libutils.a  -Wl,-no-whole-archive

$(call build-target, shared_library)

shared_library: foundation utils
foundation:utils

include $(BUILD_DIR)/Footer.mk

