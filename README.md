The first aim of this project is to reuse old Anycubic LCD's (dgus based) after you moved to klipper from Marlin.

Install dependencies
```
sudo apt install libcurl4-openssl-dev
```

Clone and compile:
```
cd ~
git clone --recurse-submodules https://github.com/Ogurezzz/Moonraker-HMI.git
cd Moonraker-HMI
make
```
Make binary executable:
```
sudo chmod +x ./bin/Moonraker-HMI
```
To run it, just connect your old LCD to local machine with USB-UART converter and run (check your converter tty and printer address):
```
./bin/Moonraker-HMI http://[your klipper address]:7125 [your tty device]
# example:
./bin/Moonraker-HMI http://192.168.5.10:7125 /dev/ttyUSB0
```
Also you can add this binary to PATH and set autostart.
