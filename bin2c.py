#!/bin/python

import sys, getopt

def main(argv):
  opts, args = getopt.getopt(argv, "hi")
  data = open(args[0], 'rb').read()

  invertColormap = False
  for opt, arg in opts:
    if opt == '-h':
      usage()
      sys.exit()
    elif opt == '-i':
      invertColormap = True

  str_out = "#include <stdint.h>\n#include <avr/pgmspace.h>\n\nconst uint8_t image_data[0x12c1] PROGMEM = {"
  for i in range(0, (320*120) / 8):
    val = 0;

    for j in range(0, 8):
        val |= ord(data[(i * 8) + j]) << j

    if (invertColormap):
      val = ~val & 0xFF;
    else:
      val = val & 0xFF;

    str_out += hex(val) + ", "

  str_out += "0x0};"

  with open('image.c', 'w') as f:
    f.write(str_out)

  if (invertColormap):
      print("{} converted with inverted colormap and saved to image.c".format(args[0]))
  else:
      print("{} converted with original colormap and saved to image.c".format(args[0]))

def usage():
  print("To convert to image.c: bin2c.py yourImage.data")
  print("To convert to an inverted image.c: bin2c.py -i yourImage.data")

if __name__ == "__main__":
  main(sys.argv[1:])
