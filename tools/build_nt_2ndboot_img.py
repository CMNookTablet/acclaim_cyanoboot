#!/usr/bin/python

import getopt, struct, sys


KERNEL_OFFSET = 0x120
PAGE_SIZE = 4096


def write_alignment_padding(fp):
    while (fp.tell() % PAGE_SIZE) != 0:
        fp.write('\xff')

def write_android_header(fp, kern_size, kern_addr, rd_size, rd_addr):
    # struct boot_img_hdr
    fp.write(struct.pack("<8sIIIIIIIIII16s512sIIIIIIII", "ANDROID!",
        kern_size, kern_addr, rd_size, rd_addr, 0, 0, 0, PAGE_SIZE, 0, 0,
        "BauwksBoot", "",
        0, 0, 0, 0, 0, 0, 0, 0))
    write_alignment_padding(fp)

def usage():
    print("Usage: %s [options] <u-boot.bin>" % sys.argv[0])

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'fo:')
    except getopt.GetoptError:
        usage()
        sys.exit(1)

    internal_flash_boot = False
    output_filename = "bauwks_boot.img"
    uboot2_loadaddr = 0x80f00000
    for o, a in opts:
        if o == "-f":
            internal_flash_boot = True
        elif o == "-o":
            output_filename = a
    if len(args) < 1:
        usage()
        sys.exit(1)

    uboot2_fp = open(args[0], "rb")
    uboot2_img = uboot2_fp.read()
    uboot2_fp.close()

    stacksmash_img = struct.pack("<I", uboot2_loadaddr) * 1024
    top_of_stack = (0x80e80000 - (2*128*1024) - 128 - 12) & ~0x7
    stacksmash_loadaddr = top_of_stack - len(stacksmash_img)

    output_fp = open(output_filename, "wb")
    if internal_flash_boot:
        # For building images that are loaded using "booti mmcN"
        # That code path loads the kernel at a different offset than the
        # "booti <address>" code path. Nice.
        write_android_header(output_fp,
            len(uboot2_img), uboot2_loadaddr,
            len(stacksmash_img), stacksmash_loadaddr + KERNEL_OFFSET)
    else:
        # For building flashing_boot.img that is loaded using fatload
        # and run using "booti <address>"
        write_android_header(output_fp,
            len(uboot2_img), uboot2_loadaddr + KERNEL_OFFSET,
            len(stacksmash_img), stacksmash_loadaddr + KERNEL_OFFSET)
    output_fp.write(uboot2_img)
    write_alignment_padding(output_fp)
    output_fp.write(stacksmash_img) # the "ramdisk"
    output_fp.close()

if __name__ == '__main__':
    main()


