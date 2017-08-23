Hal-Plugin

Object: Add virtual soft control to sound card
Status: Proof of concept and not a usable product
Author: Fulup Ar Foll
Date  : June-2017

HAL-plugin allows:
 - to expose any existing NUMID under a customized label name, this is order to abstract sound card config.
 - to add non alsa NUMID supported through a callback mechanism (eg: volume rampdown, power off, ...)

installation
 - Plugin should be placed where ever alsaplugins are located (typically: /usr/share/alsa-lib)
 - Callback sharelib directory should be declare in $LD_LIBRARY_PATH

Config
```
cat ~/.asoundrc
  ctl.agl_hal {
    type  afbhal
    devid "hw:4"
    cblib "afbhal_cb_sample.so"
    ctls {
        # ctlLabel {numid integer name "Alsa Ctl Name"}
        MasterSwitch { numid 4 name "My_First_Control" }
        MasterVol    { numid 5 name "My_Second_Control" }
        CB_sample    { ctlcb @AfbHalSampleCB name "My_Sample_Callback"}
    }
    pcm.agl_hal {
        type copy     # Copy PCM
        slave "hw:4"  # Slave name
  }
```

With such a config file
 - numid=4 from sndcard=hw:4 is renamed "My_First_Control"
 - numid=4 from sndcard=hw:4 is renamed "My_Second_Control"
 - numid=4 will call AfbHalSampleCB from afbhal_cb_sample.so

Note: to really implement a usable HAL at minimum every ALSA call should be implemented and read/write of values should be normalised from 0 to 100% with a step of 1.

Conclusion: This demonstrate that implementing a HAL that both abstract ALSA get/set and enable non ALSA support is possible at an acceptable cost
without having to hack ALSA source code. The beauty of the model is than it is fully transparent to any ALSA application. The limit is that the plugin is loaded
within every application context, thus interaction with an external event loop remains complete as well as conflict management in case of share resources.
