ifndef DEFINE_MK
export DEFINE_MK = 1

export BASE_DIR := $(shell pwd)
export BUILD_DIR := $(BASE_DIR)/build
export LIBS_DIR := $(BUILD_DIR)/lib
export OBJS_DIR := $(BUILD_DIR)/objs
#export MAKE := make --no-print-directory

endif

ifndef DEFINE_MACRO
DEFINE_MACRO=1

define cur-subdirs
	$(eval subdirs :=$(shell find . -maxdepth 1 -type d))
	$(eval subdirs :=$(basename $(patsubst ./%,%,$(subdirs))))
endef

define make-in-subdirs2
for dir in $(2);do \
	if [ -f $$$$dir/Makefile ];then \
	 $(MAKE) -C$$$$dir $(1) || exit "$$$$?"; \
	fi; \
done;
endef

define make-in-subdirs 
target clean:; 
	$(call cur-subdirs)
	@$(call make-in-subdirs2, $$@, $(subdirs))
endef

define set-build-target
target:$(1)
	@echo -n
	@$(call make-in-subdirs2, $$@, $(subdirs))

build_type := $(1)
endef

define build-target
$(eval $(cur-subdirs)) \
$(eval $(call set-build-target, $(1)))
endef


endif

