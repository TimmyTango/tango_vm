import argparse
import sys

from PIL import Image

def main():
    parser = argparse.ArgumentParser(description='Converts images to TangoVM Game Console ROM')
    parser.add_argument('source_filename', metavar='source_filename', type=str, help='image file to convert')
    parser.add_argument('-o', '--out', dest='out_file', metavar='output_file', default='img.rom')
    args = parser.parse_args()

    with Image.open(args.source_filename) as img:
        if img.size != (64, 64):
            print('Image file must be 64x64 pixels')
            sys.exit(1)
        elif img.mode != 'P':
            print('Tool currently only works for indexed images')
            sys.exit(1)

        data = list(img.getdata())
        rom_output = ''

        for y in range(64):
            addr = 0xF000 + y * 32
            rom_output += f'{addr:04X}: '
            for x in range(0, 64, 2):
                i = y * 64 + x
                value = (data[i] << 4) + data[i+1]
                rom_output += f'{value:02X} '
            rom_output = rom_output[:-1] + '\n'

        with open(args.out_file, 'w') as outfile:
            outfile.write(rom_output)

if __name__ == '__main__': main()