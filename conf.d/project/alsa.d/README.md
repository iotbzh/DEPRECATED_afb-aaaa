
Alsa Configuration is not complex, but it's heavy and every except intuitive. 

In order to set your configuration move step by step. And verify at each new step that you did not introduce a regression.

### Make sure your board is not taken by PulseAudio

* Pavucontrol is your friend. Go in device configuration and switch to off the device you want to make available to ALSA.
* Check for your device list with 'play -l'. Note that just after card number your get an alias for your sound card. 
  it is simpler to use this alias, card number will change depending on plug/unplug of device when this alias name
  is more stable (except for few stupid driver who use 'USB' as sound card name.
* When your know your sound alias (eg:v1340 in mu case) you can test it with 'speaker-test -D v1340 -c2 -twav'. You may also
  use sound card number with 'speaker-test -D hw:0 -c2 -twav'. Nevertheless as said previously this number is not stable.
* you are now ready to write your $HOME/.asoundrc config

Note that $HOME/.asoundrc is loaded within libasound client and not by Alsa kernel, this is the reason why you do not need
to activate any control or restart a daemon for modifications to be taken in account.

To use ALSA with AAAA and the controller you need to write 1 section in your ALSA config
 
* Sound Card Mixer: Allows multiple audio stream to be played on the same sound card. If hardware support mixer, Alsa will use it. If  
  not it will provide mixing by software.
* Audio Role Volume: They provide independent volume for each audio role. For reference, we use Alsa softvolume, depending on 
  your hardware you may have this directly avaliable from your sound card.
* Authorised Audio PCM: those channel are designed for applications we do not trust and then enforce AAAA control check
  before granting the access to a given channel. 

### Sound Card Mixer

```
pcm.SoundCardMixer {
    type dmix
    ipc_key 1024
    ipc_key_add_uid false
    ipc_perm 0666   # mixing for all users

    # Define target effective sound card (cannot be a plugin)
    slave { pcm "hw:v1340" } #Jabra Solmate

    # DMIX can only map two channels
    bindings {
        0 0
        1 1
    }
}
```

Warning: if you have more than one Mixer each of them should have a unique ipc_key. You sound card alias move in the slave section.
When this is done you may try your mixer with:

```
 speaker-test -D MyMixerPCM -c2 -twav
```


### Audio Role

```
pcm.NavigationRole {
    type        softvol

    # Point Slave on HOOK for policies control
    slave.pcm   "SoundCardMixer"

    # name should match with HAL but do not set card=xx
    control.name    "Playback Navigation"

}

```

The slave you point to your SoundCardMixer, and the control.name should be EXACTLY the same as the one defined in your HAL.


WARNING: The control here  "Playback Navigation" is a user defined kernel control. It means that this kernel is created in
kernel space, but that its action happen in user space. When create those control remains visible until you reset your
sound card (eg: unplug USB), but they are save each time you reboot. It is recommended to start AAAA binder before testing
your softvol audio role channel. If you do the opposite the control will be create by Alsa Plugin and this will not inherit
of the default value you have in your HAL.

When in place you should test it with:
```
 speaker-test -D NavigationRole -c2 -twav
```

IMPORTANT: control volume are attache to your physical hardware and not to intermediary level (Softvol or Mixer). To see the
newly created channel you should use
```
  amixer -Dhw:v1340 controls | grep -i playback
```

## Authorised Audio PCM

This PCM is supervised with the AAAA audio hook plugin. The pluging and will any application request on this PCM and will
1st request an autorisation from AAAA controller to grant access for the client application. To do so, two things:
* the plugin should be declared (only once)
* you should declare as many authorized PCM as you need.

### Plugin declaration

```
pcm_hook_type.MyHookPlugin {
    install "AlsaInstallHook"
    lib "/home/fulup/Workspace/AGL-AppFW/audio-bindings-dev/build/Alsa-Plugin/Alsa-Policy-Hook/policy_hook_cb.so"
}

```

Lib is the path where to find AAAA Alsa hook plugin, install is the name of the init function it should not be changed.


When your plugin is defined you may declare your authorised PCM. Those PCM like softvol will take a slave, typically a lower
level of the audio role, or directly a mixer if your goal is to protect directly the Mixer. The AAAA Plugin hook take as

```
pcm.AuthorisedToNavigationOnly {
    type hooks
    slave.pcm "NavigationRole"
    # Defined used hook sharelib and provide arguments/config to install func
    hooks.0 {
        type "MyHookPlugin"
        hook_args {
            verbose true # print few log messages (default false);

            # Every Call should return OK in order PCM to open (default timeout 100ms)
            uri   "ws://localhost:1234/api?token=audio-agent-token&uuid=audio-agent-session"
            request {
                # Request autorisation to write on navigation
                RequestNavigation {
                    api    "control"
                    verb   "dispatch"
	            query  "{'target':'navigation', 'args':{'device':'Jabra SOLEMATE v1.34.0'}}"
                }
            }
            # map event reception to self generated signal
            event {
                pause  30
                resume 31
                stop   3
            }
        }
    }
}

```

 * The slave is the PCM that application will be transfer to if access to control is granted. 
 * Request is a suite à control action that respond to AGL standard application framework API
 * Event is the mapping of signal an appplication will receive in case AAAA controller push event to the audio application.

When using AAAA controller, most action should be transfert to the controller that will take action to authorise/deny the access. 
Nevertheless it is also possible to directly access a lower level (e.g. call alsa Use Case Manager). People may also have their
own audio policy engine and request it directly from here.

To test this last part your need to have a controller ready to respond to your request. Otherwise control will systematically
be denied. When your AAAA controller is ready to serve your request you may check this with
```  
  amixer -Dhw:AuthorisedToMusicOnly controls | grep -i playback
  amixer -Dhw:AuthorisedToNavigationOnly controls | grep -i playback
```

IMPORTANT: you need at least to audio role to really test this part. While you may assert with one channel that your flow
to accept/deny application works. You need two simultaneous audio stream to really play with the control. Typically when playing
music if you send a navigation message then the audio will be lower during the rendering of the navigation message.

The action on how you lower an audio role when an other one with a higger level of priority come in place not defined at the
plugin level, but in the AAAA controller, where the API control/dispatch?target=xxxxx will execute a set of actions corresponding
the set/unset accept/deny of requested control.

Remark: to understand what is happening it is a good idea to have an alxamixer option on the your soundcard
```
 amixer -Dhw:v1340
```

(!) Do not forget to replace 'hw:v1340' by what ever is the alias of your sound card.