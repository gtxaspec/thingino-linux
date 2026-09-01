obj-$(CONFIG_COMMON_CLK_NA51068)	+= nvt-im-clk-bk.o na51068-clk.o

ifdef CONFIG_FUNCTION_TRACER
ORIG_CFLAGS := $(KBUILD_CFLAGS)
KBUILD_CFLAGS = $(subst $(CC_FLAGS_FTRACE),,$(ORIG_CFLAGS))
endif
obj-$(CONFIG_COMMON_CLK_NA51089)	+= nvt-im-clk-fr.o na51089-clk.o nvt-clk-dbg.o na51089/pll.o_shipped

obj-$(CONFIG_COMMON_CLK_NA51055)	+= nvt-im-clk-fr.o na51055-clk.o nvt-clk-dbg.o na51055/pll.o_shipped
