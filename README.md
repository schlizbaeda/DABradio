# Raspberry Pi DAB Radio Daemon
This DAB radio project is based on a Raspberry Pi and the board
[DAB DAB+ FM Digital Radio Development Board Pro with SlideShow](https://www.monkeyboard.org/products/85-developmentboard/85-dab-dab-fm-digital-radio-development-board-pro)
from [MonkeyBoard](https://www.monkeyboard.org/).

## Hardware Conception
This project is still a proof of concept! At the moment the hardware
connections are done on a breadboard. You can see the wiring setup in
the file [`wiring_schematics.pdf`](https://github.com/schlizbaeda/DABradio/blob/master/wiring_schematics.pdf):
* A Raspberry Pi is used as base unit
* A HifiBerry DAC+ is used as audio output because it offers I²S input pins
* The GPIO connections from Raspberry Pi to HifiBerry DAC+ are done by breadboard wires
* The MonkeyBoard (DAB receiver board) is connected to the Raspberry Pi with an USB cable to control it via a serial port (/dev/ttyACM0)
* The I²S bus lines from the MonkeyBoard are connected to the HifiBerry DAC+ directly. The I²S lines of the Raspberry Pi are disconnected.
#### Attention!
This project doesn't work on the native audio outputs (HDMI and
3,5mm socket) of the Raspberry Pi neither on USB soudcards!

The I²S audio output of the MonkeyBoard needs an I²S-DAC like the
[PCM5122](http://www.ti.com/product/PCM5122) from Texas Instruments
to convert the digital audio data into analogue signals. To make this
demo DAB radio work you will need an I²S DAC board like for example
the [HifiBerry DAC+](https://www.hifiberry.com/products/dacplus/)
where the I²S connectors are disconnected from the Raspberry Pi and
connected to the MonkeyBoard instead as shown in file
`wiring_schematics.pdf`.

## Software Installation on the Raspberry Pi
#### Preparation of Raspbian
Download the latest [Raspbian Stretch with desktop and recommended software ("full image")](https://www.raspberrypi.org/downloads/raspbian)
and flash it on a SD card with capacity of 8GB or higher as described
by the Raspberry Pi Foundation [here](https://www.raspberrypi.org/documentation/installation/installing-images/README.md).

Modify the file `/boot/config.txt` for activating the HifiBerry DAC+
or a compatible board:
```shell
sudo nano /boot/config.txt
```
and change the audio driver overlay like this:
```shell
# Disable audio (loads snd_bcm2835)
#dtparam=audio=on
# Enable the I2S DAC:
dtoverlay=hifiberry-dacplus
```
Reboot the Raspberry Pi and start a fake audio playback of a freely 
chosen audio file via I²S:
```shell
omxplayer -o alsa /home/pi/Music/any_audiofile.flac # alternatively any *.mp3 file
```
Of course, you won't hear anything from this audio file because the
I²S wires were disconnected from the Raspberry Pi. But the I²C
connection of the DAC (PCM5122 resp. HifiBerry DAC+) continues to be
necessary for setting the I²S DAC into an active playback mode which
will be used by the MonkeyBoard later.

#### Installation of the DAB Radio Daemon (`dabd`)
Clone this repository onto the Raspberry Pi and start the installation
shell script [`setup.sh`](https://github.com/schlizbaeda/DABradio/blob/master/setup.sh)
in an LXTerminal:
```shell
cd /home/pi
git clone https://github.com/schlizbaeda/DABradio
cd DABradio
./setup.sh
```
Now the KeyStone library delivered by MonkeyBoard is installed properly
on the Raspbian OS for further use.

## Getting Started
`dabd` is a DAB radio daemon which accepts commands from `stdin` and
sends its result strings to `stdout`. So you can control `dabd` using
the keyboard.

Call the software `dabd` in the LXTerminal:
```shell
cd /home/pi/DABradio
./dabd
```
Start audio playback by entering the following commands into the
LXTerminal. They will be read by `dabd`:
```shell
open
set volume 9
set stereo 1
scan
list
playstream 14
close
quit
```
As you can hear, the DAB playback continues after the connection to the
MonkeyBoard was closed and the application itself has quit.
At my home the parameter `14` of the command `playstream ...`
represents [Radio BOB!](https://www.radiobob.de/), my favourite DAB+
station. Enjoy listening to DAB Radio.

## Usage of an advanced frontend
Using the named pipe (FIFO) mechanism of Linux offers a lot of
possibilities to redirect the DAB radio control to more convenient
user frontends as there are
* Graphical User Interface
* GPIO control (e.g. hardware push buttons or even a remote control via LIRC)
* Web server control via browser

#### dabgui.py
This Python3 script offers a simple tkinter GUI to control `dabd`. Call
this script standalone in the LXTerminal:
```shell
./dabgui.py
```
Test this GUI application by entering some keyboard text. Each line
entered in `stdin` of the LXTerminal will be printed into a textbox of
the GUI. Any command sent by clicking onto the buttons of `dabgui.py`
will be sent to `stdout`. So it will appear in the LXTerminal window.

#### Linux Named Pipes (FIFO)
On Linux anonymous pipes are created by shell commands like
```
ls -l | grep less
```
The disadvantage is they offer only a data transfer in a single
direction. But to use an interactive frontend for `dabd` a
bidirectional mechanism is necessary.

Linux Named Pipes are special files which can be read or written
by shell redirection. Detailed explanations can be found at
[www.linuxjournal.com/article/2156](https://www.linuxjournal.com/article/2156)
or at [unix.stackexchange.com](https://unix.stackexchange.com/questions/53641/how-to-make-bidirectional-pipe-between-two-programs).

You need two named pipes, one which sends commands from the GUI to
`dabd` and another one which receives the data from `dabd`. Their names
are chosen as `fromDABD`and `toDABD` and they were created by the
installation script `setup.sh` with command
```shell
mkfifo fromDABD toDABD
```

The shell script `dabradio.sh` calls both programs using the Named
Pipes in this way:
```shell
./dabd >fromDABD <toDABD &
./dabgui.py <fromDABD >toDABD
```

## Description of the C++ Code for `dabd`
First this application starts another thread to read its commands from
`stdin` in parallel. Then it creates the class `KeyStone` which contains
several methods to control the DAB radio board.

To convert the UTF-16 strings returned by the original KeyStoneCOMM.h
into UTF-8 strings the GNU library [libiconv](https://www.gnu.org/software/libiconv/)
is used.

Typing the command `help` on `stdin` inside `dabd` prints a short help
text onto `stdout`.

## Hint
The MOT slideshow feature isn't implemented yet because there are some
issues. It doesn't work properly!

