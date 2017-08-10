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

#include "Alsa-ApiHat.h"

/*
 * array of the verbs exported to afb-daemon
 */
static const struct afb_verb_v2 api_verbs[] = {
    /* VERB'S NAME          FUNCTION TO CALL  */
    { .verb = "ping", .callback = pingtest},
    { .verb = "getinfo", .callback = alsaGetInfo},
    { .verb = "getctl", .callback = alsaGetCtls},
    { .verb = "setctl", .callback = alsaSetCtls},
    { .verb = "subscribe", .callback = alsaEvtSubcribe},
    { .verb = "getcardid", .callback = alsaGetCardId},
    { .verb = "halregister", .callback = alsaRegisterHal},
    { .verb = "hallist", .callback = alsaActiveHal},
    { .verb = "ucmquery", .callback = alsaUseCaseQuery},
    { .verb = "ucmset", .callback = alsaUseCaseSet},
    { .verb = "ucmget", .callback = alsaUseCaseGet},
    { .verb = "ucmreset", .callback = alsaUseCaseReset},
    { .verb = "ucmclose", .callback = alsaUseCaseClose},
    { .verb = "addcustomctl", .callback = alsaAddCustomCtls},
    { .verb = NULL} /* marker for end of the array */
};

/*
 * description of the binding for afb-daemon
 */
const struct afb_binding_v2 afbBindingV2 = {
    .api = "alsacore",
    .verbs = api_verbs,
};
