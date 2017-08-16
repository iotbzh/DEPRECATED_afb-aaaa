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
  
 
  Simple API script to be use with AGL control LuaDoCall API
   - After the script is loaded by lua_docall
   - Controller start function=xxxx where xxxx is taken from script filename doscript-xxxx-anything 

--]]

function helloworld (request, query) 

    AFB:notice ("LUA HelloWorld: Simple test  query=%s", query);

    if (query == nil) then
       AFB:notice ("LUA HelloWorld:FX query should not be empty");
       AFB:fail  (request, "LUA HelloWorld: query should not be empty");
    else
       AFB:notice  ("LUA HelloWorld:OK query=%s", query);
       AFB:success (request, {arg0="Demat", arg1="Bonjours", arg2="Gootentag", arg3="Morning"});
    end

end