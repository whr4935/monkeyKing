LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
   renderer.cpp \
   MessageQueue.cpp


LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../pjproject/androidwrapper/include  \
					$(LOCAL_PATH)/../

LOCAL_SHARED_LIBRARIES := libpjandroidwrapper libfaceDetection libfaceViewjni

LOCAL_LDLIBS +=-llog -lEGL -lGLESv2 -landroid

LOCAL_MODULE:= librenderer


include $(BUILD_SHARED_LIBRARY)



