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
  
 
  Provide sample policy function for AGL Advance Audio Agent
--]]

function Audio_Init_CB (status, result, context)
    print ("--inlua:Audio_Init_CB-- result=", Dump_Table(result))
    print ("--inlua:Audio_Init_CB-- context=", Dump_Table(context))

  
    -- AFB:notice ("Audio_Init_Hal result='%s' context='%s'", result, context)
    -- AFB:debug ("Audio_Init_Hal result=%s context=%s", {["ret1"]=5678, ["ret2"]="abcd"}, context)
   
end

-- Function call at binding load time
function Audio_Init_Hal(args, query)

   local nextT = {
    ["next1"]=1234,
    ["next2"]="nested",
    ["next3"]=9999,
   }

   local response = {
    ["arg1"]=1234,
    ["arg2"]=nextT,
    ["arg3"]=5678,
   }

   print ("--inlua:Audio_Init-- response=", Dump_Table(response))

    AFB:notice ("**** in-lua table='%s' ****", response)


    AFB:notice ("--LUA:Audio_Init_Hal args=%s query=%s", args, query);
    
    -- query asynchronously loaded HAL
    AFB:service ('alsacore', 'hallist', {}, "Audio_Init_CB", {arg1=1234, arg2="toto"})
 
end

function Audio_Set_Navigation(args, query)

    AFB:notice ("LUA:Audio_Set_Use_Case args=%s query=%s", args, query);

    -- synchronous call to alsacore service
    local error,data= AFB:callsync ('alsacore', 'ping', {})  
    if (error) then
      AFB:error ("LUA:Audio_Set_Use_Case FAIL args=%s", args)
    else 
      AFB:notice ("--LUA:Audio_Set_Use_Case DONE args=%s response=%s", args, data["response"])
    end

    -- return OK
    return 0
end


