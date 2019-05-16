#!/bin/bash
echo "######## install KeyStone library:"
#wget http://www.monkeyboard.org/images/products/dab_fm/raspberrypi_keystone.tgz
tar -xvzpf raspberrypi_keystone.tgz keystonecomm/
cd keystonecomm/KeyStoneCOMM/
sudo make install
cd ../app
#make
echo "######## install dabd sourcecode and executable:"
cd ../..
mv dabd keystonecomm
cd keystonecomm/dabd
make
cd ../..
echo "######## create symbolic link and named pipes:"
ln -s keystonecomm/dabd/dabd
mkfifo fromDABD toDABD
