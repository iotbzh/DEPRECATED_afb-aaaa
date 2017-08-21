
static const char _afb_description_v2_control[] =
    "{\"openapi\":\"3.0.0\",\"$schema\":\"http:iot.bzh/download/openapi/schem"
    "a-3.0/default-schema.json\",\"info\":{\"description\":\"\",\"title\":\"c"
    "ontroler\",\"version\":\"1.0\",\"x-binding-c-generator\":{\"api\":\"cont"
    "rol\",\"version\":2,\"prefix\":\"ctlapi_\",\"postfix\":\"\",\"start\":nu"
    "ll,\"onevent\":\"DispatchOneEvent\",\"init\":\"CtlBindingInit\",\"scope\""
    ":\"\",\"private\":false}},\"servers\":[{\"url\":\"ws://{host}:{port}/api"
    "/polctl\",\"description\":\"Unicens2 API.\",\"variables\":{\"host\":{\"d"
    "efault\":\"localhost\"},\"port\":{\"default\":\"1234\"}},\"x-afb-events\""
    ":[{\"$ref\":\"#/components/schemas/afb-event\"}]}],\"components\":{\"sch"
    "emas\":{\"afb-reply\":{\"$ref\":\"#/components/schemas/afb-reply-v2\"},\""
    "afb-event\":{\"$ref\":\"#/components/schemas/afb-event-v2\"},\"afb-reply"
    "-v2\":{\"title\":\"Generic response.\",\"type\":\"object\",\"required\":"
    "[\"jtype\",\"request\"],\"properties\":{\"jtype\":{\"type\":\"string\",\""
    "const\":\"afb-reply\"},\"request\":{\"type\":\"object\",\"required\":[\""
    "status\"],\"properties\":{\"status\":{\"type\":\"string\"},\"info\":{\"t"
    "ype\":\"string\"},\"token\":{\"type\":\"string\"},\"uuid\":{\"type\":\"s"
    "tring\"},\"reqid\":{\"type\":\"string\"}}},\"response\":{\"type\":\"obje"
    "ct\"}}},\"afb-event-v2\":{\"type\":\"object\",\"required\":[\"jtype\",\""
    "event\"],\"properties\":{\"jtype\":{\"type\":\"string\",\"const\":\"afb-"
    "event\"},\"event\":{\"type\":\"string\"},\"data\":{\"type\":\"object\"}}"
    "}},\"x-permissions\":{\"monitor\":{\"permission\":\"urn:AGL:permission:a"
    "udio:public:monitor\"},\"multimedia\":{\"permission\":\"urn:AGL:permissi"
    "on:audio:public:monitor\"},\"navigation\":{\"permission\":\"urn:AGL:perm"
    "ission:audio:public:monitor\"},\"emergency\":{\"permission\":\"urn:AGL:p"
    "ermission:audio:public:emergency\"}},\"responses\":{\"200\":{\"descripti"
    "on\":\"A complex object array response\",\"content\":{\"application/json"
    "\":{\"schema\":{\"$ref\":\"#/components/schemas/afb-reply\"}}}}}},\"path"
    "s\":{\"/monitor\":{\"description\":\"Subcribe Audio Agent Policy Control"
    " End\",\"get\":{\"x-permissions\":{\"$ref\":\"#/components/x-permissions"
    "/monitor\"},\"parameters\":[{\"in\":\"query\",\"name\":\"event_patern\","
    "\"required\":true,\"schema\":{\"type\":\"string\"}}],\"responses\":{\"20"
    "0\":{\"$ref\":\"#/components/responses/200\"}}}},\"/dispatch\":{\"descri"
    "ption\":\"Request Access to Navigation Audio Channel.\",\"get\":{\"x-per"
    "missions\":{\"$ref\":\"#/components/x-permissions/navigation\"},\"parame"
    "ters\":[{\"in\":\"query\",\"name\":\"zone\",\"required\":false,\"schema\""
    ":{\"type\":\"string\"}}],\"responses\":{\"200\":{\"$ref\":\"#/components"
    "/responses/200\"}}}},\"/request\":{\"description\":\"Execute LUA string "
    "script.\",\"get\":{\"x-permissions\":{\"$ref\":\"#/components/x-permissi"
    "ons/navigation\"},\"parameters\":[{\"in\":\"query\",\"name\":\"func\",\""
    "required\":true,\"schema\":{\"type\":\"string\"}},{\"in\":\"query\",\"na"
    "me\":\"args\",\"required\":false,\"schema\":{\"type\":\"array\"}}],\"res"
    "ponses\":{\"200\":{\"$ref\":\"#/components/responses/200\"}}}},\"/execlu"
    "a\":{\"description\":\"Execute LUA string script.\",\"get\":{\"x-permiss"
    "ions\":{\"$ref\":\"#/components/x-permissions/navigation\"},\"parameters"
    "\":[{\"in\":\"query\",\"required\":true,\"schema\":{\"type\":\"string\"}"
    "}],\"responses\":{\"200\":{\"$ref\":\"#/components/responses/200\"}}}},\""
    "/scriptlua\":{\"description\":\"Execute LUA string script.\",\"get\":{\""
    "x-permissions\":{\"$ref\":\"#/components/x-permissions/navigation\"},\"p"
    "arameters\":[{\"in\":\"query\",\"name\":\"filename\",\"required\":true,\""
    "schema\":{\"type\":\"string\"}}],\"responses\":{\"200\":{\"$ref\":\"#/co"
    "mponents/responses/200\"}}}}}}"
;

static const struct afb_auth _afb_auths_v2_control[] = {
	{ .type = afb_auth_Permission, .text = "urn:AGL:permission:audio:public:monitor" }
};

 void ctlapi_monitor(struct afb_req req);
 void ctlapi_dispatch(struct afb_req req);
 void ctlapi_request(struct afb_req req);
 void ctlapi_execlua(struct afb_req req);
 void ctlapi_scriptlua(struct afb_req req);

static const struct afb_verb_v2 _afb_verbs_v2_control[] = {
    {
        .verb = "monitor",
        .callback = ctlapi_monitor,
        .auth = &_afb_auths_v2_control[0],
        .info = NULL,
        .session = AFB_SESSION_NONE_V2
    },
    {
        .verb = "dispatch",
        .callback = ctlapi_dispatch,
        .auth = &_afb_auths_v2_control[0],
        .info = NULL,
        .session = AFB_SESSION_NONE_V2
    },
    {
        .verb = "request",
        .callback = ctlapi_request,
        .auth = &_afb_auths_v2_control[0],
        .info = NULL,
        .session = AFB_SESSION_NONE_V2
    },
    {
        .verb = "execlua",
        .callback = ctlapi_execlua,
        .auth = &_afb_auths_v2_control[0],
        .info = NULL,
        .session = AFB_SESSION_NONE_V2
    },
    {
        .verb = "scriptlua",
        .callback = ctlapi_scriptlua,
        .auth = &_afb_auths_v2_control[0],
        .info = NULL,
        .session = AFB_SESSION_NONE_V2
    },
    { .verb = NULL }
};

const struct afb_binding_v2 afbBindingV2 = {
    .api = "control",
    .specification = _afb_description_v2_control,
    .info = NULL,
    .verbs = _afb_verbs_v2_control,
    .preinit = NULL,
    .init = CtlBindingInit,
    .onevent = DispatchOneEvent,
    .noconcurrency = 0
};

