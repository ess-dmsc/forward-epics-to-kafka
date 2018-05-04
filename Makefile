#Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG

# exclude Visual Studio before 2015 (which is 14.0 - there is no 13.0)
VS_OK=1
ifneq ($(findstring 10.0,$(VCVERSION)),)
VS_OK=0
endif
ifneq ($(findstring 11.0,$(VCVERSION)),)
VS_OK=0
endif
ifneq ($(findstring 12.0,$(VCVERSION)),)
VS_OK=0
endif
ifeq ($(VS_OK),1)
DIRS := $(DIRS) $(filter-out $(DIRS), configure)
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *App))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard iocBoot))
endif

define DIR_template
 $(1)_DEPEND_DIRS = configure
endef
$(foreach dir, $(filter-out configure,$(DIRS)),$(eval $(call DIR_template,$(dir))))

iocBoot_DEPEND_DIRS += $(filter %App,$(DIRS))

UNINSTALL_DIRS += $(TOP)/install

include $(TOP)/configure/RULES_TOP
