dtb-$(CONFIG_DT_PRJ007)	+= goat.dtb
dtb-$(CONFIG_DT_ZERATUL)	+= zeratul.dtb

obj-y				+= $(patsubst %.dtb, %.dtb.o, $(dtb-y))

# Force kbuild to make empty built-in.o if necessary
obj-				+= dummy.o

always				:= $(dtb-y)
clean-files			:= *.dtb *.dtb.S
