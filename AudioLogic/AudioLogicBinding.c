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
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>

#include "AudioLogicLib.h"

PUBLIC const struct afb_binding_interface *afbIface;   

STATIC void localping(struct afb_req request) {
    json_object *query = afb_req_json(request);
    afb_req_success(request, query, NULL); 
}


/*
 * array of the verbs exported to afb-daemon
 */
STATIC const struct afb_verb_desc_v1 binding_verbs[] = {
  /* VERB'S NAME            SESSION MANAGEMENT          FUNCTION TO CALL         SHORT DESCRIPTION */
  { .name= "ping"   ,    .session= AFB_SESSION_NONE,  .callback= localping,      .info= "Ping Binding" },
  { .name= "setvolume",  .session= AFB_SESSION_CHECK, .callback= audioLogicSetVol,    .info= "Set Volume" },
  { .name= "getvolume",  .session= AFB_SESSION_CHECK, .callback= audioLogicGetVol,    .info= "Get Volume" },
  { .name= "subscribe",  .session= AFB_SESSION_CHECK, .callback= audioLogicSubscribe, .info= "Subscribe AudioBinding Events" },
  { .name= "monitor",    .session= AFB_SESSION_CHECK, .callback= audioLogicMonitor,   .info= "Activate AlsaCtl Monitoring" },
  { .name= "open",       .session= AFB_SESSION_CREATE,.callback= audioLogicOpen,      .info= "Open a Dedicated SoundCard" },
  { .name= "close",      .session= AFB_SESSION_CLOSE, .callback= audioLogicClose,     .info= "Close previously open SoundCard" },
  { .name= NULL } /* marker for end of the array */
};

/*
 * description of the binding for afb-daemon
 */
STATIC const struct afb_binding binding_description = {
  /* description conforms to VERSION 1 */
  .type= AFB_BINDING_VERSION_1,
  .v1= {
    .prefix= "audio",
    .info= "High Level Interface to Audio bindings",
    .verbs = binding_verbs
  }
};

// This receive all event this binding subscribe to 
PUBLIC void afbBindingV1ServiceEvent(const char *evtname, struct json_object *object) {
  
    NOTICE (afbIface, "afbBindingV1ServiceEvent evtname=%s [msg=%s]", evtname, json_object_to_json_string(object));
}

// this is call when after all bindings are loaded
PUBLIC int afbBindingV1ServiceInit(struct afb_service service) {
    
   return (audioLogicInit(service)); 
};

/*
 * activation function for registering the binding called by afb-daemon
 */
PUBLIC const struct afb_binding *afbBindingV1Register(const struct afb_binding_interface *itf) {
    afbIface= itf;
    
    return &binding_description;	/* returns the description of the binding */
}

