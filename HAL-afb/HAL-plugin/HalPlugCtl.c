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
 * Testing:
 *  1) Copy generated plugin [libasound_module_pcm_afbhal.so] in alsa-lib/ dir visible from LD_LIBRARY_PATH (eg: /usr/lib64/alsa-lib)
 *  2) Create a ~/.asounrc file base on following template
 *      ctl.agl_hal {
 *      type  afbhal
 *      devid "hw:4"
 *      cblib "afbhal_cb_sample.so"
 *      ctls {
 *          # ctlLabel {numid integer name "Alsa Ctl Name"}
 *          MasterSwitch { numid 4 name "My_First_Control" }
 *          MasterVol    { numid 5 name "My_Second_Control" }
 *          CB_sample    { ctlcb @AfbHalSampleCB name "My_Sample_Callback"} 
 *      }
 *      pcm.agl_hal {
 *          type copy     # Copy PCM
 *          slave "hw:4"  # Slave name
 *      }
 *
 *  }
 *  3) Test with
 *     - amixer -Dagl_hal controls # should list all your controls
 *     - amixer -Dagl_hal cget numid=1
 *     - amixer -Dagl_hal cset numid=1 '10,20'
 */


#include <stdio.h>
#include <sys/ioctl.h>
#include "HalPlug.h"
#include <dlfcn.h>


static snd_ctl_ext_key_t AfbHalElemFind(snd_ctl_ext_t *ext, const snd_ctl_elem_id_t *id) {
    snd_ctl_hal_t *plughandle = (snd_ctl_hal_t*) ext->private_data;
    snd_ctl_ext_key_t key=NULL;
    
    int numid = snd_ctl_elem_id_get_numid(id);
    if (numid > 0) {
        if (numid > plughandle->ctlsCount) goto OnErrorExit;
        key= (snd_ctl_ext_key_t) numid -1;
        goto SucessExit;
    }
    
    const char *ctlName= snd_ctl_elem_id_get_name(id);
    if (ctlName == NULL)  goto OnErrorExit;
    
    for (int idx=0; idx < plughandle->ctlsCount; idx++) {
        if (! strcmp(ctlName, plughandle->ctls[idx].ctlName)) {
            key = idx;
            goto SucessExit;
        }
    }
    
    SucessExit:
    return key;
    
    OnErrorExit:
      return -1;
}

static int AfbHalGetAttrib(snd_ctl_ext_t *ext, snd_ctl_ext_key_t key, int *type, unsigned int *acc, unsigned int *count) {
    snd_ctl_hal_t *plughandle = (snd_ctl_hal_t*) ext->private_data;
    snd_ctl_elem_info_t  *elemInfo = plughandle->infos[key];
    snd_ctl_cb_t callback = plughandle->cbs[key];   

    
    // search for equivalent NUMID in effective sound card
    if (elemInfo) {
            *type  = snd_ctl_elem_info_get_type(elemInfo);
            *count = snd_ctl_elem_info_get_count(elemInfo);
            *acc   = SND_CTL_EXT_ACCESS_READWRITE; // Future ToBeDone
            return 0;
    }

    if (callback) {
        snd_ctl_get_attrib_t item;
        
        int err = callback(plughandle, CTLCB_GET_ATTRIBUTE, key, &item);
        if (!err) {
            *type = item.type;
            *acc  = item.acc;
            *count= item.count;
        }
        return err;
    }
    
    return -1;
}

static int AfbHalGetIntInfo(snd_ctl_ext_t *ext,	snd_ctl_ext_key_t key, long *imin, long *imax, long *istep) {
    snd_ctl_hal_t *plughandle = (snd_ctl_hal_t*) ext->private_data;
    snd_ctl_elem_info_t  *elemInfo = plughandle->infos[key];
    snd_ctl_cb_t callback = plughandle->cbs[key];   
    
    if (elemInfo) {
    
        // Should be normalised to make everything 0-100% 
        *imin = (long)snd_ctl_elem_info_get_min(elemInfo);
        *imax = (long)snd_ctl_elem_info_get_min(elemInfo);
        *istep= (long)snd_ctl_elem_info_get_min(elemInfo);
        return 0;
    } 
    
    if (callback) {
        snd_ctl_get_int_info_t item;
        
        int err = callback(plughandle, CTLCB_GET_INTEGER_INFO, key, &item);
        if (!err) {
            *imin = item.imin;
            *imax = item.imax;
            *istep= item.istep;
        }
        return err;
    }
    
    return -1;
}

static int AfbHalGetEnumInfo(snd_ctl_ext_t *ext, snd_ctl_ext_key_t key, unsigned int *items) {
    snd_ctl_hal_t *plughandle = (snd_ctl_hal_t*) ext->private_data;
    snd_ctl_elem_info_t  *elemInfo = plughandle->infos[key];
    snd_ctl_cb_t callback = plughandle->cbs[key];
    
    if(elemInfo) *items= snd_ctl_elem_info_get_items(elemInfo);
    if(callback) callback(plughandle, CTLCB_GET_ENUMERATED_INFO, key, items);
        
    return 0;
}

static int AfbHalGetEnumName(snd_ctl_ext_t *ext, snd_ctl_ext_key_t key, unsigned int item, char *name, size_t name_max_len) {
    snd_ctl_hal_t *plughandle = (snd_ctl_hal_t*) ext->private_data;
    snd_ctl_elem_info_t  *elemInfo = plughandle->infos[key];
    
    //name= snd_ctl_elem_info_get_item_name(elemInfo);
    return 0;
}

static int AfbHalReadInt(snd_ctl_ext_t *ext, snd_ctl_ext_key_t key, long *value) {
    snd_ctl_hal_t *plughandle = (snd_ctl_hal_t*) ext->private_data;
    snd_ctl_elem_info_t  *elemInfo = plughandle->infos[key];

    return 0;
}

static int AfbHalReadEnumerate(snd_ctl_ext_t *ext, snd_ctl_ext_key_t key, unsigned int *items) {
    snd_ctl_hal_t *plughandle = (snd_ctl_hal_t*) ext->private_data;

    return 0;
}

static int AfbHalWriteInt(snd_ctl_ext_t *ext, snd_ctl_ext_key_t key, long *value) {
    snd_ctl_hal_t *plughandle = (snd_ctl_hal_t*) ext->private_data;

    return 0;
}

static int AfbHalWriteEnum(snd_ctl_ext_t *ext, snd_ctl_ext_key_t key, unsigned int *items) {
    snd_ctl_hal_t *plughandle = (snd_ctl_hal_t*) ext->private_data;

    return 0;
}

static int AfbHalEventRead(snd_ctl_ext_t *ext, snd_ctl_elem_id_t *id, unsigned int *event_mask) {
    snd_ctl_hal_t *plughandle = (snd_ctl_hal_t*) ext->private_data;
 
    return -EAGAIN;
}

static int AfbHalElemList(snd_ctl_ext_t *ext, unsigned int offset, snd_ctl_elem_id_t *id) {
    snd_ctl_hal_t *plughandle = (snd_ctl_hal_t*) ext->private_data;

    snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
    snd_ctl_elem_id_set_name(id, plughandle->ctls[offset].ctlName);
    
    return 0;
}

static int AfbHalElemCount(snd_ctl_ext_t *ext) {
    snd_ctl_hal_t *plughandle = (snd_ctl_hal_t*) ext->private_data;
    int count = plughandle->ctlsCount;
    return count;
}

static void AfbHalClose(snd_ctl_ext_t *ext) {
	snd_ctl_hal_t *plughandle = (snd_ctl_hal_t*) ext->private_data;
        int err;

        for (int idx=0; idx < plughandle->ctlsCount; idx++) {
            if (plughandle->ctls[idx].ctlName) free((void*)plughandle->ctls[idx].ctlName);
        }

        err = snd_ctl_close(plughandle->ctlDev);
        if (err) SNDERR("Fail Close sndctl: devid=%s err=%s", plughandle->devid, snd_strerror(err)); 
	
	if (plughandle->devid) free(plughandle->devid);
	free(plughandle);
}

static snd_ctl_ext_callback_t afbHalCBs = {
	.close               = AfbHalClose,
	.elem_count          = AfbHalElemCount,
	.elem_list           = AfbHalElemList,
	.find_elem           = AfbHalElemFind,
	.get_attribute       = AfbHalGetAttrib,
	.get_integer_info    = AfbHalGetIntInfo,
	.get_enumerated_info = AfbHalGetEnumInfo,
	.get_enumerated_name = AfbHalGetEnumName,
	.read_integer        = AfbHalReadInt,
	.read_enumerated     = AfbHalReadEnumerate,
	.write_integer       = AfbHalWriteInt,
	.write_enumerated    = AfbHalWriteEnum,
	.read_event          = AfbHalEventRead,
};

SND_CTL_PLUGIN_DEFINE_FUNC(afbhal) {

    snd_config_iterator_t it, next;
    snd_ctl_hal_t *plughandle;
    int err;
    snd_ctl_cb_t AfbHalInitCB;
    const char *libname;
       
    plughandle = calloc(1, sizeof(snd_ctl_hal_t));
   
    snd_config_for_each(it, next, conf) {
        snd_config_t *node = snd_config_iterator_entry(it);
        const char *id;

        // ignore comment en empty lines
        if (snd_config_get_id(node, &id) < 0) continue;
        if (strcmp(id, "comment") == 0 || strcmp(id, "type") == 0 || strcmp(id, "hint") == 0) continue;
       
        // devid should point onto a valid sound card
        if (strcmp(id, "devid") == 0) {
            const char *devid;
            if (snd_config_get_string(node, &devid) < 0) {
                SNDERR("Invalid string type for %s", id);
                return -EINVAL;
            }
            plughandle->devid=strdup(devid);
                            
            // open control interface for devid
            err = snd_ctl_open(&plughandle->ctlDev, plughandle->devid, 0);
            if (err < 0) {
                SNDERR("Fail to open control device for devid=%s", plughandle->devid);
                return -EINVAL;
            }
            continue;
        }
        
        if (strcmp(id, "cblib") == 0) {
            if (snd_config_get_string(node, &libname) < 0) {
                SNDERR("Invalid libname string for %s", id);
                return -EINVAL;
            }
            
            plughandle->dlHandle= dlopen(libname, RTLD_NOW);
            if (!plughandle->dlHandle) {
                SNDERR("Fail to open callback sharelib=%s error=%s", libname, dlerror());
                return -EINVAL;                
            }
            
	    AfbHalInitCB = dlsym(plughandle->dlHandle, "AfbHalInitCB");
            if (!AfbHalInitCB) {
                SNDERR("Fail find 'AfbHalInitCB' symbol into callbacks sharelib=%s", libname);
                return -EINVAL;
            }
            
            err = (*AfbHalInitCB)(plughandle,CTLCB_INIT, 0,0);
            if (err) {
                SNDERR("Fail AfbHalInitCB err=%d", err);
                return -EINVAL;
            }
            
            continue;
        }
        
        if (strcmp(id, "ctl") == 0) {
            const char *ctlConf;
            snd_config_type_t ctype;
            snd_config_iterator_t currentCtl, follow;
            snd_config_t *itemConf;
            
            ctype = snd_config_get_type (node);
            if (ctype != SND_CONFIG_TYPE_COMPOUND) {
               snd_config_get_string (node, &ctlConf); 
               SNDERR("Invalid compound type for %s", node);
               return -EINVAL;
            }
            
            // loop on each ctl within ctls
            snd_config_for_each (currentCtl, follow, node) {
                snd_config_t *ctlconfig = snd_config_iterator_entry(currentCtl);
                snd_ctl_elem_info_t  *elemInfo;
                const char *ctlLabel, *ctlName;
                
                // ignore empty line
                if (snd_config_get_id(ctlconfig, &ctlLabel) < 0) continue;
                
                // each clt should be a valid config compound
                ctype = snd_config_get_type (ctlconfig);
                if (ctype != SND_CONFIG_TYPE_COMPOUND) {
                   snd_config_get_string (node, &ctlConf); 
                   SNDERR("Invalid ctl config for %s", ctlLabel);
                   return -EINVAL;
                }
                                
                err=snd_config_search(ctlconfig, "numid", &itemConf);
                if (!err) {
                    if (snd_config_get_integer(itemConf, (long*)&plughandle->ctls[plughandle->ctlsCount].ctlNumid) < 0) {
                    SNDERR("Not Integer: ctl:%s numid should be a valid integer", ctlLabel);
                    return -EINVAL;
                    }

                    // Make sure than numid is valid on slave snd card
                    snd_ctl_elem_info_malloc(&elemInfo);
                    snd_ctl_elem_info_set_numid(elemInfo, (int)plughandle->ctls[plughandle->ctlsCount].ctlNumid);
                    plughandle->infos[plughandle->ctlsCount]= elemInfo;

                    err = snd_ctl_elem_info(plughandle->ctlDev, elemInfo);
                    if (err) {    
                        SNDERR("Not Found: 'numid=%d' for 'devid=%s'", plughandle->ctls[plughandle->ctlsCount].ctlNumid, plughandle->devid);
                        return -EINVAL;                                        
                    }
                } 
                
                err=snd_config_search(ctlconfig, "ctlcb", &itemConf);
                if (!err) {
                    const char *funcname;
                    void *funcaddr;
                    
                    if (snd_config_get_string(itemConf, &funcname) < 0) {
                        SNDERR("Not string: ctl:%s cbname should be a valid string", ctlLabel);
                        return -EINVAL;
                    }
                    
                    if (funcname[0] != '@') {
                        SNDERR("Not string: ctl:%s cbname=%s should be prefixed with '@' ", ctlLabel, funcname);
                        return -EINVAL;
                    }
                    
                    if (!plughandle->dlHandle) {
                        SNDERR("No CB: ctl:%s cblib:/my/libcallback missing from asoundrc", ctlLabel);
                        return -EINVAL;
                    }
                    
              	    funcaddr = dlsym(plughandle->dlHandle, &funcname[1]);
                    if (!funcaddr) {
                        SNDERR("NotFound CB: ctl:%s cbname='%s' no symbol into %s", ctlLabel, &funcname[1], libname);
                        return -EINVAL;
                    }
                    plughandle->cbs[plughandle->ctlsCount]=funcaddr;
                }
                    
                err=snd_config_search(ctlconfig, "name", &itemConf);
                if (err) {
                    SNDERR("Not Found: 'name' mandatory in ctl config");
                    return -EINVAL;                    
                }
                
                if (snd_config_get_string(itemConf, &ctlName) < 0) {
                    SNDERR("Not String: ctl:%s 'name' should be a valie string", ctlLabel);
                    return -EINVAL;
                }
                plughandle->ctls[plughandle->ctlsCount].ctlName = strdup(ctlName);
                
                // move to next ctl if any
                plughandle->ctlsCount++;            
            } // end for each ctl
            continue;
        }
        SNDERR("Unknown field %s", id);
        return -EINVAL;
    }        

    
    
    // Create ALSA control plugin structure
    plughandle->ext.version         = SND_CTL_EXT_VERSION;
    plughandle->ext.card_idx        = 0; /* FIXME */
    strcpy(plughandle->ext.id       , "AFB-HAL-CTL");
    strcpy(plughandle->ext.driver   , "AFB-HAL");
    strcpy(plughandle->ext.name     , "AFB-HAL Control Plugin");
    strcpy(plughandle->ext.mixername, "AFB-HAL Mixer Plugin");
    strcpy(plughandle->ext.longname , "Automotive-Linux Sound Abstraction Control Plugin");
    plughandle->ext.poll_fd         = -1;
    plughandle->ext.callback        = &afbHalCBs;
    plughandle->ext.private_data    = (void*)plughandle;



    err = snd_ctl_ext_create(&plughandle->ext, name, mode);
    if (err < 0) {
        SNDERR("Fail Register sndctl for devid=%s", plughandle->devid);
        goto OnErrorExit;
    }

    // Plugin register controls update handlep before exiting
    *handlep = plughandle->ext.handle;
    return 0;

OnErrorExit: 
	free(plughandle);
	return -1;
}

SND_CTL_PLUGIN_SYMBOL(afbhal);
