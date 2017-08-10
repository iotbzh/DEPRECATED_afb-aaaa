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

count=0

-- Adjust Volume function of vehicle speed
function Adjust_Volume_Speed (request, speed_meters_second)

  AFB:notice("In Adjust_Volume_Speed speed=%d", speed_meters_second);

   print (string.format("*****(From Lua) Adjust_Volume_Speed speed=%d count=%d", speed_meters_second, count));

   -- compute volume
   volume = speed_meters_second * 2
   count=count+1

  AFB:success (request, 1234, volume, count, 5678) 
end


function Test_Binder_CB (result, context) 

   local myTable= { ["arg1"] = "myString", ["arg2"] = 1234, ["arg4"] = true, ["arg5"] = 3.1416 }

   AFB:notice ("In Test_Binder_CB", result, context)

   AFB:success (1234, "ABCD", myTable, 5678)

end

function Test_Binder_Call_Async () 

   local query= {
      ["arg1"] = "myString",
      ["arg2"] = 1234,
      ["arg4"] = true,
      ["arg5"] = 3.1416,
   }

   AFB:service("alsacore","ping", query, Test_Binder_CB, "myContext")

end

function Test_Binder_Call_Sync () 

    local query= {
      ["arg1"] = "myString",
      ["arg2"] = 1234,
      ["arg4"] = true,
      ["arg5"] = 3.1416,
    }

   err= AFB:service_sync ("alsacore","ping", query)

    if (err) then
        AFB:fail ("AFB:service_call_sync fail");
    else
        AFB:success (1234, "ABCD", myTable)
    end


end

function Ping_Test(...)

    print ("Ping_Test script arguments:");

    for i,v in ipairs(arg)
    do
        print(" -- ", tostring(v))
    end

    -- push response to client
    AFB:success (true, 1234, "ABCD"); 

end


