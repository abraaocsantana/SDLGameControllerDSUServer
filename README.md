# SDL Game Controller DSU Server
---
## DSU Server ( [Cemuhook Motion Provider Protocol](https://github.com/v1993/cemuhook-protocol) ) for SDL gamepads
---

This version was hacked together in an afternoon to provide motion controls to my **Steam Controller** when it's connected through BLE (Bluetooth Low Energy).
It may work with other devices supported by SDL2.


(Nowadays, most software that supports SDU also supports motion directly through SDL, but when the input goes through Steam all motion data is thrown away :( )

#### Quick info
Server running on the default port 26760

### TODO
* Add non-motion data, add real data to the controller info
* Re-do and reorganize the C-like code that deals with SDL
