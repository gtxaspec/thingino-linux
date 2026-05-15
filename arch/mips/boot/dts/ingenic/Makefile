dtb-$(CONFIG_DT_PRJ007_GOAT)	+= goat.dtb
dtb-$(CONFIG_DT_PRJ007_VELA)	+= vela.dtb
dtb-$(CONFIG_DT_PRJ008_GOAT)	+= goat_prj008.dtb
dtb-$(CONFIG_DT_PRJ008_VELA)	+= vela_prj008.dtb
dtb-$(CONFIG_DT_PRJ007_ZERATUL)	+= zeratul.dtb
dtb-$(CONFIG_DT_PRJ008_ZERATUL)	+= zeratul_prj008.dtb
dtb-$(CONFIG_DT_PRJ007_ATLAS)	+= atlas.dtb
dtb-$(CONFIG_DT_PRJ008_ATLAS)	+= atlas_prj008.dtb

obj-y				+= $(patsubst %.dtb, %.dtb.o, $(dtb-y))

# Force kbuild to make empty built-in.o if necessary
obj-				+= dummy.o

always				:= $(dtb-y)
clean-files			:= *.dtb *.dtb.S
