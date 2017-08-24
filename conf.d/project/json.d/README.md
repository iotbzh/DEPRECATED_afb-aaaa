By default controller searches for a config filename with the same 'middlename' as daemon process. As an example if your process name is afb-daemon then middle name is 'daemon'.

```
 onload-middlename-xxxxx.json

 # Middlename is taken from process middlename.
```

You may overload config search path with environement variables
 * AFB_BINDER_NAME: change patern config search path. 'export AFB_BINDER_NAME=sample' will make controller to search for a configfile name 'onload-sample-xxx.json'.
 * CONTROL_CONFIG_PATH: change default reserch path for configuration. You may provide multiple directories separated by ':'.
 * CONTROL_LUA_PATH: same as CONTROL_CONFIG_PATH but for Lua script files.

Example to load a config name 'onload-myconfig-test.json' do
```
  AFB_BINDER_NAME='myconfig' afb-daemon --verbose ...'
```

Note: you may change search pattern for Lua script by adding 'ctlname=afb-middlename-xxx' in the metadata section of your config 'onload-*.json'

WARNING: Audio Control are the one from the HAL and not from Alsa LowLevel

