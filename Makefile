include $(BUILD_DIR)/Define.mk

all:

TARGET_DIRS:= $(OBJS_DIR) $(LIBS_DIR)
$(eval $(make-in-subdirs))

all:$(TARGET_DIRS) build_target

main:android

$(TARGET_DIRS):
	@-mkdir -p $@ 

.PHONY:all 
