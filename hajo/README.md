Signal K
========

- https://signalk.org/installation.html
    - https://demo.signalk.org/@signalk/instrumentpanel/
- https://github.com/SignalK/signalk-server#how-to-get-signal-k-server
- https://github.com/SignalK/signalk-server/blob/master/docker/README.md#quickstart

```
docker run -d --init  --name signalk-server -p 3000:3000 -v $(pwd):/home/node/.signalk cr.signalk.io/signalk/signalk-server
```
