SRC_UTILS = $(SRC_PATH)/utils

MAKEFILE_APP_VERSIONS = $(OUT_UTILS)/version.mk
MAKEFILE_APP_NAMES = $(OUT_UTILS)/name.mk
MAKEFILE_APP_DEPENDS = $(OUT_UTILS)/depend.mk
MAKEFILE_INSTALL = $(BUILD_UTILS)/install.mk

export SRC_UTILS OUT_UTILS
export MAKEFILE_APP_VERSIONS MAKEFILE_APP_NAMES MAKEFILE_APP_DEPENDS MAKEFILE_INSTALL

include $(MAKEFILE_DEFINES)

all: build_env
	$(Q)python $(PYTHON_PARSER) -v $(MAKEFILE_APP_VERSIONS) -n $(MAKEFILE_APP_NAMES) -d $(MAKEFILE_APP_DEPENDS) $(XML_APPLICATION)
	$(Q)+make -f $(MAKEFILE_INSTALL)

build_env:
	$(Q)mkdir $(SRC_UTILS) $(OUT_UTILS) -pv

.PHONY: build_env
