fe310_dir = $(abspath $(src_dir)/../../fw/fe310)
include $(fe310_dir)/platform.mk
CFLAGS += -I$(fe310_dir) -D__FE310__
