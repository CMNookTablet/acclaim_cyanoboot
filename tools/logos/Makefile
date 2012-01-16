
all: o_nookcolor_logo_large.rle o_nookcolor_logo_loading.rle o_nookcolor_logo_recovery.rle

o_nookcolor_logo_large.rle:
	convert $(patsubst %.rle,%.bmp,$@) rgb:$(patsubst %.rle,%.raw,$@)
	~/android/ics/out/host/linux-x86/bin/rgb2565 -rle < $(patsubst %.rle,%.raw,$@) > $@

o_nookcolor_logo_loading.rle:
	convert $(patsubst %.rle,%.bmp,$@) rgb:$(patsubst %.rle,%.raw,$@)
	~/android/ics/out/host/linux-x86/bin/rgb2565 -rle < $(patsubst %.rle,%.raw,$@) > $@

o_nookcolor_logo_recovery.rle:
	convert $(patsubst %.rle,%.bmp,$@) rgb:$(patsubst %.rle,%.raw,$@)
	~/android/ics/out/host/linux-x86/bin/rgb2565 -rle < $(patsubst %.rle,%.raw,$@) > $@

install:
	mv *.rle ../../board/omap4430sdp/

clean:
	$(RM) *.rle *.raw


