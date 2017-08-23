/*
 * AlsaLibMapping -- provide low level interface with ALSA lib (extracted from alsa-json-gateway code)
 * Copyright (C) 2015,2016,2017, Fulup Ar Foll fulup@iot.bzh
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

#define _GNU_SOURCE  // needed for vasprintf

#include "Alsa-ApiHat.h"

// generic sndctrl event handle hook to event callback when pooling

typedef struct {
    struct pollfd pfds;
    sd_event_source *src;
    snd_ctl_t *ctlDev;
    int mode;
    struct afb_event afbevt;
} evtHandleT;

typedef struct {
    int ucount;
    int cardId;
    evtHandleT *evtHandle;
} sndHandleT;

typedef struct {
    char *devid;
    char *apiprefix;
    char *shortname;
    char *longname;
} cardRegistryT;

cardRegistryT *cardRegistry[MAX_SND_CARD + 1];

PUBLIC json_object *alsaCheckQuery(afb_req request, queryValuesT *queryValues) {

    json_object *tmpJ;
    int done;

    // get query from request
    json_object *queryInJ = afb_req_json(request);

    done = json_object_object_get_ex(queryInJ, "devid", &tmpJ);
    if (!done) {
        afb_req_fail_f(request, "devid-missing", "Invalid query='%s'", json_object_get_string(queryInJ));
        goto OnErrorExit;
    }
    queryValues->devid = json_object_get_string(tmpJ);

    done = json_object_object_get_ex(queryInJ, "mode", &tmpJ);
    if (!done) queryValues->mode = QUERY_QUIET; // default quiet
    else queryValues->mode = json_object_get_int(tmpJ);

    return queryInJ;

OnErrorExit:
    return NULL;
}

// This routine is called when ALSA event are fired

STATIC int sndCtlEventCB(sd_event_source* src, int fd, uint32_t revents, void* userData) {
    int err, ctlNumid;
    evtHandleT *evtHandle = (evtHandleT*) userData;
    snd_ctl_event_t *eventId;
    json_object *ctlEventJ;
    unsigned int mask;
    int iface;
    int device;
    int subdev;
    const char*ctlName;
    ctlRequestT ctlRequest;
    snd_ctl_elem_id_t *elemId;

    if ((revents & EPOLLHUP) != 0) {
        AFB_NOTICE("SndCtl hanghup [car disconnected]");
        goto ExitOnSucess;
    }

    if ((revents & EPOLLIN) != 0) {

        // initialise event structure on stack
        snd_ctl_event_alloca(&eventId);
        snd_ctl_elem_id_alloca(&elemId);

        err = snd_ctl_read(evtHandle->ctlDev, eventId);
        if (err < 0) goto OnErrorExit;

        // we only process sndctrl element
        if (snd_ctl_event_get_type(eventId) != SND_CTL_EVENT_ELEM) goto ExitOnSucess;

        // we only process value changed events
        mask = snd_ctl_event_elem_get_mask(eventId);
        if (!(mask & SND_CTL_EVENT_MASK_VALUE)) goto ExitOnSucess;

        snd_ctl_event_elem_get_id(eventId, elemId);

        err = alsaGetSingleCtl(evtHandle->ctlDev, elemId, &ctlRequest, evtHandle->mode);
        if (err) goto OnErrorExit;

        // If CTL as a value use it as container for response
        if (ctlRequest.valuesJ) ctlEventJ = ctlRequest.valuesJ;
        else {
            ctlEventJ = json_object_new_object();
            ctlNumid = snd_ctl_event_elem_get_numid(eventId);
            json_object_object_add(ctlEventJ, "id", json_object_new_int(ctlNumid));
        }

        if (evtHandle->mode >= QUERY_COMPACT) {
            ctlName = snd_ctl_event_elem_get_name(eventId);
            json_object_object_add(ctlEventJ, "name", json_object_new_string(ctlName));
        }

        if (evtHandle->mode >= QUERY_VERBOSE) {
            iface = snd_ctl_event_elem_get_interface(eventId);
            device = snd_ctl_event_elem_get_device(eventId);
            subdev = snd_ctl_event_elem_get_subdevice(eventId);
            json_object_object_add(ctlEventJ, "ifc", json_object_new_int(iface));
            json_object_object_add(ctlEventJ, "dev", json_object_new_int(device));
            json_object_object_add(ctlEventJ, "sub", json_object_new_int(subdev));
        }


        AFB_DEBUG("sndCtlEventCB=%s", json_object_get_string(ctlEventJ));
        afb_event_push(evtHandle->afbevt, ctlEventJ);
    }

ExitOnSucess:
    return 0;

OnErrorExit:
    AFB_WARNING("sndCtlEventCB: ignored unsupported event type");
    return (0);
}

// Subscribe to every Alsa CtlEvent send by a given board

PUBLIC void alsaEvtSubcribe(afb_req request) {
    static sndHandleT sndHandles[MAX_SND_CARD];
    evtHandleT *evtHandle = NULL;
    snd_ctl_t *ctlDev = NULL;
    int err, idx, cardId, idxFree = -1;
    snd_ctl_card_info_t *cardinfo;
    queryValuesT queryValues;

    json_object *queryJ = alsaCheckQuery(request, &queryValues);
    if (!queryJ) goto OnErrorExit;

    // open control interface for devid
    err = snd_ctl_open(&ctlDev, queryValues.devid, SND_CTL_READONLY);
    if (err < 0) {
        afb_req_fail_f(request, "devid-unknown", "SndCard devid=%s Not Found err=%s", queryValues.devid, snd_strerror(err));
        goto OnErrorExit;
    }

    snd_ctl_card_info_alloca(&cardinfo);
    if ((err = snd_ctl_card_info(ctlDev, cardinfo)) < 0) {
        afb_req_fail_f(request, "devid-invalid", "SndCard devid=%s Not Found err=%s", queryValues.devid, snd_strerror(err));
        goto OnErrorExit;
    }

    cardId = snd_ctl_card_info_get_card(cardinfo);

    // search for an existing subscription and mark 1st free slot
    for (idx = 0; idx < MAX_SND_CARD; idx++) {
        if (sndHandles[idx].ucount > 0 && cardId == sndHandles[idx].cardId) {
            evtHandle = sndHandles[idx].evtHandle;
            break;
        } else if (idxFree == -1) idxFree = idx;
    };

    // if not subscription exist for the event let's create one
    if (idx == MAX_SND_CARD) {

        // reach MAX_SND_CARD event registration
        if (idxFree == -1) {
            afb_req_fail_f(request, "register-toomany", "Cannot register new event Maxcard==%d", idx);
            goto OnErrorExit;
        }

        evtHandle = malloc(sizeof (evtHandleT));
        evtHandle->ctlDev = ctlDev;
        evtHandle->mode = queryValues.mode;
        sndHandles[idxFree].ucount = 0;
        sndHandles[idxFree].cardId = cardId;
        sndHandles[idxFree].evtHandle = evtHandle;

        // subscribe for sndctl events attached to devid
        err = snd_ctl_subscribe_events(evtHandle->ctlDev, 1);
        if (err < 0) {
            afb_req_fail_f(request, "subscribe-fail", "Cannot subscribe events from devid=%s err=%d", queryValues.devid, err);
            goto OnErrorExit;
        }

        // get pollfd attach to this sound board
        snd_ctl_poll_descriptors(evtHandle->ctlDev, &evtHandle->pfds, 1);

        // register sound event to binder main loop
        err = sd_event_add_io(afb_daemon_get_event_loop(), &evtHandle->src, evtHandle->pfds.fd, EPOLLIN, sndCtlEventCB, evtHandle);
        if (err < 0) {
            afb_req_fail_f(request, "register-mainloop", "Cannot hook events to mainloop devid=%s err=%d", queryValues.devid, err);
            goto OnErrorExit;
        }

        // create binder event attached to devid name
        evtHandle->afbevt = afb_daemon_make_event(queryValues.devid);
        if (!afb_event_is_valid(evtHandle->afbevt)) {
            afb_req_fail_f(request, "register-event", "Cannot register new binder event name=%s", queryValues.devid);
            goto OnErrorExit;
        }

        // everything looks OK let's move forward
        idx = idxFree;
    }

    // subscribe to binder event
    err = afb_req_subscribe(request, evtHandle->afbevt);
    if (err != 0) {
        afb_req_fail_f(request, "register-eventname", "Cannot subscribe binder event name=%s [invalid channel]", queryValues.devid);
        goto OnErrorExit;
    }

    // increase usage count and return success
    sndHandles[idx].ucount++;
    afb_req_success(request, NULL, NULL);
    return;

OnErrorExit:
    if (ctlDev) snd_ctl_close(ctlDev);
    return;
}

// Subscribe to every Alsa CtlEvent send by a given board

STATIC json_object *alsaProbeCardId(afb_req request) {
    char devid [10];
    const char *ctlName, *shortname, *longname;
    int card, err, index, idx;
    json_object *responseJ;
    snd_ctl_t *ctlDev;
    snd_ctl_card_info_t *cardinfo;

    const char *sndname = afb_req_value(request, "sndname");
    if (sndname == NULL) {
        afb_req_fail_f(request, "argument-missing", "sndname=SndCardName missing");
        goto OnErrorExit;
    }

    // loop on potential card number
    snd_ctl_card_info_alloca(&cardinfo);
    for (card = 0; card < MAX_SND_CARD; card++) {

        // build card devid and probe it
        snprintf(devid, sizeof (devid), "hw:%i", card);

        // open control interface for devid
        err = snd_ctl_open(&ctlDev, devid, SND_CTL_READONLY);
        if (err < 0) continue;

        // extract sound card information
        snd_ctl_card_info(ctlDev, cardinfo);
        index = snd_ctl_card_info_get_card(cardinfo);
        ctlName = snd_ctl_card_info_get_id(cardinfo);
        shortname = snd_ctl_card_info_get_name(cardinfo);
        longname = snd_ctl_card_info_get_longname(cardinfo);

        // check if short|long name match
        if (!strcmp(sndname, ctlName)) break;
        if (!strcmp(sndname, shortname)) break;
        if (!strcmp(sndname, longname)) break;
    }

    if (card == MAX_SND_CARD) {
        afb_req_fail_f(request, "ctlDev-notfound", "Fail to find card with name=%s", sndname);
        goto OnErrorExit;
    }

    // proxy ctlevent as a binder event
    responseJ = json_object_new_object();
    json_object_object_add(responseJ, "index", json_object_new_int(index));
    json_object_object_add(responseJ, "devid", json_object_new_string(devid));
    json_object_object_add(responseJ, "shortname", json_object_new_string(shortname));
    json_object_object_add(responseJ, "longname", json_object_new_string(longname));

    // search for a HAL binder card mapping name to api prefix
    for (idx = 0; (idx < MAX_SND_CARD && cardRegistry[idx]); idx++) {
        if (!strcmp(cardRegistry[idx]->shortname, shortname)) {
            json_object_object_add(responseJ, "halapi", json_object_new_string(cardRegistry[idx]->apiprefix));
            break;
        }
    }

    return responseJ;

OnErrorExit:
    return NULL;
}

// Make alsaProbeCardId compatible with AFB request

PUBLIC void alsaGetCardId(afb_req request) {

    json_object *responseJ = alsaProbeCardId(request);
    if (responseJ) afb_req_success(request, responseJ, NULL);
}

// Return list of active resgistrated HAL with corresponding sndcard

PUBLIC void alsaActiveHal(afb_req request) {
    json_object *responseJ = json_object_new_array();

    for (int idx = 0; idx < MAX_SND_CARD; idx++) {
        if (!cardRegistry[idx]) break;

        json_object *haldevJ = json_object_new_object();
        json_object_object_add(haldevJ, "api", json_object_new_string(cardRegistry[idx]->apiprefix));
        if (cardRegistry[idx]->devid) json_object_object_add(haldevJ, "devid", json_object_new_string(cardRegistry[idx]->devid));
        if (cardRegistry[idx]->shortname)json_object_object_add(haldevJ, "shortname", json_object_new_string(cardRegistry[idx]->shortname));
        if (cardRegistry[idx]->longname) json_object_object_add(haldevJ, "longname", json_object_new_string(cardRegistry[idx]->longname));
        json_object_array_add(responseJ, haldevJ);
    }

    afb_req_success(request, responseJ, NULL);
}


// Register loaded HAL with board Name and API prefix

PUBLIC void alsaRegisterHal(afb_req request) {
    static int index = 0;
    json_object *responseJ;
    const char *shortname, *apiPrefix;

    apiPrefix = afb_req_value(request, "prefix");
    if (apiPrefix == NULL) {
        afb_req_fail_f(request, "argument-missing", "prefix=BindingApiPrefix missing");
        goto OnErrorExit;
    }

    shortname = afb_req_value(request, "sndname");
    if (shortname == NULL) {
        afb_req_fail_f(request, "argument-missing", "sndname=SndCardName missing");
        goto OnErrorExit;
    }

    if (index == MAX_SND_CARD) {
        afb_req_fail_f(request, "alsahal-toomany", "Fail to register sndname=[%s]", shortname);
        goto OnErrorExit;
    }

    // alsaGetCardId should be check to register only valid card
    responseJ = alsaProbeCardId(request);
    if (responseJ) {
        json_object *tmpJ;
        int done;

        cardRegistry[index] = malloc(sizeof (cardRegistry));
        cardRegistry[index]->apiprefix = strdup(apiPrefix);
        cardRegistry[index]->shortname = strdup(shortname);

        done = json_object_object_get_ex(responseJ, "devid", &tmpJ);
        if (done) cardRegistry[index]->devid = strdup(json_object_get_string(tmpJ));
        else cardRegistry[index]->devid = NULL;

        done = json_object_object_get_ex(responseJ, "longname", &tmpJ);
        if (done) cardRegistry[index]->longname = strdup(json_object_get_string(tmpJ));
        else cardRegistry[index]->longname = NULL;

        // make sure register close with a null value
        index++;
        cardRegistry[index] = NULL;

        afb_req_success(request, responseJ, NULL);
    }

    // If OK return sound card Alsa ID+Info
    return;

OnErrorExit:
    return;
}

