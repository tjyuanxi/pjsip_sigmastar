# build/os-auto.mak.  Generated from os-auto.mak.in by configure.

export OS_CFLAGS   := $(CC_DEF)PJ_AUTOCONF=1 -I/home/mylinux/sigmastar_pjsip/lib/include -I/home/mylinux/sigmastar_pjsip/lib/include -I/home/mylinux/sigmastar_pjsip/include -I/home/mylinux/sigmastar_pjsip/include/sdl -I/home/mylinux/sigmastar_pjsip/include/sigmastar -I/home/mylinux/sigmastar_pjsip/include/jpeg -DPJ_IS_BIG_ENDIAN=0 -DPJ_IS_LITTLE_ENDIAN=1 -fPIC

export OS_CXXFLAGS := $(CC_DEF)PJ_AUTOCONF=1 -g -O2

export OS_LDFLAGS  := -L/home/mylinux/sigmastar_pjsip/lib/lib -L/home/mylinux/sigmastar_pjsip/lib/lib -L/home/mylinux/sigmastar_pjsip/lib -lbcg729 -lopenh264 -lstdc++ -lm -lrt -lpthread -lSDL2 -lbcg729 -lmi_vdec -lmi_sys -lmi_venc -lmi_divp -lmi_disp -lmi_panel -lmi_common -ljpeg -lssl -lcrypto -lz -ldl -lasound

export OS_SOURCES  := 


