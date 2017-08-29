------------------------------------------------------------------------
#                AAAA-demo Fulup setup
------------------------------------------------------------------------

1) Check Alsa config (see config.d/project/alsa.d)

 start afb-aaaa
````
cd afb-aaaa/build
cmake ..
make populate

# Define Controller environment
 export AFB_BINDER_NAME=aaaa
 export CONTROL_CONFIG_PATH=../conf.d/json.d
 export CONTROL_LUA_PATH=../conf.d/lua.d

afb-daemon --port=1234 --workdir=. --roothttp=../htdocs --token= --verbose \
  --alias=/monitoring:${HOME}/opt/afb-monitoring  --ldpaths=package/lib \
  --alias=/monitoring:/home/fulup/opt/afb-monitoring  \
  --binding=../../afb-controller/build/package/lib/afb-controller.so \
  --ws-server=unix:/var/tmp/afb-ws/alsacore \
  --ws-server=unix:/var/tmp/afb-ws/controller
  
````

We may now check AlsaHookPlugin. If your 

```
 speaker-test -D NavigationRole -c2 -twav
```
  


2) Start one Media Player Daemon per Audio Role

 mpd.conf should point on valid PMC as defined in .asoundrc in previous step.

 * mpd --no-daemon --stderr ~/opt/etc/mpd.d/multimedia-mpd.conf
 * mpd --no-daemon --stderr ~/opt/etc/mpd.d/navigation-mpd.conf
 * mpd --no-daemon --stderr ~/opt/etc/mpd.d/emergency-mpd.conf 

3) Check Music Player Daemon works

 * MPD_PORT=6601 mpc ls   # multimedia
 * MPD_PORT=6602 mpc ls   # navigation
 * MPD_PORT=6603 mpc ls   # emergency

4) Start MPDC Binder

Note: debug/monitoring port should be different from the one use by AAAA-binder 


```
cd afb-mpdc/build
cmake ..
make populate
afb-daemon --port=1235 --alias=/monitoring:/home/fulup/opt/afb-monitoring \
   --ldpaths=package --workdir=. --roothttp=../htdocs --token= --verbose \
   --ws-server=unix:/var/tmp/afb-ws/mpdc \
   --ws-client=unix:/var/tmp/afb-ws/control
```
