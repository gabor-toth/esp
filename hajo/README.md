Signal K
========

- https://signalk.org/installation.html
    - https://demo.signalk.org/@signalk/instrumentpanel/
- https://github.com/SignalK/signalk-server#how-to-get-signal-k-server
- https://github.com/SignalK/signalk-server/blob/master/docker/README.md#quickstart

Docker
------

```
docker run -d --init  --name signalk-server -p 3000:3000 -v $(pwd):/home/node/.signalk cr.signalk.io/signalk/signalk-server
```

Local
-----

```
sudo apt isntall libavahi-compat-libdnssd-dev
sudo npm install -g mdns
sudo npm install -g signalk-server

```

Requests
---------

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

Other links
-----------

- [Discovery and Connection Establishment](https://signalk.org/specification/1.7.0/doc/connection.html)
- [Streaming API](https://signalk.org/specification/1.7.0/doc/streaming_api.html)
- [KIP](https://github.com/mxtommy/Kip)

Service Sniffer
===============

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

URLs
====

http://192.168.72.182/description.xml
http://192.168.72.182/index.html
http://192.168.72.182/signalk
ws://182.72.168.192:81/

MDNS
====

```
+ docker0 IPv4 6d65d96b9f13                                  _signalk-ws._tcp     local
= docker0 IPv4 6d65d96b9f13                                  _signalk-ws._tcp     local
  hostname = [6d65d96b9f13.local]
  address = [172.17.0.2]
  port = [3000]
  txt = ["vuuid=urn:mrn:signalk:uuid:7f446102-b734-40b4-a384-0ed9ee12579c" "self=urn:mrn:signalk:uuid:7f446102-b734-40b4-a384-0ed9ee12579c" "roles=master, main" "swvers=2.4.1" "swname=signalk-server" "txtvers=1"]
```

Raspberry PI
============

Install
-------

System

```
sudo apt install ssh unattended-upgrades
sudo touch /boot/ssh
reboot
sudo raspi-config
sudo apt update
sudo apt upgrade
sudo vi /etc/hosts
  + 127.0.1.1       solpi
  - 127.0.1.1       raspberrypi
vi /etc/inputrc
  history-search-*
sudo hostnamectl set-hostname solpi
sudo dpkg-reconfigure tzdata
sudo dpkg-reconfigure --priority=low unattended-upgrades

sudo vi /etc/wpa_supplicant/wpa_supplicant.conf
  country=HU
  
  network={
      ssid="TothKiss"
      psk="ToThKiSs"
  }
sudo killall -HUP wpa_supplicant 

mkdir .ssh
chmod 700 .ssh/
echo '...' > .ssh/authorized_keys
```

Signalk

```
curl -fsSL https://deb.nodesource.com/setup_lts.x | sudo -E bash -
sudo apt install nodejs npm -y
sudo npm install -g npm@latest
sudo apt install libnss-mdns avahi-utils libavahi-compat-libdnssd-dev -y
sudo apt-mark auto libnss-mdns
sudo npm install -g --unsafe-perm signalk-server -y
sudo signalk-server-setup

vi /etc/systemd/system/signalk.service
  [Unit]
  Wants=network.target
systemctl daemon-reload

```

Wifi
----

sudo raspi-config


Power
-----

- https://linuxhint.com/tips-tricks-optimize-power-consumption-raspberry-pi/
- https://raspberrypi.stackexchange.com/questions/114422/what-is-the-minimum-power-required-for-an-rpi-4-in-halt-or-shutdown/114423#114423

Turn off bus power for USB and Ethernet
---------------------------------------

- https://forums.raspberrypi.com/viewtopic.php?t=138888

sudo nano /etc/rc.local

```
sleep 10
echo '1-1' | sudo tee /sys/bus/usb/drivers/usb/unbind
```

or

```
echo -n 0x0 | sudo tee /sys/devices/platform/soc/3f980000.usb/buspower
```

Tweak hardware settings
-----------------------

- https://linuxhint.com/tips-tricks-optimize-power-consumption-raspberry-pi/

vi /boot/config.txt

```
dtoverlay=pi3-disable-bt
dtparam=act_led_trigger=none
dtparam=act_led_activelow=off
dtparam=pwr_led_trigger=none
dtparam=pwr_led_activelow=off

arm_freq_min=200
core_freq_min=100
sdram_freq_min=50
over_voltage_min=0

sudo vcgencmd display_power 0
display_power=1 ???
```
