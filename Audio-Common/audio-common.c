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
 */
#define _GNU_SOURCE  // needed for vasprintf
#include "audio-common.h"

PUBLIC const char *halCtlsLabels[] = {
   
    [Master_Playback_Volume] = "Master_Playback_Volume",
    [Master_OnOff_Switch] = "Master_OnOff_Switch",
    [Master_Playback_Ramp]= "Master_Playback_Ramp",
    [PCM_Playback_Volume] = "PCM_Playback_Volume",
    [PCM_Playback_Switch] = "PCM_Playback_Switch",
    [Capture_Volume]      = "Capture_Volume",

    [Vol_Ramp_Set_Mode]  = "Volume_Ramp_Mode",        
    [Vol_Ramp_Set_Delay] = "Volume_Ramp_Delay",        
    [Vol_Ramp_Set_Down]  = "Volume_Ramp_Down",        
    [Vol_Ramp_Set_Up]    = "Volume_Ramp_Up",        
    [Vol_Ramp_Set_Slave] = "Volume_Ramp_Slave",        
       
    // Application Virtual Channels
    [Multimedia_Playback_Volume] = "Multimedia_Playback_Volume",       
    [Navigation_Playback_Volume] = "Navigation_Playback_Volume",
    [Emergency_Playback_Volume]  = "Emergency_Playback_Volume",   

    // Do not remove EndHalCrlTag 
    [EndHalCrlTag] = NULL
};

PUBLIC const char *halVolRampModes[] = {
   
   [RAMP_VOL_NONE]      = "None",
   [RAMP_VOL_NORMAL]    = "Normal",
   [RAMP_VOL_SMOOTH]    = "Smooth",
   [RAMP_VOL_EMERGENCY] = "Emergency",
   
   [EndHalVolMod] = NULL,
   
};

PUBLIC void pingtest(struct afb_req request) {
    json_object *query = afb_req_json(request);
    afb_req_success(request, query, NULL);
}
