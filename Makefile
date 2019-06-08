GO_EASY_ON_ME = 1

ARCHS = arm64

include $(THEOS)/makefiles/common.mk

TOOL_NAME = iousbpoc
iousbpoc_FILES = $(wildcard *.mm *.m *.c)
iousbpoc_CODESIGN_FLAGS = -Sent.xml
iousbpoc_PRIVATE_FRAMEWORKS = IOKit

include $(THEOS_MAKE_PATH)/tool.mk
