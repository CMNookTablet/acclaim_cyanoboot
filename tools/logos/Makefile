
all: o_nookcolor_logo_large.rle connect_charge.rle lowbatt_charge.rle

o_nookcolor_logo_large.rle:
	convert $(patsubst %.rle,%.bmp,$@) rgb:$(patsubst %.rle,%.raw,$@)
	rgb2565 -rle < $(patsubst %.rle,%.raw,$@) > $@

connect_charge.rle:
	convert $(patsubst %.rle,%.bmp,$@) rgb:$(patsubst %.rle,%.raw,$@)
	rgb2565 -rle < $(patsubst %.rle,%.raw,$@) > $@

lowbatt_charge.rle:
	convert $(patsubst %.rle,%.bmp,$@) rgb:$(patsubst %.rle,%.raw,$@)
	rgb2565 -rle < $(patsubst %.rle,%.raw,$@) > $@

install:
	mv *.rle ../../board/omap4430sdp/

clean:
	$(RM) *.rle *.raw


