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

  Note: this file should be called before any other to assert declare function
  is loaded before anything else.

  References:
    http://lua-users.org/wiki/DetectingUndefinedVariables
  
--]]

-- Set Navigation lower sound when play
function _Control_Init(source, args)

    -- in strict mode every variables should be declared
    local err=0

    AFB:notice ("Control_Init args=%s", args);
   
    return 0 -- control accepted
end

-- Set Navigation lower sound when play
function _Button_Press(source, args, query)

    -- in strict mode every variables should be declared
    local err=0

    AFB:notice ("Button_Press button=%s", args["button"]);
   
    return 0 -- control accepted
end

-- Set Navigation lower sound when play
function _Event_Received(source, args, query)

    -- in strict mode every variables should be declared
    local err=0

    AFB:notice ("Event_Received event=%s", args["evtname"]);
   
    return 0 -- control accepted
end

