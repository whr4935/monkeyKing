include Define.mk

TARGET_DIRS:= $(OBJS_DIR) $(LIBS_DIR)

FILTER_OUT_DIRS:=build include tmp

SUB_DIRS:=$(shell find . -maxdepth 1 -type d)
SUB_DIRS:=$(basename $(patsubst ./%,%,$(SUB_DIRS)))
SUB_DIRS:=$(filter-out $(FILTER_OUT_DIRS), $(SUB_DIRS))


all: TARGET:=target
all:$(TARGET_DIRS) $(SUB_DIRS)


main:utils foundation



clean: TARGET:=clean
clean:$(SUB_DIRS)

$(SUB_DIRS):
	@$(MAKE) -C$@ $(TARGET)

$(TARGET_DIRS):
	@-mkdir -p $@ 

.PHONY:all clean $(SUB_DIRS)
