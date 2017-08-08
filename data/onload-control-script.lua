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
function Adjust_Volume_Speed (speed_meters_second)

   message= string.format("****  Adjust_Volume_Speed speed=%d count=%d", speed_meters_second, count);
   print (message);

   -- compute volume
   volume = speed_meters_second * 2
   count=count+1

  return true, volume, count 
end

function Ping_Test(...)

    print ("Ping_Test script arguments:");

    for i,v in ipairs(arg)
    do
        print(" -- ", tostring(v))
    end
 

    -- return two arguments on top of status
    return true, 1234, "ABCD"

end


