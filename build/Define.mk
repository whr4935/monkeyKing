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
define set-subdirs-target
$(if $(subdirs),.PHONY:$(subdirs))
$(if $(subdirs),$(subdirs):
	@echo -n
	@if [ -f $$@/Makefile ];then \
	 $(MAKE) -C $$@ $$(TARGET) || exit $$?; \
	fi)
endef

define do-make-in-subdirs
.PHONY:build_target clean
build_target clean:
	@echo -n
	$(if $(subdirs), @$(MAKE) $(subdirs) TARGET=$$@)
$(set-subdirs-target)
endef

define make-in-subdirs
$(eval $(cur-subdirs)) \
$(eval $(do-make-in-subdirs))
endef

#########################################
define set-build-target
build_type := $(1)
.PHONY:build_target
build_target:$(1)
	@echo -n
	$(if $(subdirs), @$(MAKE) $(subdirs) TARGET=$$@)
$(set-subdirs-target)
endef

define build-target
$(eval $(cur-subdirs)) \
$(eval $(call set-build-target, $(1)))
endef


endif

