LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
	omx_rpc_config.c \
	omx_rpc_platform.c \
	omx_rpc_skel.c \
	omx_rpc_stub.c \
	omx_rpc_utils.c \
	omx_rpc.c

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../../ \
	$(LOCAL_PATH)/../../omx_rpc/inc \
	$(LOCAL_PATH)/../../../domx \
	$(LOCAL_PATH)/../../../omx_core/inc \
	$(LOCAL_PATH)/../../../mm_osal/inc \
	$(LOCAL_PATH)/../../../mmplatform \
	hardware/ti/tiler/memmgr \
	hardware/ti/tiler/d2c \
	hardware/ti/syslink/syslink/api/include \

LOCAL_CFLAGS := -pipe -fomit-frame-pointer -Wall  -Wno-trigraphs -Werror-implicit-function-declaration  -fno-strict-aliasing -mapcs -mno-sched-prolog -mabi=aapcs-linux -mno-thumb-interwork -msoft-float -Uarm -DMODULE -D__LINUX_ARM_ARCH__=7  -fno-common -DLINUX -fpic
LOCAL_CFLAGS += -D_Android

LOCAL_STATIC_LIBRARIES += \
		libipc_setup \

LOCAL_SHARED_LIBRARIES += \
		liblog \

LOCAL_MODULE:= libomx_rpc


include $(BUILD_STATIC_LIBRARY)
