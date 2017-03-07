------------------------------------------------------------------------
   AudioLogic High Level APIs
------------------------------------------------------------------------

Testing: (from project directory bindings)
 * start binder:  ~/opt/bin/afb-daemon --ldpaths=./Alsa/src/low-level-binding:./BusinessLogic/audiologic-afb.so --roothttp=htdocs
 * connect browser on http://localhost:1234

 # Subscribe event for a given board
 http://localhost:1234/api/audio/subscribe&devid=hw:0

 # Increase Volume
 http://localhost:1234/api/audio/setvol?devid=hw:0&pcm=master&vol=50%

 # Get Volume
 http://localhost:1234/api/audio/getvol?devid=hw:0&pcm=master

 