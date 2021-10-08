LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libopus
LOCAL_SRC_FILES := libs/libopus.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/voice/include/audio
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libfreetype
LOCAL_SRC_FILES := libs/libfreetype.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/vendor/freetype/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libsanex
APP_DEBUG := false
APP_OPTIM := release
#LOCAL_STATIC_LIBRARIES := libfreetype
#LOCAL_C_INCLUDES := $(LOCAL_PATH)/vendor/freetype/include

# samp
FILE_LIST := $(wildcard $(LOCAL_PATH)/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/game/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/net/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/util/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/game/RW/RenderWare.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/gui/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/voice/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/voice/include/util/*.cpp)

# vendor
FILE_LIST += $(wildcard $(LOCAL_PATH)/vendor/inih/cpp/INIReader.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/vendor/inih/ini.c)
FILE_LIST += $(wildcard $(LOCAL_PATH)/vendor/RakNet/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/vendor/RakNet/SAMP/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/vendor/imgui/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/vendor/hash/md5.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/vendor/base64/*.c)

LOCAL_SRC_FILES := $(FILE_LIST:$(LOCAL_PATH)/%=%)

LOCAL_LDLIBS := -lm -llog -lc -lz -ljnigraphics -landroid
LOCAL_CPPFLAGS := -w -s -fvisibility=hidden -pthread -Wall -fpack-struct=1 -O2 -O3 -fPIC -g -std=c++14 -fexceptions

LOCAL_STATIC_LIBRARIES := libfreetype
LOCAL_STATIC_LIBRARIES := libopus

include $(BUILD_SHARED_LIBRARY)

#include $(LOCAL_PATH)/vendor/freetype/Android.mk