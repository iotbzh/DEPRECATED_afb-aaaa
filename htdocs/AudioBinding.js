    var afb = new AFB("api", "mysecret");
    var ws;
    var halapi="HALNotSelected";
    var evtidx=0;

    function getParameterByName(name, url) {
        if (!url) {
          url = window.location.href;
        }
        name = name.replace(/[\[\]]/g, "\\$&");
        var regex = new RegExp("[?&]" + name + "(=([^&#]*)|&|#|$)"),
            results = regex.exec(url);
        if (!results) return null;
        if (!results[2]) return '';
        return decodeURIComponent(results[2].replace(/\+/g, " "));
    }
    
    // default soundcard is "PCH"
    var devid=getParameterByName("devid");
    if (!devid) devid="hw:1";
    
    var haldev=getParameterByName("haldev");
    if (!haldev) haldev="scarlett-usb";
    
    var sndname=getParameterByName("sndname");
    if (!sndname) sndname="PCH";
    
    var quiet=getParameterByName("quiet");
    if (!quiet) quiet="99";


    
    function init(elemid, api, verb, query) {
        
        function onopen() {
                // check for active HALs
                probeActiveHal (elemid, api, verb, query);
                
                document.getElementById("main").style.visibility = "visible";
                document.getElementById("connected").innerHTML = "Binder WS Active";
                document.getElementById("connected").style.background  = "lightgreen";
                ws.onevent("*", gotevent);
        }

        function onabort() {
                document.getElementById("main").style.visibility = "hidden";
                document.getElementById("connected").innerHTML = "Connected Closed";
                document.getElementById("connected").style.background  = "red";

        }
            
        ws = new afb.ws(onopen, onabort);
    }

    function replyok(obj) {
            console.log("replyok:" + JSON.stringify(obj));
            document.getElementById("output").innerHTML = "OK: "+JSON.stringify(obj);
    }
    
    function replyerr(obj) {
            console.log("replyerr:" + JSON.stringify(obj));
            document.getElementById("output").innerHTML = "ERROR: "+JSON.stringify(obj);
    }
    
    function gotevent(obj) {
            console.log("gotevent:" + JSON.stringify(obj));
            document.getElementById("outevt").innerHTML = (evtidx++) +": "+JSON.stringify(obj);
    }
    
    function send(message) {
            var api = document.getElementById("api").value;
            var verb = document.getElementById("verb").value;
            document.getElementById("question").innerHTML = "subscribe: "+api+"/"+verb + " (" + JSON.stringify(message) +")";
            ws.call(api+"/"+verb, {data:message}).then(replyok, replyerr);
    }
    

     // On button click from HTML page    
    function callbinder(api, verb, query) {
            console.log ("subscribe api="+api+" verb="+verb+" query=" +query);
            document.getElementById("question").innerHTML = "apicall: " + api+"/"+verb +" ("+ JSON.stringify(query)+")";
            ws.call(api+"/"+verb, query).then(replyok, replyerr);
    }


    // Retrieve the list of active HAL
    function probeActiveHal (elemid, api, verb, query) {
        var selectobj = document.getElementById(elemid);
        
        // onlick update selected HAL api
        selectobj.onclick=function(){
           halapi= this.value; 
           console.log ("New Default HAL=" + halapi);           
        };

        function gotit (result) {
            
           // display response as for normal onclick action
           replyok(result);
           var response=result.response;
           
           // fulfill select with avaliable active HAL
           for (idx=0; idx<response.length; idx++) {
               console.log ("probeActiveHal =" + response[idx].shortname);
               var opt = document.createElement('option');
               opt.value = response[idx].api;
               opt.text  = response[idx].shortname;
               selectobj.appendChild(opt);
           }
               
           halapi= selectobj.value;
        }

        // request lowlevel ALSA to get API list
        ws.call(api+"/"+verb, query).then(gotit, replyerr);
    }
