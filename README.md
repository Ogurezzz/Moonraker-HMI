Overview:
---
The first aim of this project is to reuse old Anycubic LCD's (dgus based) after you moved your Anycubic i3 Mega to klipper from Marlin.

Install
--
```
cd ~
git clone https://github.com/Ogurezzz/Moonraker-HMI.git
cd Moonraker-HMI
~/Moonraker-HMI/scripts/install.sh
```
Follow instruction on the scree to install single or multiple instances.
Script will install dependencies, compile binary and register as many daemons as you request. Usually you will nee only one service for one moonraker instance. Obviously you need separate LCD for each moonraker-hmi service instance.


Update
---
```
cd ~/Moonraker-HMI
git pull
make
sudo service moonraker-hmi restart
```
Restart service after update.

See [Wiki](https://github.com/Ogurezzz/Moonraker-HMI/wiki) for more details

![overview](Pictures/overview.jpg)
