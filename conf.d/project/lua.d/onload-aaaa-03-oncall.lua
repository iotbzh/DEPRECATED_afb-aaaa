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
  
 
  Provide sample LUA routines to be used with AGL control "lua_docall" API
--]]

--global counter to keep track of calls
_count=0

-- Display receive arguments and echo them to caller
function _Simple_Echo_Args (request, args)
    _count=_count+1
    AFB:notice("LUA OnCall Echo Args count=%d args=%s", count, args)

    print ("--inlua-- args=", Dump_Table(args))

    local response={
       ["count"]=_count,
       ["args"]=args,
    }

    -- fulup Embdeded table ToeDone AFB:success (request, response) 
    AFB:success (request,  {["func"]="Simple_Echo_Args", ["ret1"]=5678, ["ret2"]="abcd"}) 
end

local function Test_Async_CB (request, result, context)
   response={
     ["result"]=result,
     ["context"]=context,
   }

   AFB:notice ("Test_Async_CB result=%s context=%s", result, context)
   AFB:success (request, response)
end

function _Test_Call_Async (request, args) 
   local context={
     ["value1"]="abcd",
     ["value2"]=1234
   }

   AFB:notice ("Test_Call_Async args=%s cb=Test_Async_CB", args)
   AFB:service("alsacore","ping", "Test_Async_CB", context)
end

function _Test_Call_Sync (request, args) 

    AFB:notice ("Test_Call_Sync args=%s", args)
    local err, response= AFB:servsync ("alsacore","ping", args)
    if (err) then
        AFB:fail ("AFB:service_call_sync fail");
    else
        AFB:success (request, response)
    end
end

