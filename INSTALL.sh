#!/bin/bash

echo -----Script Install Domoserv_pi-----

echo Install git
sudo apt-get install git

echo Clone repo Domoserv_pi
cd $HOME
git clone https://github.com/firedream89/domoserv_pi.git

if ! test -d /usr/local/qt5pi; then
echo copie domoserv_pi/qt5pi to /usr/local
sudo mkdir /usr/local/qt5pi
sudo cp -r qt5pi /usr/local/qt5pi
fi

if ! test -f /ect/init.d/Domoserv_pi; then
echo copie deamon Domoserv_pi to /etc/init.d
sudo cp domoserv_pi/Domoserv_pi /etc/init.d/
fi