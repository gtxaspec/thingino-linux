dtb-$(CONFIG_DT_GOAT)	+= goat.dtb
dtb-$(CONFIG_DT_VELA)	+= vela.dtb
dtb-$(CONFIG_DT_GOAT_PRJ008)	+= goat_prj008.dtb
dtb-$(CONFIG_DT_VELA_PRJ008)	+= vela_prj008.dtb
dtb-$(CONFIG_DT_ZERATUL)	+= zeratul.dtb
dtb-$(CONFIG_DT_ZERATUL_PRJ008)	+= zeratul_prj008.dtb
dtb-$(CONFIG_DT_ATLAS_PRJ008)	+= atlas_prj008.dtb

obj-y				+= $(patsubst %.dtb, %.dtb.o, $(dtb-y))

# Force kbuild to make empty built-in.o if necessary
obj-				+= dummy.o

always				:= $(dtb-y)
clean-files			:= *.dtb *.dtb.S
