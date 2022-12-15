export PJDIR := /home/mylinux/sigmastar_pjsip
include $(PJDIR)/version.mak
export PJ_DIR := $(PJDIR)

# build.mak.  Generated from build.mak.in by configure.
export MACHINE_NAME := auto
export OS_NAME := auto
export HOST_NAME := unix
export CC_NAME := gcc
export TARGET_ARCH := 
export TARGET_NAME := arm-unknown-linux-gnueabihf
export CROSS_COMPILE := arm-linux-gnueabihf-
export LINUX_POLL := select 
export SHLIB_SUFFIX := so

export prefix := /home/mylinux/pj
export exec_prefix := ${prefix}
export includedir := ${prefix}/include
export libdir := ${exec_prefix}/lib

LIB_SUFFIX := $(TARGET_NAME).a

ifeq (1,1)
export PJ_SHARED_LIBRARIES := 1
endif

ifeq (,1)
export PJ_EXCLUDE_PJSUA2 := 1
endif

ifndef EXCLUDE_APP
ifeq ($(findstring android,$(TARGET_NAME)),)
export EXCLUDE_APP := 0
else
export EXCLUDE_APP := 1
endif
endif

# Determine which party libraries to use
export APP_THIRD_PARTY_EXT :=
export APP_THIRD_PARTY_LIBS :=
export APP_THIRD_PARTY_LIB_FILES :=

ifneq (0,0)
# External SRTP library
APP_THIRD_PARTY_EXT += -l
else
APP_THIRD_PARTY_LIB_FILES += $(PJ_DIR)/third_party/lib/libsrtp-$(LIB_SUFFIX)
ifeq ($(PJ_SHARED_LIBRARIES),)
APP_THIRD_PARTY_LIBS += -lsrtp-$(TARGET_NAME)
else
APP_THIRD_PARTY_LIBS += -lsrtp
APP_THIRD_PARTY_LIB_FILES += $(PJ_DIR)/third_party/lib/libsrtp.$(SHLIB_SUFFIX).$(PJ_VERSION_MAJOR) $(PJ_DIR)/third_party/lib/libsrtp.$(SHLIB_SUFFIX)
endif
endif

ifeq (libresample,libresample)
APP_THIRD_PARTY_LIB_FILES += $(PJ_DIR)/third_party/lib/libresample-$(LIB_SUFFIX)
ifeq ($(PJ_SHARED_LIBRARIES),)
ifeq (,1)
export PJ_RESAMPLE_DLL := 1
APP_THIRD_PARTY_LIBS += -lresample
APP_THIRD_PARTY_LIB_FILES += $(PJ_DIR)/third_party/lib/libresample.$(SHLIB_SUFFIX).$(PJ_VERSION_MAJOR) $(PJ_DIR)/third_party/lib/libresample.$(SHLIB_SUFFIX)
else
APP_THIRD_PARTY_LIBS += -lresample-$(TARGET_NAME)
endif
else
APP_THIRD_PARTY_LIBS += -lresample
APP_THIRD_PARTY_LIB_FILES += $(PJ_DIR)/third_party/lib/libresample.$(SHLIB_SUFFIX).$(PJ_VERSION_MAJOR) $(PJ_DIR)/third_party/lib/libresample.$(SHLIB_SUFFIX)
endif
endif

ifneq (1,1)
ifeq (0,1)
# External GSM library
APP_THIRD_PARTY_EXT += -lgsm
else
APP_THIRD_PARTY_LIB_FILES += $(PJ_DIR)/third_party/lib/libgsmcodec-$(LIB_SUFFIX)
ifeq ($(PJ_SHARED_LIBRARIES),)
APP_THIRD_PARTY_LIBS += -lgsmcodec-$(TARGET_NAME)
else
APP_THIRD_PARTY_LIBS += -lgsmcodec
APP_THIRD_PARTY_LIB_FILES += $(PJ_DIR)/third_party/lib/libgsmcodec.$(SHLIB_SUFFIX).$(PJ_VERSION_MAJOR) $(PJ_DIR)/third_party/lib/libgsmcodec.$(SHLIB_SUFFIX)
endif
endif
endif

ifneq (1,1)
ifeq (0,1)
APP_THIRD_PARTY_EXT += -lspeex -lspeexdsp
else
APP_THIRD_PARTY_LIB_FILES += $(PJ_DIR)/third_party/lib/libspeex-$(LIB_SUFFIX)
ifeq ($(PJ_SHARED_LIBRARIES),)
APP_THIRD_PARTY_LIBS += -lspeex-$(TARGET_NAME)
else
APP_THIRD_PARTY_LIBS += -lspeex
APP_THIRD_PARTY_LIB_FILES += $(PJ_DIR)/third_party/lib/libspeex.$(SHLIB_SUFFIX).$(PJ_VERSION_MAJOR) $(PJ_DIR)/third_party/lib/libspeex.$(SHLIB_SUFFIX)
endif
endif
endif

ifneq (,1)
APP_THIRD_PARTY_LIB_FILES += $(PJ_DIR)/third_party/lib/libilbccodec-$(LIB_SUFFIX)
ifeq ($(PJ_SHARED_LIBRARIES),)
APP_THIRD_PARTY_LIBS += -lilbccodec-$(TARGET_NAME)
else
APP_THIRD_PARTY_LIBS += -lilbccodec
APP_THIRD_PARTY_LIB_FILES += $(PJ_DIR)/third_party/lib/libilbccodec.$(SHLIB_SUFFIX).$(PJ_VERSION_MAJOR) $(PJ_DIR)/third_party/lib/libilbccodec.$(SHLIB_SUFFIX)
endif
endif

ifneq (,1)
APP_THIRD_PARTY_LIB_FILES += $(PJ_DIR)/third_party/lib/libg7221codec-$(LIB_SUFFIX)
ifeq ($(PJ_SHARED_LIBRARIES),)
APP_THIRD_PARTY_LIBS += -lg7221codec-$(TARGET_NAME)
else
APP_THIRD_PARTY_LIBS += -lg7221codec
APP_THIRD_PARTY_LIB_FILES += $(PJ_DIR)/third_party/lib/libg7221codec.$(SHLIB_SUFFIX).$(PJ_VERSION_MAJOR) $(PJ_DIR)/third_party/lib/libg7221codec.$(SHLIB_SUFFIX)
endif
endif

ifeq (0,1)
# External PA
APP_THIRD_PARTY_EXT += -lportaudio
endif

ifneq (,1)
ifeq (0,1)
APP_THIRD_PARTY_EXT += -lyuv
else
APP_THIRD_PARTY_LIB_FILES += $(PJ_DIR)/third_party/lib/libyuv-$(LIB_SUFFIX)
ifeq ($(PJ_SHARED_LIBRARIES),)
APP_THIRD_PARTY_LIBS += -lyuv-$(TARGET_NAME)
else
APP_THIRD_PARTY_LIBS += -lyuv
APP_THIRD_PARTY_LIB_FILES += $(PJ_DIR)/third_party/lib/libyuv.$(SHLIB_SUFFIX).$(PJ_VERSION_MAJOR) $(PJ_DIR)/third_party/lib/libyuv.$(SHLIB_SUFFIX)
endif
endif
endif

ifneq (1,1)
ifeq (0,1)
APP_THIRD_PARTY_EXT += -lwebrtc
else
APP_THIRD_PARTY_LIB_FILES += $(PJ_DIR)/third_party/lib/libwebrtc-$(LIB_SUFFIX)
ifeq ($(PJ_SHARED_LIBRARIES),)
APP_THIRD_PARTY_LIBS += -lwebrtc-$(TARGET_NAME)
else
APP_THIRD_PARTY_LIBS += -lwebrtc
APP_THIRD_PARTY_LIB_FILES += $(PJ_DIR)/third_party/lib/libwebrtc.$(SHLIB_SUFFIX).$(PJ_VERSION_MAJOR) $(PJ_DIR)/third_party/lib/libwebrtc.$(SHLIB_SUFFIX)
endif
endif
endif

ifneq (1,1)
ifeq (0,1)
APP_THIRD_PARTY_EXT += -lwebrtc-aec3
else
APP_THIRD_PARTY_LIB_FILES += $(PJ_DIR)/third_party/lib/libwebrtc-aec3-$(LIB_SUFFIX)
ifeq ($(PJ_SHARED_LIBRARIES),)
APP_THIRD_PARTY_LIBS += -lwebrtc-aec3-$(TARGET_NAME)
else
APP_THIRD_PARTY_LIBS += -lwebrtc-aec3
APP_THIRD_PARTY_LIB_FILES += $(PJ_DIR)/third_party/lib/libwebrtc-aec3.$(SHLIB_SUFFIX).$(PJ_VERSION_MAJOR) $(PJ_DIR)/third_party/lib/libwebrtc.$(SHLIB_SUFFIX)
endif
endif
endif


# Additional flags


#
# Video
# Note: there are duplicated macros in pjmedia/os-auto.mak.in (and that's not
#       good!

# SDL flags
SDL_CFLAGS = 
SDL_LDFLAGS = 

# FFMPEG flags
FFMPEG_CFLAGS =  
FFMPEG_LDFLAGS =  

# Video4Linux2
V4L2_CFLAGS = 
V4L2_LDFLAGS = 

# OPENH264 flags
OPENH264_CFLAGS = -DPJMEDIA_HAS_OPENH264_CODEC=1 -I/home/mylinux/sigmastar_pjsip/lib/include 
OPENH264_LDFLAGS =  -L/home/mylinux/sigmastar_pjsip/lib/lib -lopenh264 -lstdc++

# VPX flags
VPX_CFLAGS =  
VPX_LDFLAGS =  

# QT
AC_PJMEDIA_VIDEO_HAS_QT = 
# QT_CFLAGS = 

# Darwin (Mac and iOS)
AC_PJMEDIA_VIDEO_HAS_DARWIN = 
AC_PJMEDIA_VIDEO_HAS_VTOOLBOX = 
AC_PJMEDIA_VIDEO_HAS_IOS_OPENGL = 
DARWIN_CFLAGS = 

# mingw
AC_PJMEDIA_VIDEO_DEV_HAS_DSHOW = 
ifeq (,yes)
DSHOW_CFLAGS = 
DSHOW_LDFLAGS = 
APP_THIRD_PARTY_LIB_FILES += $(PJ_DIR)/third_party/lib/libbaseclasses-$(LIB_SUFFIX)
APP_THIRD_PARTY_LIBS += -lbaseclasses-$(TARGET_NAME)
endif

# Android
ANDROID_CFLAGS = 
OBOE_CFLAGS = 

# PJMEDIA features exclusion
PJ_VIDEO_CFLAGS += $(SDL_CFLAGS) $(FFMPEG_CFLAGS) $(V4L2_CFLAGS) $(DSHOW_CFLAGS) $(QT_CFLAGS) \
		   $(OPENH264_CFLAGS) $(VPX_CFLAGS) $(DARWIN_CFLAGS)
PJ_VIDEO_LDFLAGS += $(SDL_LDFLAGS) $(FFMPEG_LDFLAGS) $(V4L2_LDFLAGS) $(DSHOW_LDFLAGS) \
                   $(OPENH264_LDFLAGS) $(VPX_LDFLAGS)

# CFLAGS, LDFLAGS, and LIBS to be used by applications
export APP_CC := arm-linux-gnueabihf-gcc
export APP_CXX := arm-linux-gnueabihf-g++
export APP_CFLAGS := -DPJ_AUTOCONF=1\
	-I/home/mylinux/sigmastar_pjsip/lib/include -I/home/mylinux/sigmastar_pjsip/lib/include -I/home/mylinux/sigmastar_pjsip/include -I/home/mylinux/sigmastar_pjsip/include/sdl -I/home/mylinux/sigmastar_pjsip/include/sigmastar -I/home/mylinux/sigmastar_pjsip/include/jpeg -DPJ_IS_BIG_ENDIAN=0 -DPJ_IS_LITTLE_ENDIAN=1 -fPIC\
	$(PJ_VIDEO_CFLAGS) \
	-I$(PJDIR)/pjlib/include\
	-I$(PJDIR)/pjlib-util/include\
	-I$(PJDIR)/pjnath/include\
	-I$(PJDIR)/pjmedia/include\
	-I$(PJDIR)/pjsip/include
export APP_CXXFLAGS := -g -O2 $(APP_CFLAGS)
export APP_LDFLAGS := -L$(PJDIR)/pjlib/lib\
	-L$(PJDIR)/pjlib-util/lib\
	-L$(PJDIR)/pjnath/lib\
	-L$(PJDIR)/pjmedia/lib\
	-L$(PJDIR)/pjsip/lib\
	-L$(PJDIR)/third_party/lib\
	$(PJ_VIDEO_LDFLAGS) \
	-L/home/mylinux/sigmastar_pjsip/lib/lib -L/home/mylinux/sigmastar_pjsip/lib/lib -L/home/mylinux/sigmastar_pjsip/lib
export APP_LDXXFLAGS := $(APP_LDFLAGS)

export APP_LIB_FILES := \
	$(PJ_DIR)/pjsip/lib/libpjsua-$(LIB_SUFFIX) \
	$(PJ_DIR)/pjsip/lib/libpjsip-ua-$(LIB_SUFFIX) \
	$(PJ_DIR)/pjsip/lib/libpjsip-simple-$(LIB_SUFFIX) \
	$(PJ_DIR)/pjsip/lib/libpjsip-$(LIB_SUFFIX) \
	$(PJ_DIR)/pjmedia/lib/libpjmedia-codec-$(LIB_SUFFIX) \
	$(PJ_DIR)/pjmedia/lib/libpjmedia-videodev-$(LIB_SUFFIX) \
	$(PJ_DIR)/pjmedia/lib/libpjmedia-$(LIB_SUFFIX) \
	$(PJ_DIR)/pjmedia/lib/libpjmedia-audiodev-$(LIB_SUFFIX) \
	$(PJ_DIR)/pjnath/lib/libpjnath-$(LIB_SUFFIX) \
	$(PJ_DIR)/pjlib-util/lib/libpjlib-util-$(LIB_SUFFIX) \
	$(APP_THIRD_PARTY_LIB_FILES) \
	$(PJ_DIR)/pjlib/lib/libpj-$(LIB_SUFFIX)
export APP_LIBXX_FILES := \
	$(PJ_DIR)/pjsip/lib/libpjsua2-$(LIB_SUFFIX) \
	$(APP_LIB_FILES)

ifeq ($(PJ_SHARED_LIBRARIES),)
export PJLIB_LDLIB := -lpj-$(TARGET_NAME)
export PJLIB_UTIL_LDLIB := -lpjlib-util-$(TARGET_NAME)
export PJNATH_LDLIB := -lpjnath-$(TARGET_NAME)
export PJMEDIA_AUDIODEV_LDLIB := -lpjmedia-audiodev-$(TARGET_NAME)
export PJMEDIA_VIDEODEV_LDLIB := -lpjmedia-videodev-$(TARGET_NAME)
export PJMEDIA_LDLIB := -lpjmedia-$(TARGET_NAME)
export PJMEDIA_CODEC_LDLIB := -lpjmedia-codec-$(TARGET_NAME)
export PJSIP_LDLIB := -lpjsip-$(TARGET_NAME)
export PJSIP_SIMPLE_LDLIB := -lpjsip-simple-$(TARGET_NAME)
export PJSIP_UA_LDLIB := -lpjsip-ua-$(TARGET_NAME)
export PJSUA_LIB_LDLIB := -lpjsua-$(TARGET_NAME)
export PJSUA2_LIB_LDLIB := -lpjsua2-$(TARGET_NAME)
else
export PJLIB_LDLIB := -lpj
export PJLIB_UTIL_LDLIB := -lpjlib-util
export PJNATH_LDLIB := -lpjnath
export PJMEDIA_AUDIODEV_LDLIB := -lpjmedia-audiodev
export PJMEDIA_VIDEODEV_LDLIB := -lpjmedia-videodev
export PJMEDIA_LDLIB := -lpjmedia
export PJMEDIA_CODEC_LDLIB := -lpjmedia-codec
export PJSIP_LDLIB := -lpjsip
export PJSIP_SIMPLE_LDLIB := -lpjsip-simple
export PJSIP_UA_LDLIB := -lpjsip-ua
export PJSUA_LIB_LDLIB := -lpjsua
export PJSUA2_LIB_LDLIB := -lpjsua2

export ADD_LIB_FILES := $(PJ_DIR)/pjsip/lib/libpjsua.$(SHLIB_SUFFIX).$(PJ_VERSION_MAJOR) $(PJ_DIR)/pjsip/lib/libpjsua.$(SHLIB_SUFFIX) \
	$(PJ_DIR)/pjsip/lib/libpjsip-ua.$(SHLIB_SUFFIX).$(PJ_VERSION_MAJOR) $(PJ_DIR)/pjsip/lib/libpjsip-ua.$(SHLIB_SUFFIX) \
	$(PJ_DIR)/pjsip/lib/libpjsip-simple.$(SHLIB_SUFFIX).$(PJ_VERSION_MAJOR) $(PJ_DIR)/pjsip/lib/libpjsip-simple.$(SHLIB_SUFFIX) \
	$(PJ_DIR)/pjsip/lib/libpjsip.$(SHLIB_SUFFIX).$(PJ_VERSION_MAJOR) $(PJ_DIR)/pjsip/lib/libpjsip.$(SHLIB_SUFFIX) \
	$(PJ_DIR)/pjmedia/lib/libpjmedia-codec.$(SHLIB_SUFFIX).$(PJ_VERSION_MAJOR) $(PJ_DIR)/pjmedia/lib/libpjmedia-codec.$(SHLIB_SUFFIX) \
	$(PJ_DIR)/pjmedia/lib/libpjmedia-videodev.$(SHLIB_SUFFIX).$(PJ_VERSION_MAJOR) $(PJ_DIR)/pjmedia/lib/libpjmedia-videodev.$(SHLIB_SUFFIX) \
	$(PJ_DIR)/pjmedia/lib/libpjmedia.$(SHLIB_SUFFIX).$(PJ_VERSION_MAJOR) $(PJ_DIR)/pjmedia/lib/libpjmedia.$(SHLIB_SUFFIX) \
	$(PJ_DIR)/pjmedia/lib/libpjmedia-audiodev.$(SHLIB_SUFFIX).$(PJ_VERSION_MAJOR) $(PJ_DIR)/pjmedia/lib/libpjmedia-audiodev.$(SHLIB_SUFFIX) \
	$(PJ_DIR)/pjnath/lib/libpjnath.$(SHLIB_SUFFIX).$(PJ_VERSION_MAJOR) $(PJ_DIR)/pjnath/lib/libpjnath.$(SHLIB_SUFFIX) \
	$(PJ_DIR)/pjlib-util/lib/libpjlib-util.$(SHLIB_SUFFIX).$(PJ_VERSION_MAJOR) $(PJ_DIR)/pjlib-util/lib/libpjlib-util.$(SHLIB_SUFFIX) \
	$(PJ_DIR)/pjlib/lib/libpj.$(SHLIB_SUFFIX).$(PJ_VERSION_MAJOR) $(PJ_DIR)/pjlib/lib/libpj.$(SHLIB_SUFFIX)

APP_LIB_FILES += $(ADD_LIB_FILES)

APP_LIBXX_FILES += $(PJ_DIR)/pjsip/lib/libpjsua2.$(SHLIB_SUFFIX).$(PJ_VERSION_MAJOR) $(PJ_DIR)/pjsip/lib/libpjsua2.$(SHLIB_SUFFIX) \
	$(ADD_LIB_FILES)
endif

ifeq ($(PJ_EXCLUDE_PJSUA2),1)
export PJSUA2_LIB_LDLIB :=
endif

export APP_LDLIBS := $(PJSUA_LIB_LDLIB) \
	$(PJSIP_UA_LDLIB) \
	$(PJSIP_SIMPLE_LDLIB) \
	$(PJSIP_LDLIB) \
	$(PJMEDIA_CODEC_LDLIB) \
	$(PJMEDIA_LDLIB) \
	$(PJMEDIA_VIDEODEV_LDLIB) \
	$(PJMEDIA_AUDIODEV_LDLIB) \
	$(PJMEDIA_LDLIB) \
	$(PJNATH_LDLIB) \
	$(PJLIB_UTIL_LDLIB) \
	$(APP_THIRD_PARTY_LIBS)\
	$(APP_THIRD_PARTY_EXT)\
	$(PJLIB_LDLIB) \
	-lbcg729 -lopenh264 -lstdc++ -lm -lrt -lpthread -lSDL2 -lbcg729 -lmi_vdec -lmi_sys -lmi_venc -lmi_divp -lmi_disp -lmi_panel -lmi_common -ljpeg -lssl -lcrypto -lz -ldl -lasound
export APP_LDXXLIBS := $(PJSUA2_LIB_LDLIB) \
	-lstdc++ \
	$(APP_LDLIBS)

# Here are the variables to use if application is using the library
# from within the source distribution
export PJ_CC := $(APP_CC)
export PJ_CXX := $(APP_CXX)
export PJ_CFLAGS := $(APP_CFLAGS)
export PJ_CXXFLAGS := $(APP_CXXFLAGS)
export PJ_LDFLAGS := $(APP_LDFLAGS)
export PJ_LDXXFLAGS := $(APP_LDXXFLAGS)
export PJ_LDLIBS := $(APP_LDLIBS)
export PJ_LDXXLIBS := $(APP_LDXXLIBS)
export PJ_LIB_FILES := $(APP_LIB_FILES)
export PJ_LIBXX_FILES := $(APP_LIBXX_FILES)

# And here are the variables to use if application is using the
# library from the install location (i.e. --prefix)
export PJ_INSTALL_DIR := /home/mylinux/pj
export PJ_INSTALL_INC_DIR := ${prefix}/include
export PJ_INSTALL_LIB_DIR := ${exec_prefix}/lib
export PJ_INSTALL_CFLAGS := -I$(PJ_INSTALL_INC_DIR) -DPJ_AUTOCONF=1  -DPJ_IS_BIG_ENDIAN=0 -DPJ_IS_LITTLE_ENDIAN=1
export PJ_INSTALL_LDFLAGS_PRIVATE := $(APP_THIRD_PARTY_LIBS) $(APP_THIRD_PARTY_EXT) -lbcg729 -lopenh264 -lstdc++ -lm -lrt -lpthread -lSDL2 -lbcg729 -lmi_vdec -lmi_sys -lmi_venc -lmi_divp -lmi_disp -lmi_panel -lmi_common -ljpeg -lssl -lcrypto -lz -ldl -lasound
export PJ_INSTALL_LDFLAGS := -L$(PJ_INSTALL_LIB_DIR) $(filter-out $(PJ_INSTALL_LDFLAGS_PRIVATE),$(APP_LDXXLIBS))
