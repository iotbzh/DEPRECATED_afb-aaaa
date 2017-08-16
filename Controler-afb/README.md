Controler AAAA(AGL Advance Audio Controler) and more.
------------------------------------------------------------

 * Object: Generic Controler to handle Policy,Small Business Logic, Glue in between components, ... 
 * Status: Release Candidate
 * Author: Fulup Ar Foll fulup@iot.bzh
 * Date  : August-2017

## Functionalities:
 - Create an application dedicate controller from a JSON config file
 - Each controls (eg: navigation, multimedia, ...) is a suite of actions. When all actions succeed control is granted, if one fail control acces is denied.
 - Actions can either be:
   + Invocation to an other binding API, either internal or external (eg: a policy service, Alsa UCM, ...)
   + C routines from a user provider plugin (eg: policy routine, proprietary code, ...)
   + Lua script function. Lua provides access to every AGL appfw functionalities and can be extended from C user provided plugins.

## Installation
 - Controler is a native part of AGL Advance Audio Framework but may be used independently with any other service or application binder.
 - Dependencies: the only dependencies are audio-common for JSON-WRAP and Filescan-utils capabilities.
 - Controler relies on Lua-5.3, when not needed Lua might be removed at compilation time.

## Config

Configuration is loaded dynamically during startup time. The controller scans CONTROL_CONFIG_PATH for a file corresponding to pattern
"onload-bindername-xxxxx.json". When controller runs within AAAA binder it searches for "onload-audio-xxxx.json". First file found in the path the loaded
any other files corresponding to the same pather are ignored and only generate a warning.

Each bloc in the configuration file are defined with
 * label: must be provided is used either for debugging or as input for the action (eg: signal name, control name, ...)
 * info:  optional used for documentation purpose only

### Config is organised in 4 sections:

 * metadata 
 * onload defines the set of action to be executed at startup time
 * control defines the set of controls with corresponding actions
 * event define the set of actions to be executed when receiving a given signal

### Metadata
 
As today matadata is only used for documentation purpose. 
 * label + version mandatory
 * info optional

### OnLoad section

Defines startup time configuration. Onload may provide multiple initialisation profiles, each with a different label.  
 * label is mandatory. Label is used to select onload profile at initialisation through DispatchOneOnLoad("onload-label") API;
 * info is optional
 * plugin provides optional unique plugin name. Plugin should follow "onload-bindername-xxxxx.ctlso" patern
   and are search into CONTROL_PLUGIN_PATH. When defined controller will execute user provided function context=CTLP_ONLOAD(label,version,info).
   The context returned by this routine is provided back to any C routines call later by the controller. Note that Lua2C function
   are prefix in Lua script with plugin label (eg: MyPlug: in following config sample)
 * lua2c list of Lua commands shipped with provided plugin.
 * require list of binding that should be initialised before the controller starts. Note that some listed requirer binding might be absent,
   nevertheless any present binding from this list will be started before controller binding, missing ones generate a warning.
 * action the list of action to execute during loadtime. Any failure in action will prevent controller binding from starting.

### Control section

Defines a list of controls that are accessible through (api="control", verb="request", control="control-label"). 

 * label mandatory
 * info optional
 * permissions Cynara needed privileges to request this control (same as AppFw-V2)
 * action the list of actions

### Event section

Defines a list of actions to be executed on event reception. Even can do anything a controller can (change state,
send back signal, ...) eg: if a controller subscribes to vehicule speed, then speed-event may ajust master-volume to speed.

 * label mandatory
 * info optional
 * action the list of actions

### Actions Categories

Controler support tree categories of actions. Each action return a status status where 0=success and 1=failure.
 * AppFw API, Provides a generic model to request other bindings. Requested binding can be local (eg: ALSA/UCM) or
   external (eg: vehicle signalling).
    * api  provides requested binding API name
    * verb provides verb to requested binding 
    * args optionally provides a jsonc object for targeted binding API. Note that 'args' are statically defined
       in JSON configuration file. Controler client may also provided its own arguments from the query list. Targeted
       binding receives both arguments defined in the config file and the argument provided by controller client. 
 * C-API, when defined in the onload section, the plugin may provided C native API with CTLP-CAPI(apiname, label, args, query, context).
   Plugin may also create Lua command with  CTLP-Lua2C(LuaFuncName, label, args, query, context). Where args+query are JSONC object
   and context the value return from CTLP_ONLOAD function. Any missing value is set to NULL.
 * Lua-API, when compiled with Lua option, the controller support action defined directly in Lua script. During "onload" phase the
   controller search in CONTROL_Lua_PATH file with pattern "onload-bindername-xxxx.lua". Any file corresponding to this pattern
   is automatically loaded. Any function defined in those Lua script can be called through a controller action. Lua functions receive
   three parameters (label, args, query).

Note: Lua added functions systematically prefix. AGL standard AppFw functions are prefixed with AGL: (eg: AGL:notice(), AGL_success(), ...). 
User Lua functions added though the plugin and CTLP_Lua2C are prefix with plugin label (eg: MyPlug:HelloWorld1).

## Config Sample

Here after a simple configuration sample.

```
{
    "$schema": "ToBeDone",
    "metadata": {
        "label": "sample-audio-control",
        "info": "Provide Default Audio Policy for Multimedia, Navigation and Emergency",
        "version": "1.0"
    },
    "onload": [{
            "label": "onload-default",
            "info": "onload initialisation config",
                        "plugin": {
                "label" : "MyPlug",
                "sharelib": "ctl-audio-plugin-sample.ctlso",
                "lua2c": ["Lua2cHelloWorld1", "Lua2cHelloWorld2"]
            },
            "require": ["intel-hda", "jabra-usb", "scarlett-usb"],
            "actions": [
                {
                    "label": "onload-sample-cb",
                    "info": "Call control sharelib install entrypoint",
                    "callback": "SamplePolicyInit",
                    "args": {
                        "arg1": "first_arg",
                        "nextarg": "second arg value"
                    }
                }, {
                    "label": "onload-sample-api",
                    "info": "Assert AlsaCore Presence",   
                    "api": "alsacore",
                    "verb": "ping",
                    "args": "test"
                }, {
                    "label": "onload-hal-lua",
                    "info": "Load avaliable HALs",
                    "lua": "Audio_Init_Hal"
                }
            ]
        }],
    "controls": 
            [
                {
                    "label": "multimedia",
                    "permissions": "urn:AGL:permission:audio:public:mutimedia",
                    "actions": {
                            "label": "multimedia-control-lua",
                            "info": "Call Lua Script function Test_Lua_Engin",   
                            "lua": "Audio_Set_Multimedia"
                        }
                }, {
                    "label": "navigation",
                    "permissions": "urn:AGL:permission:audio:public:navigation",
                    "actions": {
                            "label": "navigation-control-lua",
                            "info": "Call Lua Script to set Navigation",   
                            "lua": "Audio_Set_Navigation"
                        }
                }, {
                    "label": "emergency",
                    "permissions": "urn:AGL:permission:audio:public:emergency",
                    "actions": {
                            "label": "emergency-control-ucm",
                            "lua": "Audio_Set_Emergency"
                        }
                }, {
                    "label": "multi-step-sample",
                    "info" : "all actions must succeed for control to be accepted",
                    "actions": [{
                            "label": "multimedia-control-cb",
                            "info": "Call Sharelib Sample Callback",
                            "callback": "sampleControlNavigation",
                            "args": {
                                "arg1": "snoopy",
                                "arg2": "toto"
                            }
                        }, {
                            "label": "navigation-control-ucm",
                            "api": "alsacore",
                            "verb": "ping",
                            "args": {
                                "test": "navigation"
                            }
                        }, {
                            "label": "navigation-control-lua",
                            "info": "Call Lua Script to set Navigation",   
                            "lua": "Audio_Set_Navigation"
                        }]
                }
            ],
    "events":
            [
                {
                    "label": "Vehicle-Speed",
                    "info": "Action when Vehicule speed change",
                    "actions": [
                        {
                            "label": "speed-action-1",
                            "callback": "Blink-when-over-130",
                            "args": {
                                "speed": 130
                                "blink-speed": 1000
                            }
                        }, {
                            "label": "Adjust-Volume",
                            "lua": "Adjust_Volume_To_Speed",
                        }
                    ]
                },
                {
                    "label": "Reverse-Engage",
                    "info": "When Reverse Gear is Engage",
                    "actions": [
                        {
                            "label": "Display-Rear-Camera",
                            "callback": "Display-Rear-Camera",
                        }, {
                            "label": "Prevent-Phone-Call",
                            "api"  : "phone",
                            "verb" : "status",
                            "args": {
                                "call-accepted": false
                            }
                        }
                    ]
                },
                {
                    "label": "Neutral-Engage",
                    "info": "When Reverse Neutral is Engage",
                    "actions": [
                        {
                            "label": "Authorize-Video",
                            "api"  : "video",
                            "verb" : "status",
                            "args": {
                                "tv-accepted": true
                            }
                        }
                    ]
                }
            ]
}

```

