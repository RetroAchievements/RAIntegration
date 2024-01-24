#!/usr/bin/python3
import PIL.Image as Image

def convert(source, output, name):
    bytes = []
    with Image.open(source, mode='r') as img:
        bytes = img.tobytes()

    with open(output, mode='w') as f:
        f.write("#ifndef {0}_H\n".format(name));
        f.write("#define {0}_H\n\n".format(name));
        f.write("const uint8_t {0}_PIXELS[] = {{\n".format(name));

        i = 0
        num_bytes = len(bytes)
        while i < num_bytes:
            f.write("    ")
            for j in range(0,20):
                if i < num_bytes:
                    # just write the alpha value. if it's non-zero the RGB will be constant
                    f.write("0x{0:02x},".format(bytes[i+3]))
                i += 4
            f.write("\n")
    
        f.write("};\n\n");
        f.write("#endif {0}_H\n".format(name));

convert('missable.png', 'missable.h', 'MISSABLE')
convert('progression.png', 'progression.h', 'PROGRESSION')
convert('win-condition.png', 'win-condition.h', 'WIN_CONDITION')

