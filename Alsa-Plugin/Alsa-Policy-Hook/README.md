Alsa-Hook-Plugin

Object: Provide a Hook on Alsa PCM to check permission again AGL Advance Audio Agent
Status: Working Proof of Concept
Author: Fulup Ar Foll fulup@iot.bzh
Date  : August-2017

Functionalities:
 - Execute a set of websocket RPC request again AGL binders to allow/deny access
 - Keep websocket open in an idepandant thread on order to monitor event receive from AGL audio agent 

Installation
 - Alsaplugins are typically search in /usr/share/alsa-lib. Nevertheless a full path might be given
 - This plugin implement a hook on a slave PCM. Typically this slave PCM is a dedicated virtual channel (eg: navigation, emergency,...)
 - Config should be place in ~/.asoundrc (see config sample in PROJECT_ROOT/conf.d/alsa)

Config
```
# Define sharelib location and entry point
# -----------------------------------------
pcm_hook_type.MyHookPlugin {
    install "AlsaInstallHook" 
    lib "/home/fulup/Workspace/AGL-AppFW/audio-bindings-dev/build/Alsa-Plugin/Alsa-Hook-Callback/alsa_hook_cb.so"
}

# Create PCM HOOK with corresponding request calls to AGL Audio Agent
# --------------------------------------------------------------------
pcm.MyNavigationHook {
    type hooks
    slave.pcm "MyMixerPCM" # Target PCM to effectively play sound 

    # Call request that should succeed before accessing slave PCM
    hooks.0 {
        type "MyHookPlugin"   # Name=xxxx should match with pcm_hook_type.xxxx
        hook_args {

            # URI to AGL-Advance-Audio-Agent binder
            uri   "ws://localhost:1234/api?token='audio-agent-token'"

            # Call request to send to binder (initial label is free, query is optional)
            request {
                MyFirstCheck {
                    api   "alsacore"
                    verb  "ping"
                } 
                MySecondCheck {
                    api   "alsacore"
                    verb  "ucmset"
                    query "{'devid':'hw:v1340','verb':'Navigation','dev':'speakers'}"
                }    
            }
        }
    }
}

```

NOTE: 

* Hook plugin is loaded by Alsa libasound within player context. It inherits client process attributes, as UID/GID or
the SMACK label on AGL. This smack label is tested by AGL security framework when requested a call on the audio-agent binder.
As a result the call will only succeed it the permission attached the application in Cynara matches. 

* Hook plugin keep a connection with the Audio-Agent until PCM is closed by the application. This connection allow the
Audio-Agent to send events. eg: pause, quit, mute, ...