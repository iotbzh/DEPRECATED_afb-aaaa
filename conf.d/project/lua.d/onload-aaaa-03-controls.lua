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

  Following function are called when a control activate a label with
  labeller api -> APi=label VERB=dispatch
  arguments are
    - source (0) when requesting the label (-1) when releasing
    - label comme from config given with 'control' in onload-middlename-xxxxx.json
    - control is the argument part of the query as providing by control requesting the label.

--]]

_CurrentHalVolume={}



--[[ Apply control
 * source=1  permanent change requested
 * source=0  temporally request control
 * source=-1 temporally release control
-- ------------------------------------ --]]
local function Apply_Hal_Control(source, label, adjustment) 
 local HAL = _Global_Context["registry"]
 
    -- check we really got some data
    if (adjustment == nil) then
        AFB:error ("--* (Hoops) Control should provide volume adjustment")  
        return 1
    end

    -- loop on each HAL save current volume and push adjustment
    for key,hal in pairs(_Global_Context["registry"]) do
       printf ("--- HAL=%s", Dump_Table(hal))

        -- action set loop on active HAL and get current volume
        -- if label respond then do volume adjustment

        if (source >= 0) then

            -- get current volume for each HAL
            local err,result= AFB:servsync(hal["api"],"ctlget", {["label"]=label})

            -- if no error save current volume and set adjustment
            if (err ~= nil) then
                local response= result["response"]
                printf ("--- Response %s=%s", hal["api"], Dump_Table(response))

                if (response == nil) then
                   printf ("--- Fail to Activate '%s'='%s' result=%s", hal["api"], label, Dump_Table(result)) 
                   return 1 -- unhappy
                end

                -- save response in global space 
                _CurrentHalVolume [hal["api"]] = response

                -- finally set the new value
                local query= {
                    ["tag"]= response["tag"],
                    ["val"]= adjustment
                }

                -- best effort to set adjustment value
                AFB:servsync(hal["api"],"ctlset",query)
            end

        else  -- when label is release reverse action at preempt time
           
            if (_CurrentHalVolume [hal["api"]] ~= nil) then

                printf("--- Restoring initial volume HAL=%s Control=%s", hal["api"], _CurrentHalVolume [hal["api"]])

                AFB:servsync(hal["api"],"ctlset", _CurrentHalVolume [hal["api"]])
            end
        end    

    end
    return 0 -- happy end
end


-- Temporally adjust volume
function _Temporarily_Control(source, control, client)

    printf ("[--> _Temporarily_Control -->] source=%d control=%s client=%s", source, Dump_Table(control), Dump_Table(client))

    -- Init should have been properly done
    if (_Global_Context["registry"] == nil) then
       AFB:error ("--* (Hoops) No Hal in _Global_Context=%s", Dump_Table(_Global_Context)) 
       return 1
    end

    -- make sure label as valid
    if (control["ctl"] == nil or control["val"] == nil) then
        AFB:error ("--* Action Ignore no/invalid control=%s", Dump_Table(control)) 
        return 1 -- unhappy
    end

    if (source == 0) then
        AFB:info("-- Adjust %s=%d", control["ctl"], control["val"])
        local error=Apply_Hal_Control(source, control["ctl"], control["val"])
        if (error == nil) then 
            return 1 -- unhappy
        end
        AFB:notice ("[<-- _Temporarily_Control Granted<--]")
    else
        Apply_Hal_Control(source, control["ctl"],0)
        AFB:notice ("[<-- _Temporarily_Control Restore--]")
    end

    return 0 -- happy return
end


-- Permanent Adjust volume
function _Permanent_Control(source, control, client)

    printf ("[--> _Permanent_Control -->] source=%d control=%s client=%s", source, Dump_Table(control), Dump_Table(client))

    -- Init should have been properly done
    if (_Global_Context["registry"] == nil) then
       AFB:error ("--* (Hoops) No Hal in _Global_Context=%s", Dump_Table(_Global_Context)) 
       return 1
    end

    -- make sure label as valid
    if (control["ctl"] == nil or control["val"] == nil) then
        AFB:error ("--* Action Ignore no/invalid control=%s", Dump_Table(control)) 
        return 1 -- unhappy
    end

    if (source == 0) then
        AFB:info("-- Adjust %s=%d", control["ctl"], control["val"])
        local error=Apply_Hal_Control(1, control["ctl"], control["val"])
        if (error == nil) then 
            return 1 -- unhappy
        end
        AFB:notice ("[<-- _Permanent_Control Granted<--]")
    else
        Apply_Hal_Control(source, control["ctl"],0)
        AFB:notice ("[<-- _Permanent_Control Restore--]")
    end

    return 0 -- happy return
end

