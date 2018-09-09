ifndef FOOTER_MK
FOOTER_MK = 1

ifeq ($(MODULE),)
 MODULE := $(notdir $(CURDIR))
 # MODULE := $(notdir $(shell pwd))
endif

CFLAGS := -g -std=c99 -fPIC
CXXFLAGS:= -g -std=gnu++14 -fPIC

vpath %.c $(SRC_DIR)
vpath %.cpp $(SRC_DIR)
SRC := $(foreach dir, $(SRC_DIR), $(wildcard $(dir)/*.c))
SRC += $(wildcard *.c)
SRC += $(foreach dir, $(SRC_DIR), $(wildcard $(dir)/*.cpp))
SRC += $(wildcard *.cpp)

OBJ_DIR := $(OBJS_DIR)/$(MODULE)

OBJ := $(SRC:.c=.o)
OBJ := $(OBJ:.cpp=.o)
OBJ := $(addprefix $(OBJ_DIR)/, $(notdir $(OBJ)))


plugin:$(PLUGIN_DIR)/lib$(MODULE).so

shared_library:$(LIBS_DIR)/lib$(MODULE).so

static_library:$(LIBS_DIR)/lib$(MODULE).a

executable:$(BUILD_DIR)/$(MODULE)

$(PLUGIN_DIR)/lib$(MODULE).so:$(OBJ)
	@$(CXX) -shared $(LDFLAGS) -o$@ $^ $(LIBS)
	@echo "  PLUGIN  \033[1m\033[32mlib$(MODULE).so\033[0m"

$(LIBS_DIR)/lib$(MODULE).so:$(OBJ)
	@$(CXX) -shared $(LDFLAGS) -o$@ $^ $(LIBS)
	@echo "  LINK    \033[1m\033[32mlib$(MODULE).so\033[0m"

$(LIBS_DIR)/lib$(MODULE).a:$(OBJ)
	@$(AR) -r $@ $^
	@echo "  AR      \033[1m\033[32mlib$(MODULE).a\033[0m"

$(BUILD_DIR)/$(MODULE):$(OBJ)
	@$(CXX) $(LDFLAGS) -o$@ $^ $(LIBS)
	@echo "  BUILD   \033[1m\033[32m$(MODULE)\033[0m"


-include $(OBJ:.o=.dep)

$(OBJ_DIR):
	@mkdir $(OBJ_DIR)

$(OBJ_DIR)/%.o:%.c |$(OBJ_DIR)
	@$(CC) $(CFLAGS) $(DEFINE) $(COMMON_INC) $(INCLUDE) -c $< -o $@ -MD -MF $(@:.o=.dep)
	@echo "  CC      $<"

$(OBJ_DIR)/%.o:%.cpp |$(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(DEFINE) $(COMMON_INC) $(INCLUDE) -c $< -o $@ -MD -MF $(@:.o=.dep)
	@echo "  CXX     $<"

$(eval $(cur-subdirs))
clean:
	@for dir in $(subdirs);do \
		if [ -f $$dir/Makefile ];then \
		 $(MAKE) -C$$dir clean || exit "$$?"; \
		fi; \
	done;
	-rm -rf $(OBJ_DIR)
	@if  [ $(build_type) = 'executable' ];then \
		target=$(BUILD_DIR)/$(MODULE); \
	elif [ $(build_type) = 'shared_library' ];then \
		target=$(LIBS_DIR)/lib$(MODULE).so; \
	elif [ $(build_type) = 'plugin' ];then \
		target=$(PLUGIN_DIR)/lib$(MODULE).so; \
	elif [ $(build_type) = 'static_library' ];then \
		target=$(LIBS_DIR)/lib$(MODULE).a; \
	else \
		target=""; \
	fi; \
	 if  [ -n "$$target" ];then \
		echo rm -rf $$target; \
		rm -rf $$target; \
	fi

install_plugin:
	-cp $(PLUGIN_DIR)/lib$(MODULE).so ~/.silentdream/plugins/ -rf


endif
