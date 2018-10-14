ifndef DEFINE_MK
export DEFINE_MK = 1

export BASE_DIR := $(shell pwd)
export BUILD_DIR := $(BASE_DIR)/build
export OUT_DIR := $(BASE_DIR)/out
export LIBS_DIR := $(OUT_DIR)/lib
export OBJS_DIR := $(OUT_DIR)/objs
#export MAKE := make --no-print-directory

endif

ifndef DEFINE_MACRO
DEFINE_MACRO=1

define cur-subdirs
	$(eval subdirs :=$(shell find . -maxdepth 1 -type d))
	$(eval subdirs :=$(basename $(patsubst ./%,%,$(subdirs))))
endef

#########################################
define make-in-subdir
	@if [ -f $@/Makefile ];then \
	 $(MAKE) -C $@ $(TARGET) || exit $$?; \
	fi
endef

define make-in-subdirs 
$(cur-subdirs)
build_target clean: 
	@if [ -n "$$(subdirs)" ]; then $(MAKE) $$(subdirs) TARGET=$$@; fi

.PHONY:build_target clean
$(if $$(subdirs), .PHONY:$$(subdirs))
$(if $$(subdirs), $$(subdirs):;$$(make-in-subdir))
endef

#########################################
define set-build-target
build_target:$(1)
	@echo -n
	$(if $(subdirs), @$(MAKE) $(subdirs) TARGET=$$@)

.PHONY:build_target clean
$(if $$(subdirs), .PHONY:$$(subdirs))
$(if $$(subdirs), $$(subdirs):;$$(make-in-subdir))

build_type := $(1)
endef


define build-target
$(eval $(cur-subdirs)) \
$(eval $(call set-build-target, $(1)))
endef


endif

