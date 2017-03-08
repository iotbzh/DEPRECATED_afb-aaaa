------------------------------------------------------------------------
   AlsaCore Low level binding maps AlsaLib APIs
------------------------------------------------------------------------

Testing: (from project directory bindings)
 * start binder:  ~/opt/bin/afb-daemon --ldpaths=./build --roothttp=htdocs
 * connect browser on http://localhost:1234

 # List Avaliable Sound cards
 http://localhost:1234/api/alsacore/getinfo

 # Get Info on a given Sound Card
 http://localhost:1234/api/alsacore/getinfo?devid=hw:0

 # Get all controls from a given sound card
 http://localhost:1234/api/alsacore/getctl?devid=hw:0

 # Get detail on a given control (optional quiet=0=verbose,1,2)
 http://localhost:1234/api/alsacore/getctl?devid=hw:0&numid=1&quiet=0

