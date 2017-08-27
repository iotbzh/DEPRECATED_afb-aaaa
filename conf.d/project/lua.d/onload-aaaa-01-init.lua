--[[
  Copyright (C) 2016 "IoT.bzh"
  Author Fulup Ar Foll <fulup@iot.bzh>

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
--]]

-- Global variable SHOULD start with _
_Global_Context={}

--[[
   This function is call during controller init phase as describe in onload-daemon-sample.json
   It receives two argument 1st one is the source (here on load) second one is the arguments
   as expose in config file.

   In this sample we create an event that take the name of args["zzzz"], the resulting handle
   is save into _Global_Context for further use.

   Note: init functions are not call from a client and thus do not receive query

--]]
function _Audio_Controller_Init(source, control)

    printf ("[--> Audio_Controller_Init -->] source=%d control=%s", source, Dump_Table(control))

    -- create an event from configuration name
    _Global_Context["event"]=AFB:evtmake(control["evtname"])

    -- query HAL to retrieve sound card.
    local err,result= AFB:servsync ("alsacore", "hallist", {})

    if (err) then
        AFB:error("Fail to retrieve Audio HAL")
    else 
        _Global_Context["registry"]=result["response"]
        printf("[<-- Audio_Controller_Init <--] Active HAL=%s", Dump_Table(result["response"]))
    end
end
