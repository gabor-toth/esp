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

curl -X GET http://10.128.65.180:3000/signalk
{"endpoints":{"v1":{"version":"2.4.1","
signalk-http":"http://10.128.65.180:3000/signalk/v1/api/","signalk-ws":"ws://10.128.65.180:3000/signalk/v1/stream","signalk-tcp":"tcp://10.128.65.180:8375"}},"server":{"id":"signalk-server-node","version":"2.4.1"}}

wscat -c "ws://10.128.65.180:3000/signalk/v1/stream?subscribe=all"
Connected (press CTRL+C to quit)
< {"name":"signalk-server","version":"2.4.1","self":"vessels.urn:mrn:signalk:uuid:
59e1f1c9-9e32-4340-a1d0-656512c48f0a","roles":["master","main"],"timestamp":"2023-11-22T12:55:47.852Z"}
< {"context":"vessels.urn:mrn:signalk:uuid:59e1f1c9-9e32-4340-a1d0-656512c48f0a","updates":[{"$source":"defaults","
timestamp":"2023-11-22T12:43:36.969Z","
values":[{"path":"","value":{"uuid":"urn:mrn:signalk:uuid:59e1f1c9-9e32-4340-a1d0-656512c48f0a"}}]}]}

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

