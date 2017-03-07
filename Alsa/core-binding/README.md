------------------------------------------------------------------------
   AlsaCore Low level binding maps AlsaLib APIs
------------------------------------------------------------------------

Testing: (from Build dir with Binder install in $HOME/opt=
 * start binder:  ~/opt/bin/afb-daemon --ldpaths=./Alsa/src/low-level-binding
 * connect browser on http://localhost:1234/api/alsacore/????

 # List Avaliable Sound cards
 http://localhost:1234/api/alsacore/getinfo

 # Get Info on a given Sound Card
 http://localhost:1234/api/alsacore/getinfo?devid=hw:0

 # Get all controls from a given sound card
 http://localhost:1234/api/alsacore/getctl?devid=hw:0

 # Get detail on a given control (optional quiet=0=verbose,1,2)
 http://localhost:1234/api/alsacore/getctl?devid=hw:0&numid=1&quiet=0

 # Subscribe to event from a given sound card (warning fail if not WS)
 http://localhost:1234/api/alsacore/subctl?devid=hw:0