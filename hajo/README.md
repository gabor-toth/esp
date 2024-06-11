# Signal K

- https://signalk.org/installation.html
    - https://demo.signalk.org/@signalk/instrumentpanel/
- https://github.com/SignalK/signalk-server#how-to-get-signal-k-server
- https://github.com/SignalK/signalk-server/blob/master/docker/README.md#quickstart

## In Docker

```
docker run -d --init  --name signalk-server -p 3000:3000 -v $(pwd):/home/node/.signalk cr.signalk.io/signalk/signalk-server
```

## Local

```
sudo apt isntall libavahi-compat-libdnssd-dev
sudo npm install -g mdns
sudo npm install -g signalk-server

```

## Requests

```
curl -X GET http://10.128.65.180:3000/signalk
{"endpoints":{"v1":{"version":"2.4.1","
signalk-http":"http://10.128.65.180:3000/signalk/v1/api/","signalk-ws":"ws://10.128.65.180:3000/signalk/v1/stream","signalk-tcp":"tcp://10.128.65.180:8375"}},"server":{"id":"signalk-server-node","version":"2.4.1"}}
```

```
wscat -c "ws://10.128.65.180:3000/signalk/v1/stream?subscribe=all"
Connected (press CTRL+C to quit)
< {"name":"signalk-server","version":"2.4.1","self":"vessels.urn:mrn:signalk:uuid:
59e1f1c9-9e32-4340-a1d0-656512c48f0a","roles":["master","main"],"timestamp":"2023-11-22T12:55:47.852Z"}
< {"context":"vessels.urn:mrn:signalk:uuid:59e1f1c9-9e32-4340-a1d0-656512c48f0a","updates":[{"$source":"defaults","
timestamp":"2023-11-22T12:43:36.969Z","
values":[{"path":"","value":{"uuid":"urn:mrn:signalk:uuid:59e1f1c9-9e32-4340-a1d0-656512c48f0a"}}]}]}
```

## Other links

- [Discovery and Connection Establishment](https://signalk.org/specification/1.7.0/doc/connection.html)
- [Streaming API](https://signalk.org/specification/1.7.0/doc/streaming_api.html)

## Service Sniffer

```
sudo apt install gssdp-tools

# MDNS
avahi-browse -r -a -t
avahi-browse -r _signalk-ws._tcp
avahi-browse -r _signalk-http._tcp

# SSDP
gssdp-device-sniffer -i docker0
gssdp-device-sniffer -i enp7s0
gssdp-device-sniffer -i wlp0s20f3
```

## URLs

http://192.168.72.182/description.xml
http://192.168.72.182/index.html
http://192.168.72.182/signalk
ws://182.72.168.192:81/

## MDNS

```
+ docker0 IPv4 6d65d96b9f13                                  _signalk-ws._tcp     local
= docker0 IPv4 6d65d96b9f13                                  _signalk-ws._tcp     local
  hostname = [6d65d96b9f13.local]
  address = [172.17.0.2]
  port = [3000]
  txt = ["vuuid=urn:mrn:signalk:uuid:7f446102-b734-40b4-a384-0ed9ee12579c" "self=urn:mrn:signalk:uuid:7f446102-b734-40b4-a384-0ed9ee12579c" "roles=master, main" "swvers=2.4.1" "swname=signalk-server" "txtvers=1"]
```

# Raspberry PI

## Install

### System

- download Raspberry PI Imager from here: https://www.raspberrypi.com/software/
- choose 'Raspberry PI OS (other)', then choose 'OS Lite (64 bit)'
- (which will download an image from
  here https://www.raspberrypi.com/software/operating-systems/#raspberry-pi-os-64-bit)
- select target SD card and write
- put SD card into PI, boot with HDMI and keyboard connected
- select Hungarian layout and normal keyboard
- set hostname to 'signalk'
- create user 'signalk' with password 'signalk'
- login with that credentials
- set up ssh server

```
sudo raspi-config
```

- then go to menu 3 'Interfaces' and I1 'SSH'
- and enable it
- connect from your pc

```
ssh signalk@192.168.72.191
```

- and continue in ssh

```
mkdir -p .ssh
chmod 700 .ssh/
echo 'ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIKcyI/bADRtxOoJ1hDOtbntHil+7zQbVbTIEuEyCPMNb gabor.toth@p92.hu' > .ssh/authorized_keys

sudo su
apt -y update
apt -y upgrade
apt -y install traceroute vim
sed -i 's/raspberrypi/solpi/' /etc/hosts
hostnamectl set-hostname solpi
sed -i '/history-search/ s/# //' /etc/inputrc
dpkg-reconfigure tzdata
# unattended-upgrades
# dpkg-reconfigure --priority=low unattended-upgrades
# raspi-config
 # ?
exit
exit
```

### Power

- LED & HDMI: https://www.jeffgeerling.com/blogs/jeff-geerling/controlling-pwr-act-leds-raspberry-pi
- https://linuxhint.com/tips-tricks-optimize-power-consumption-raspberry-pi/
- https://raspberrypi.stackexchange.com/questions/114422/what-is-the-minimum-power-required-for-an-rpi-4-in-halt-or-shutdown/114423#114423

TODO: add commands

#### Tweak hardware settings

- https://linuxhint.com/tips-tricks-optimize-power-consumption-raspberry-pi/

```
sudo su
vi /boot/firmware/config.txt

# power save stuff
arm_boost=0
dtoverlay=disable-bt
# Turn off Power LED
dtparam=pwr_led_trigger=default-on
dtparam=pwr_led_activelow=off
# Turn off Activity LED
dtparam=act_led_trigger=none
dtparam=act_led_activelow=off
# Turn off Ethernet ACT LED
dtparam=eth_led0=4
# Turn off Ethernet LNK LED
dtparam=eth_led1=4

# clock is not stable below 600
arm_freq_min=600
core_freq_min=100
sdram_freq_min=50
over_voltage_min=0

```

Bluetooth

```
systemctl stop bluetooth
systemctl disable bluetooth
```

Startup speed

```
systemd-analyze blame
```

### SignalK

Source link?

```
sudo su
curl -fsSL https://deb.nodesource.com/setup_lts.x | bash -
apt -y install nodejs libnss-mdns avahi-utils libavahi-compat-libdnssd-dev
apt-mark auto libnss-mdns
npm install -g npm@latest
npm install -g --unsafe-perm signalk-server -y # will take a while
signalk-server-setup
# Enter the location to store server configuration: /home/signalk/.signalk
# Enter your vessel name: sol
# Enter your mmsi if you have one: 
# The Signal K default port is 3000
# Port 80 does not require ":3000" in the browser and app interfaces
# Do you want to use port 80? Yes
# Do you want to enable SSL? No

vi /etc/systemd/system/signalk.service
  # add at the end
  [Unit]
  Wants=network.target
systemctl daemon-reload
```

- go to http://192.168.72.191/
- create an admin account admin/signalk
- login
- TBC

#### Dashboard

- Install from Appstore
    - KIP (https://github.com/mxtommy/Kip)
    - signalk-alarm-silencer
    - signalk-derived-data (https://github.com/SignalK/signalk-derived-data/blob/master/README.md)
    - ? @signalk/signalk-autopilot
    - ? signalk-racing-calculator
    - ? rest-provider-signalk (https://www.npmjs.com/package/rest-provider-signalk)
- Setup
    - Server,Settings, Options, mdns enable
    - Save, restart
- Create user kip, set pwd kip, admin or at least r/w
- Link: http://192.168.72.180/@mxtommy/kip
- Copy demo page to local user
    - load demo
    - menu Configuration, Settings,
    - connect to local, don't login,
    - request token, approve in SignalK (timeout = NEVER)
    - go to Storage tab, save layout to global/default
    - log in with kip/kip
    - go to Storage tab, copy global/default to user/default
- todo
    - self.electrical.batteries.1.voltage

### Wifi AP

On Debian 12 (Bookworm), see

- https://raspberrytips.com/access-point-setup-raspberry-pi/

```
rasp-config
  Localization, Wifi coubtra, HU, Finish
nmcli con add con-name hotspot ifname wlan0 type wifi ssid "sol"
nmcli con modify hotspot ipv4.method shared ipv4.address 192.168.77.1/24
nmcli con modify hotspot wifi-sec.key-mgmt wpa-psk
nmcli con modify hotspot wifi-sec.psk "SoL37695"
nmcli con modify hotspot 802-11-wireless.mode ap 802-11-wireless.band bg 802-11-wireless.channel 2 ipv4.method shared
nmcli con modify hotspot 802-11-wireless-security.proto rsn

nmcli connection down hotspot
nmcli connection up hotspot

nmtui
```

### Wifi station

On Debian 12 (Bookworm):

```
nmcli dev show wlan1
nmcli dev wifi list
nmcli connection show

#nmcli dev set wlan1 autoconnect yes
nmcli dev wifi connect TothKiss password ******** ifname wlan1
nmcli dev wifi connect TGA password ******** ifname wlan1
#nmcli connection modify TothKiss connection.autoconnect yes

systemctl restart NetworkManager

```

Turn off Wifi dongle's LED:

- https://github.com/lwfinger/rtl8188eu/issues/82

```
echo 0 > /sys/class/leds/rtl8xxxu-usb1-1.4/brightness
```

See

- https://serverfault.com/questions/869857/systemd-how-to-selectively-disable-wpa-supplicant-for-a-specific-wlan-interface
- https://forums.raspberrypi.com/viewtopic.php?t=211853
- https://stackoverflow.com/questions/66514910/enable-predictable-network-interfaces-via-shell-on-raspberry-pi
- https://forums.raspberrypi.com/viewtopic.php?t=198946

#### Setup

```
sudo vi /etc/wpa_supplicant/wpa_supplicant.conf
  country=HU
  
  network={
      ssid="TothKiss"
      psk="ToThKiSs"
  }
sudo killall -HUP wpa_supplicant
```

### Others

#### Connect to box

```
ssh signalk@192.168.72.189
ssh signalk@192.168.72.191
```

#### Wifi Commands

```
sudo su
ip link set dev wlan0 down
ip link set dev wlan0 up
iwlist wlan0 scan
vi wpa_supplicant/wpa_supplicant.conf
wpa_supplicant -B -c /etc/wpa_supplicant/wpa_supplicant.conf -i wlan0
wpa_cli terminate -i wlan0
```

