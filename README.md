## Switch-Fightstick
Proof-of-Concept Fightstick for the Nintendo Switch. Uses the LUFA library and reverse-engineering of the Pokken Tournament Pro Pad for the Wii U to enable custom fightsticks on the Switch System v3.0.0.

### Wait, what?
On June 20, 2017, Nintendo released System Update v3.0.0 for the Nintendo Switch. Along with a number of additional features that were advertised or noted in the changelog, additional hidden features were added. One of those features allows for the use of compatible controllers, such as the Pokken Tournament Pro Pad, to be used on the Nintendo Switch.

Unlike the Wii U, which handles these controllers on a 'per-game' basis, the Switch treats the Pokken controller as if it was a Switch Pro Controller. Along with having the icon for the Pro Controller, it functions just like it in terms of using it in other games, apart from the lack of physical controls such as analog sticks, the buttons for the stick clicks, or other system buttons such as Home or Capture.

### Printing Splatoon Posts
For my own personal use, I repurposed Switch-Fightstick to output a set sequence of inputs to systematically print Splatoon posts. This works by using the smallest size pen and D-pad inputs to plot out each pixel one-by-one.

#### Printing Procedure
Just plug in the controller: it will automatically sync with the console, reset the cursor position and print. In case you see issues with controller conflicts while in docked mode, try to use an USB-C to USB-A adapter in handheld mode. Printing currently takes about an hour.

Each line is printed from left to right, and up to down. This repository has been tested using a Teensy 2.0++ and an Arduino UNO R3.

#### Compiling and Flashing onto the Teensy 2.0++
Go to the Teensy website and download/install the [Teensy Loader application](https://www.pjrc.com/teensy/loader.html). Then, follow their instructions on installing the [GCC Compiler and Tools](https://www.pjrc.com/teensy/gcc.html). (Note for Mac users - the AVR MacPack is now called AVR CrossPack. If that does not work, you can try installing avr-gcc with brew.) Next, you need to grab the LUFA library. You can download it in a zipped folder at the bottom of [this page](http://www.fourwalledcubicle.com/LUFA.php). Unzip the folder, rename it `LUFA`, and place it where you like. Then, download or clone the contents of this repository onto your computer. Next, you'll need to make sure the `LUFA_PATH` inside of the `makefile` points to the `LUFA` subdirectory inside your `LUFA` directory. My `Switch-Fightstick` directory is in the same directory as my `LUFA` directory, so I set `LUFA_PATH = ../LUFA/LUFA`.
Now you should be ready to rock. Open a terminal window in the `Switch-Fightstick` directory, type `make`, and hit enter to compile. If all goes well, the printout in the terminal will let you know it finished the build! Follow the directions on flashing `Joystick.hex` onto your Teensy, which can be found page where you downloaded the Teensy Loader application.

#### Compiling and Flashing onto an Arduino UNO R3
You will need to set your [Arduino in DFU mode](https://www.arduino.cc/en/Hacking/DFUProgramming8U2), and flash its USB controller. (Note for Mac users - try [brew](https://brew.sh/index_it.html) to install the dfu-programmer with `brew install dfu-programmer`.) Setting an Arduino UNO R3 in DFU mode is quite easy, all you need is a jumper (the boards come with the needed pins in place). Please note that once the board is flashed, you will need to flash it back with the original firmware to make it work again as a standard Arduino. To compile this project you will need the AVR GCC Compiler and Tools. (Again for Mac users - try brew, adding the [osx-cross/avr](osx-cross/avr) repository, all you need to do is to type `brew tap osx-cross/avr` and `brew install avr-gcc`.) Next, you need to grab the LUFA library: download and install it following the steps described for the Teensy 2.0++.
Finally, open a terminal window in the `Switch-Fightstick` directory, edit the `makefile` setting `MCU = atmega16u2`, and compile by typing `make`. Follow the [DFU mode directions](https://www.arduino.cc/en/Hacking/DFUProgramming8U2) to flash `Joystick.hex` onto your Arduino UNO R3 and you are done.

#### Using your own image
The image printed depends on `image.c` which is generated with `png2c.py` which takes a 320x120 .png image. `png2c.py` will pack the image to a linear 1bpp array. If the image is not already made up of only black and white pixels, it will be dithered.

In order to run `png2c.py`, you need to [install Python](https://www.python.org/downloads/) (I use Python 2.7). Also, you need to have the [Python Imaging Library](https://pillow.readthedocs.io/en/3.0.0/installation.html) installed ([install pip](https://pip.pypa.io/en/stable/installing/#do-i-need-to-install-pip) if you need to).
Using the supplied sample image, splatoonpattern.png:

```
$ python png2c.py splatoonpattern.png
```
Substitute your own .png image to generate the `image.c` file necessary to print. Just make sure your image is in the `Switch-Fightstick` directory.

#### What the dither?
As previously mentioned, png2c.py will dither the input image if you supply an image that is not already made up of only black and white pixels. Say you want to print this bomb image you created...

![http://imgur.com/r2GoVdD.png](http://imgur.com/r2GoVdD.png)

*image via [vjapolitzer](https://github.com/vjapolitzer)*

...but you want to know what it will look like before committing to printing it in Splatoon. Fret not! You can also preview or save a copy of the bilevel version of your image.

To preview the bilevel image:

```
$ python png2c.py -p yourImage.png
```

To save the bilevel image:

```
$ python png2c.py -s yourImage.png
```

![http://imgur.com/uUOeJ7P.png](http://imgur.com/uUOeJ7P.png)

*image via [vjapolitzer](https://github.com/vjapolitzer)*

Looks good! Time to get printing.

### Sample
![http://i.imgur.com/93B1Usb.jpg](http://i.imgur.com/93B1Usb.jpg)
*image via [/u/Stofers](https://www.reddit.com/user/Stofers)*
