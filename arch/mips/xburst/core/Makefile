obj-y += prom.o

ifeq ($(CONFIG_SOC_T30), y)
obj-y += mxu-v2-ex-t30.obj
else
obj-y += mxu-v2-ex.obj
endif

obj-$(CONFIG_JZ_CPU_SCACHE)	+= sc-jz.o
obj-$(CONFIG_TRAPS_USE_TCSM) += tcsm.o
obj-$(CONFIG_SMP)	+= smp.o smp_secondary.o ltimer.o
obj-$(CONFIG_PMON_DEBUG)	+= perf_proc_jz.o perf_event_jz.o
