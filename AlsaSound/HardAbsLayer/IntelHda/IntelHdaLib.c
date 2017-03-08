/*
 * Copyright (C) 2016 "IoT.bzh"
 * Author Fulup Ar Foll <fulup@iot.bzh>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * reference: 
 *   amixer contents; amixer controls;
 *   http://www.tldp.org/HOWTO/Alsa-sound-6.html 
 */
#define _GNU_SOURCE  // needed for vasprintf
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>

#include "IntelHdaLib.h"
#include "AlsaMixerHal.h"

static struct afb_service srvitf;

// Normalise Intel-HDA numid    
static AlsaHalMapT  alsaCtlsMap[]= { // check with amixer -D hw:xx controls; amixer -D "hw:3" cget numid=xx
 /* ACTION(enum) NUMID(int) IFACE(enum) VALUES(int) MinVal(int) MaxVal(int) Step(int) ACL=READ|WRITE|RW INFO=comment */
  { .action=Master_Playback_Volume, .numid=16, .group=OUTVOL, .values=1, .minval=0, .maxval= 87 , .step=0, .acl=RW, .info= "Master Playback Volume" },
  { .action=PCM_Playback_Volume   , .numid=27, .group=PCMVOL, .values=2, .minval=0, .maxval= 255, .step=0, .acl=RW, .info= "PCM Playback Volume" },
  { .action=PCM_Playback_Switch   , .numid=17, .group=SWITCH, .values=1, .minval=0, .maxval= 1  , .step=0, .acl=RW, .info= "Master Playback Switch" },
  { .action=Capture_Volume        , .numid=12, .group=INVOL , .values=2, .minval=0, .maxval= 31 , .step=0, .acl=RW, .info= "Capture Volume" },
  { .numid=0  } /* marker for end of the array */
} ;

// Warning: Longname is used to locate board on the system
static AlsaHalSndT alsaSndCard  = {
    .longname= "HDA Intel PCH",
    .info    = "Hardware Abstraction Layer for IntelHDA sound card",
    .ctls    = alsaCtlsMap,
};


// This callback is fired when afb_service_call for api/alsacore/subctl returns
STATIC void alsaSubcribeCB (void *handle, int iserror, struct json_object *result) {
	struct afb_req request = afb_req_unstore(handle);
        struct json_object *x, *resp = NULL;
	const char *info = NULL;

	if (result) {
	    INFO (afbIface, "result=[%s]\n", json_object_to_json_string (result));
            if (json_object_object_get_ex(result, "request", &x) && json_object_object_get_ex(x, "info", &x))
            info = json_object_get_string(x);
            if (!json_object_object_get_ex(result, "response", &resp)) resp = NULL;
        }
        
        // push message respond
	if (iserror) afb_req_fail_f(request,  "Fail", info);
	else         afb_req_success(request, resp, info);
        
        // free calling request
	afb_req_unref(request);
}

// Create and subscribe to alsacore ctl events
PUBLIC void intelHdaMonitor(struct afb_req request) {
    
    // save request in session as it might be used after return by callback
    struct afb_req *handle = afb_req_store(request);

    // push request to low level binding
    if (!handle) afb_req_fail(request, "error", "out of memory");
    else    afb_service_call(srvitf, "alsacore", "subctl", json_object_get(afb_req_json(request)), alsaSubcribeCB, handle);

    // success/failure messages return from callback    
}

// Subscribe to AudioBinding events
PUBLIC void intelHdaSubscribe (struct afb_req request) {
    const char *devid = afb_req_value(request, "devid");
    if (devid == NULL) {
        afb_req_fail_f (request, "devid-missing", "devid=hw:xxx missing");
    }
    
}


// Call when all bindings are loaded and ready to accept request
PUBLIC void intelHdaGetVol(struct afb_req request) {
   
    // Should call here Hardware Alsa Abstraction Layer for corresponding Sound Card
    afb_req_success (request, NULL, NULL);
    return;
    
}

PUBLIC void intelHdaSetVol(struct afb_req request) {
    const char *arg;
    char *pcm;
    
    arg = afb_req_value(request, "vol");
    if (arg == NULL) {
        afb_req_fail_f (request, "argument-missing", "vol=[0,100] missing");
        goto ExitOnError;
    }
    
    arg = afb_req_value(request, "pcm");
    if (arg == NULL) pcm="Master";
   
    // Should call here Hardware Alsa Abstraction Layer for corresponding Sound Card
    afb_req_success (request, NULL, NULL);
    return;
    
  ExitOnError:
    return;
    
}


// this function is call after all binder are loaded and initialised
PUBLIC int  intelHdaInit (struct afb_service service, const char *cardname) {
    srvitf = service;
    
    // API prefix is used as sndcard halname
    alsaSndCard.halname= cardname;
    return 0;
}