Signal K
========

- https://signalk.org/installation.html
    - https://demo.signalk.org/@signalk/instrumentpanel/
- https://github.com/SignalK/signalk-server#how-to-get-signal-k-server
- https://github.com/SignalK/signalk-server/blob/master/docker/README.md#quickstart

```
docker run -d --init  --name signalk-server -p 3000:3000 -v $(pwd):/home/node/.signalk cr.signalk.io/signalk/signalk-server
```

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

