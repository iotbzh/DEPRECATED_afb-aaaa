------------------------------------------------------------------------
   AlsaCore Low level binding maps AlsaLib APIs
------------------------------------------------------------------------

Testing: (from project directory bindings)
 * start binder:  ~/opt/bin/afb-daemon --ldpaths=./build --token=mysecret --roothttp=htdocs
 * connect browser on http://localhost:1234?devid=hw:0

 # List Avaliable Sound cards
 http://localhost:1234/api/alsacore/getinfo

 # Get Info on a given Sound Card
 http://localhost:1234/api/alsacore/getinfo?devid=hw:0

 # Get shortname/longname for a given card
 http://localhost:1234/api/alsacore/getcardid?devid=hw:0

 # Get all controls from a given sound card
 http://localhost:1234/api/alsacore/getctl?devid=hw:0

 # Get detail on a given control (optional mode=0=verbose,1,2)
 http://localhost:1234/api/alsacore/getctl?devid=hw:0&numid=1&mode=0

# Debug event with afb-client-demo
```
 ~/opt/bin/afb-client-demo localhost:1234/api?token=mysecret
 alsacore subscribe {"devid":"hw:0"}
```

# Open AlsaMixer and play with Volume
```
 alsamixer -D hw:0
```
