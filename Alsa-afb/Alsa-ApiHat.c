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
#include <sys/prctl.h>


#include "Alsa-ApiHat.h"

STATIC int AlsaInit(void) {
    int rc= prctl(PR_SET_NAME, "afb-aaaa-agent",NULL,NULL,NULL);
    if (rc) AFB_ERROR("ERROR: AlsaCore fail to rename process");

    return rc;
}

/*
 * array of the verbs exported to afb-daemon
 */
static const struct afb_verb_v2 api_verbs[] = {
    /* VERB'S NAME          FUNCTION TO CALL  */
    { .verb = "ping", .callback = pingtest, .info="Ping Presence Check on API"},
    { .verb = "infoget", .callback = alsaGetInfo, .info="Return sound cards list"},
    { .verb = "ctlget", .callback = alsaGetCtls, .info="Get one or many control values"},
    { .verb = "ctlset", .callback = alsaSetCtls, .info="Set one control or more"},
    { .verb = "subscribe", .callback = alsaEvtSubcribe, .info="subscribe to alsa events"},
    { .verb = "cardidget", .callback = alsaGetCardId, .info="get sound card id"},
    { .verb = "halregister", .callback = alsaRegisterHal, .info="register a new HAL in alsacore"},
    { .verb = "hallist", .callback = alsaActiveHal, .info="Get list of currently active HAL"},
    { .verb = "ucmquery", .callback = alsaUseCaseQuery,.info="Use Case Manager Query"},
    { .verb = "ucmset", .callback = alsaUseCaseSet,.info="Use Case Manager set"},
    { .verb = "ucmget", .callback = alsaUseCaseGet,.info="Use Case Manager Get"},
    { .verb = "ucmreset", .callback = alsaUseCaseReset, .info="Use Case Manager Reset"},
    { .verb = "ucmclose", .callback = alsaUseCaseClose, .info="Use Case Manager Close"},
    { .verb = "addcustomctl", .callback = alsaAddCustomCtls, .info="Add Software Alsa Custom Control"},
    { .verb = NULL} /* marker for end of the array */
};

/*
 * description of the binding for afb-daemon
 */
const afb_binding_v2 afbBindingV2 = {
    .api = "alsacore",
    .verbs = api_verbs,
    .preinit  = AlsaInit,
};
