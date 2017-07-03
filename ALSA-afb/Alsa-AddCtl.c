/*
 * AlsaUseCase -- provide low level interface with ALSA lib (extracted from alsa-json-gateway code)
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
 * 
 * References:
 *  https://kernel.readthedocs.io/en/sphinx-samples/writing-an-alsa-driver.html#control-names
 *  https://01.org/linuxgraphics/gfx-docs/drm/sound/designs/control-names.html

*/
#define _GNU_SOURCE  // needed for vasprintf

#include <alsa/asoundlib.h>
#include <systemd/sd-event.h>
#include "Alsa-ApiHat.h"

#include <sys/ioctl.h>

typedef struct _snd_ctl_ops {
        int (*close)(snd_ctl_t *handle);
        int (*nonblock)(snd_ctl_t *handle, int nonblock);
        int (*async)(snd_ctl_t *handle, int sig, pid_t pid);
        int (*subscribe_events)(snd_ctl_t *handle, int subscribe);
        int (*card_info)(snd_ctl_t *handle, snd_ctl_card_info_t *info);
        int (*element_list)(snd_ctl_t *handle, snd_ctl_elem_list_t *list);
        int (*element_info)(snd_ctl_t *handle, snd_ctl_elem_info_t *info);
        int (*element_add)(snd_ctl_t *handle, snd_ctl_elem_info_t *info);
        int (*element_replace)(snd_ctl_t *handle, snd_ctl_elem_info_t *info);
        int (*element_remove)(snd_ctl_t *handle, snd_ctl_elem_id_t *id);
        int (*element_read)(snd_ctl_t *handle, snd_ctl_elem_value_t *control);
        int (*element_write)(snd_ctl_t *handle, snd_ctl_elem_value_t *control);
        int (*element_lock)(snd_ctl_t *handle, snd_ctl_elem_id_t *lock);
        int (*element_unlock)(snd_ctl_t *handle, snd_ctl_elem_id_t *unlock);
        int (*element_tlv)(snd_ctl_t *handle, int op_flag, unsigned int numid,
                           unsigned int *tlv, unsigned int tlv_size);
        int (*hwdep_next_device)(snd_ctl_t *handle, int *device);
        int (*hwdep_info)(snd_ctl_t *handle, snd_hwdep_info_t * info);
        int (*pcm_next_device)(snd_ctl_t *handle, int *device);
        int (*pcm_info)(snd_ctl_t *handle, snd_pcm_info_t * info);
        int (*pcm_prefer_subdevice)(snd_ctl_t *handle, int subdev);
        int (*rawmidi_next_device)(snd_ctl_t *handle, int *device);
        int (*rawmidi_info)(snd_ctl_t *handle, snd_rawmidi_info_t * info);
        int (*rawmidi_prefer_subdevice)(snd_ctl_t *handle, int subdev);
        int (*set_power_state)(snd_ctl_t *handle, unsigned int state);
        int (*get_power_state)(snd_ctl_t *handle, unsigned int *state);
        int (*read)(snd_ctl_t *handle, snd_ctl_event_t *event);
        int (*poll_descriptors_count)(snd_ctl_t *handle);
        int (*poll_descriptors)(snd_ctl_t *handle, struct pollfd *pfds, unsigned int space);
        int (*poll_revents)(snd_ctl_t *handle, struct pollfd *pfds, unsigned int nfds, unsigned short *revents);
} snd_ctl_ops_t;

typedef struct private_snd_ctl {
        void *open_func;
        char *name;
        snd_ctl_type_t type;
        const snd_ctl_ops_t *ops;
        void *private_data;
        int nonblock;
        int poll_fd;
        void *async_handlers;
} private_snd_ctl_t;

typedef struct {
        int card;
        int fd;
        unsigned int protocol;
} snd_ctl_hw_t;

static int snd_ctl_hw_elem_replace(snd_ctl_t *ctlDev, snd_ctl_elem_info_t *info, snd_ctl_elem_id_t *elemId) {
        size_t len=snd_ctl_elem_info_sizeof();
        private_snd_ctl_t *handle= (private_snd_ctl_t*) ctlDev;
        snd_ctl_hw_t *hw = handle->private_data;
        
        NOTICE ("count=%d ITEMNAME=%s writable=%d owner=%d", snd_ctl_elem_info_get_count(info), snd_ctl_elem_info_get_name(info)
                 , snd_ctl_elem_info_is_writable(info), snd_ctl_elem_info_get_owner(info));
        snd_ctl_elem_lock(ctlDev, elemId);
        
        int err= handle->ops->element_replace (ctlDev, info);
        NOTICE ("count=%d ITEMNAME=%s writable=%d isowner=%d islocked=%d innactiv=%d", snd_ctl_elem_info_get_count(info), snd_ctl_elem_info_get_name(info)
                 , snd_ctl_elem_info_is_writable(info), snd_ctl_elem_info_is_owner(info), snd_ctl_elem_info_is_locked(info), snd_ctl_elem_info_is_inactive(info));
        return err;
}


STATIC int addOneSndCtl(afb_req request, snd_ctl_t  *ctlDev, json_object *ctlJ) {
    int err, ctlExist;
    json_object *jTmp;
    const char *ctlName;
    int  ctlNumid, ctlMax, ctlMin, ctlStep, ctlCount, ctlSubDev, ctlSndDev;
    snd_ctl_elem_type_t  ctlType;
    snd_ctl_elem_id_t    *elemId; 
    snd_ctl_elem_info_t  *elemInfo;
    snd_ctl_elem_value_t *elemValue;
    
    // parse json ctl object
    json_object_object_get_ex (ctlJ, "name" , &jTmp);
    if (!jTmp) {
        afb_req_fail_f (request, "ctl-invalid", "crl=%s name missing", json_object_get_string(ctlJ));
        goto OnErrorExit;
    }    
    ctlName  = json_object_get_string(jTmp);
    
    // default value when not present => 0
    json_object_object_get_ex (ctlJ, "numid" , &jTmp);
    ctlNumid = json_object_get_int(jTmp);
        
    // default for json_object_get_int is zero
    json_object_object_get_ex (ctlJ, "min" , &jTmp);
    ctlMin = json_object_get_int(jTmp);

    json_object_object_get_ex (ctlJ, "max" , &jTmp);
    if (!jTmp) ctlMax=1;
    else ctlMax = json_object_get_int(jTmp);
    
    json_object_object_get_ex (ctlJ, "step" , &jTmp);
    if (!jTmp) ctlStep=1;
    else ctlStep = json_object_get_int(jTmp);
    
    json_object_object_get_ex (ctlJ, "count" , &jTmp);
    if (!jTmp) ctlCount=2;
    else ctlCount = json_object_get_int(jTmp);

    json_object_object_get_ex (ctlJ, "snddev" , &jTmp);
    ctlSndDev = json_object_get_int(jTmp);
   
    json_object_object_get_ex (ctlJ, "subdev" , &jTmp);
    ctlSubDev = json_object_get_int(jTmp);
   
    json_object_object_get_ex (ctlJ, "type" , &jTmp);
    if (!jTmp) ctlType=SND_CTL_ELEM_TYPE_BOOLEAN;
    else ctlType = json_object_get_int(jTmp);
    
    // Add requested ID into elemInfo
    snd_ctl_elem_info_alloca(&elemInfo);
    snd_ctl_elem_id_alloca(&elemId);
    snd_ctl_elem_id_set_numid (elemId, ctlNumid);
    snd_ctl_elem_id_set_name (elemId, ctlName);
    snd_ctl_elem_id_set_interface(elemId, SND_CTL_ELEM_IFACE_HWDEP);
    snd_ctl_elem_id_set_device(elemId, ctlSndDev);
    snd_ctl_elem_id_set_subdevice(elemId, ctlSubDev);
    
    // Assert that this ctls is not used
    snd_ctl_elem_info_set_id (elemInfo, elemId);
    ctlExist= !snd_ctl_elem_info(ctlDev, elemInfo);
    if (ctlExist) {
        NOTICE ("1:ctl exist ctlName=%s NUMID=%d NAME=%s", ctlName, snd_ctl_elem_info_get_numid(elemInfo), snd_ctl_elem_info_get_name(elemInfo));
        snd_ctl_elem_id_set_name (elemId, ctlName);
        snd_ctl_elem_info_set_name (elemInfo, ctlName);
        err= snd_ctl_hw_elem_replace (ctlDev, elemInfo, elemId);
        if (err) {
            NOTICE ("Fail changing ctlname error=%s", snd_strerror(err));
        }
        NOTICE ("2:ctl exist ctlName=%s NUMID=%d NAME=%s", ctlName, snd_ctl_elem_info_get_numid(elemInfo), snd_ctl_elem_info_get_name(elemInfo));
    }    

    // try to normalise ctl name
    snd_ctl_elem_value_alloca(&elemValue);
    snd_ctl_elem_id_set_name (elemId, ctlName);
    snd_ctl_elem_value_set_id(elemValue, elemId);
        
    if (!ctlExist) switch (ctlType) {
        case SND_CTL_ELEM_TYPE_BOOLEAN:
            err = snd_ctl_add_boolean_elem_set(ctlDev, elemInfo, 1, ctlCount);
            if (err) {
                afb_req_fail_f (request, "ctl-invalid-bool", "devid=%s crl=%s invalid boolean data", snd_ctl_name(ctlDev), json_object_get_string(ctlJ));
                goto OnErrorExit;                
            }            

            // Provide FALSE as default value
            for (int idx=0; idx < ctlCount; idx ++) {
                snd_ctl_elem_value_set_boolean (elemValue, idx, 0);
            }            
            break;
            
        case SND_CTL_ELEM_TYPE_INTEGER:
            break;
            
        case SND_CTL_ELEM_TYPE_INTEGER64:
            break;
            
        case SND_CTL_ELEM_TYPE_ENUMERATED:
            break;
            
        case SND_CTL_ELEM_TYPE_BYTES:
            break;
            
        default:
            afb_req_fail_f (request, "ctl-invalid-type", "crl=%s invalid/unknown type", json_object_get_string(ctlJ));
            goto OnErrorExit;                
    }

    err =  snd_ctl_elem_write (ctlDev, elemValue);
    if (err < 0) {
        afb_req_fail_f (request, "ctl-write-fail", "crl=%s fail to write data data", json_object_get_string(ctlJ));
        goto OnErrorExit;                
    }            
    
   
    return 0;
    
    OnErrorExit:
        return -1;
}

PUBLIC void alsaAddCustomCtls(afb_req request) {
    int err;
    json_object *ctlsJ;
    enum json_type;
    snd_ctl_t  *ctlDev=NULL;
    const char *devid;

    devid = afb_req_value(request, "devid");
    if (devid == NULL) {
        afb_req_fail_f (request, "devid-missing", "devid MUST be defined for alsaAddCustomCtls");
        goto OnErrorExit;
    }
    
    // open control interface for devid
    err = snd_ctl_open(&ctlDev, devid, 0);
    if (err < 0) {
        afb_req_fail_f (request, "devid-unknown", "SndCard devid=[%s] Not Found err=%s", devid, snd_strerror(err));
        goto OnErrorExit;
    }
    
    // extract sound controls and parse json
    ctlsJ = json_tokener_parse (afb_req_value(request, "ctls"));
    if (!ctlsJ) {
        afb_req_fail_f (request, "ctls-missing", "ctls MUST be defined as a JSON array for alsaAddCustomCtls");
        goto OnErrorExit;
    }
     
    switch (json_object_get_type(ctlsJ)) { 
        case json_type_object:
             addOneSndCtl(request, ctlDev, ctlsJ);
             break;
        
        case json_type_array:
            for (int idx= 0; idx < json_object_array_length (ctlsJ); idx++) {
                json_object *ctlJ = json_object_array_get_idx (ctlsJ, idx);
                addOneSndCtl(request, ctlDev, ctlJ) ;
            }
            break;
            
        default:
            afb_req_fail_f (request, "ctls-invalid","ctls=%s not valid JSON array", json_object_get_string(ctlsJ));
            goto OnErrorExit;
    }
            
    OnErrorExit:
        if (ctlDev) snd_ctl_close(ctlDev);   
        return;
}