
static const char _afb_description_v2_control[] =
    "{\"openapi\":\"3.0.0\",\"$schema\":\"http:iot.bzh/download/openapi/schem"
    "a-3.0/default-schema.json\",\"info\":{\"description\":\"\",\"title\":\"p"
    "olctl\",\"version\":\"1.0\",\"x-binding-c-generator\":{\"api\":\"control"
    "\",\"version\":2,\"prefix\":\"ctlapi_\",\"postfix\":\"\",\"start\":null,"
    "\"onevent\":null,\"init\":\"CtlBindingInit\",\"scope\":\"\",\"private\":"
    "false}},\"servers\":[{\"url\":\"ws://{host}:{port}/api/polctl\",\"descri"
    "ption\":\"Unicens2 API.\",\"variables\":{\"host\":{\"default\":\"localho"
    "st\"},\"port\":{\"default\":\"1234\"}},\"x-afb-events\":[{\"$ref\":\"#/c"
    "omponents/schemas/afb-event\"}]}],\"components\":{\"schemas\":{\"afb-rep"
    "ly\":{\"$ref\":\"#/components/schemas/afb-reply-v2\"},\"afb-event\":{\"$"
    "ref\":\"#/components/schemas/afb-event-v2\"},\"afb-reply-v2\":{\"title\""
    ":\"Generic response.\",\"type\":\"object\",\"required\":[\"jtype\",\"req"
    "uest\"],\"properties\":{\"jtype\":{\"type\":\"string\",\"const\":\"afb-r"
    "eply\"},\"request\":{\"type\":\"object\",\"required\":[\"status\"],\"pro"
    "perties\":{\"status\":{\"type\":\"string\"},\"info\":{\"type\":\"string\""
    "},\"token\":{\"type\":\"string\"},\"uuid\":{\"type\":\"string\"},\"reqid"
    "\":{\"type\":\"string\"}}},\"response\":{\"type\":\"object\"}}},\"afb-ev"
    "ent-v2\":{\"type\":\"object\",\"required\":[\"jtype\",\"event\"],\"prope"
    "rties\":{\"jtype\":{\"type\":\"string\",\"const\":\"afb-event\"},\"event"
    "\":{\"type\":\"string\"},\"data\":{\"type\":\"object\"}}}},\"x-permissio"
    "ns\":{\"monitor\":{\"permission\":\"urn:AGL:permission:audio:public:moni"
    "tor\"},\"multimedia\":{\"permission\":\"urn:AGL:permission:audio:public:"
    "monitor\"},\"navigation\":{\"permission\":\"urn:AGL:permission:audio:pub"
    "lic:monitor\"},\"emergency\":{\"permission\":\"urn:AGL:permission:audio:"
    "public:emergency\"}},\"responses\":{\"200\":{\"description\":\"A complex"
    " object array response\",\"content\":{\"application/json\":{\"schema\":{"
    "\"$ref\":\"#/components/schemas/afb-reply\"}}}}}},\"paths\":{\"/monitor\""
    ":{\"description\":\"Subcribe Audio Agent Policy Control End\",\"get\":{\""
    "x-permissions\":{\"$ref\":\"#/components/x-permissions/monitor\"},\"para"
    "meters\":[{\"in\":\"query\",\"name\":\"event_patern\",\"required\":true,"
    "\"schema\":{\"type\":\"string\"}}],\"responses\":{\"200\":{\"$ref\":\"#/"
    "components/responses/200\"}}}},\"/event_test\":{\"description\":\"Pause "
    "Resume Test\",\"get\":{\"x-permissions\":{\"$ref\":\"#/components/x-perm"
    "issions/monitor\"},\"parameters\":[{\"in\":\"query\",\"name\":\"delay\","
    "\"required\":false,\"schema\":{\"type\":\"interger\"}},{\"in\":\"query\""
    ",\"name\":\"count\",\"required\":false,\"schema\":{\"type\":\"interger\""
    "}}],\"responses\":{\"200\":{\"$ref\":\"#/components/responses/200\"}}}},"
    "\"/navigation\":{\"description\":\"Request Access to Navigation Audio Ch"
    "annel.\",\"get\":{\"x-permissions\":{\"$ref\":\"#/components/x-permissio"
    "ns/navigation\"},\"parameters\":[{\"in\":\"query\",\"name\":\"zone\",\"r"
    "equired\":false,\"schema\":{\"type\":\"string\"}}],\"responses\":{\"200\""
    ":{\"$ref\":\"#/components/responses/200\"}}}},\"/lua_docall\":{\"descrip"
    "tion\":\"Execute LUA string script.\",\"get\":{\"x-permissions\":{\"$ref"
    "\":\"#/components/x-permissions/navigation\"},\"parameters\":[{\"in\":\""
    "query\",\"name\":\"func\",\"required\":true,\"schema\":{\"type\":\"strin"
    "g\"}},{\"in\":\"query\",\"name\":\"args\",\"required\":false,\"schema\":"
    "{\"type\":\"array\"}}],\"responses\":{\"200\":{\"$ref\":\"#/components/r"
    "esponses/200\"}}}},\"/lua_dostring\":{\"description\":\"Execute LUA stri"
    "ng script.\",\"get\":{\"x-permissions\":{\"$ref\":\"#/components/x-permi"
    "ssions/navigation\"},\"parameters\":[{\"in\":\"query\",\"required\":true"
    ",\"schema\":{\"type\":\"string\"}}],\"responses\":{\"200\":{\"$ref\":\"#"
    "/components/responses/200\"}}}},\"/lua_doscript\":{\"description\":\"Exe"
    "cute LUA string script.\",\"get\":{\"x-permissions\":{\"$ref\":\"#/compo"
    "nents/x-permissions/navigation\"},\"parameters\":[{\"in\":\"query\",\"na"
    "me\":\"filename\",\"required\":true,\"schema\":{\"type\":\"string\"}}],\""
    "responses\":{\"200\":{\"$ref\":\"#/components/responses/200\"}}}}}}"
;

static const struct afb_auth _afb_auths_v2_control[] = {
	{ .type = afb_auth_Permission, .text = "urn:AGL:permission:audio:public:monitor" }
};

 void ctlapi_monitor(struct afb_req req);
 void ctlapi_event_test(struct afb_req req);
 void ctlapi_navigation(struct afb_req req);
 void ctlapi_lua_docall(struct afb_req req);
 void ctlapi_lua_dostring(struct afb_req req);
 void ctlapi_lua_doscript(struct afb_req req);

static const struct afb_verb_v2 _afb_verbs_v2_control[] = {
    {
        .verb = "monitor",
        .callback = ctlapi_monitor,
        .auth = &_afb_auths_v2_control[0],
        .info = NULL,
        .session = AFB_SESSION_NONE_V2
    },
    {
        .verb = "event_test",
        .callback = ctlapi_event_test,
        .auth = &_afb_auths_v2_control[0],
        .info = NULL,
        .session = AFB_SESSION_NONE_V2
    },
    {
        .verb = "navigation",
        .callback = ctlapi_navigation,
        .auth = &_afb_auths_v2_control[0],
        .info = NULL,
        .session = AFB_SESSION_NONE_V2
    },
    {
        .verb = "lua_docall",
        .callback = ctlapi_lua_docall,
        .auth = &_afb_auths_v2_control[0],
        .info = NULL,
        .session = AFB_SESSION_NONE_V2
    },
    {
        .verb = "lua_dostring",
        .callback = ctlapi_lua_dostring,
        .auth = &_afb_auths_v2_control[0],
        .info = NULL,
        .session = AFB_SESSION_NONE_V2
    },
    {
        .verb = "lua_doscript",
        .callback = ctlapi_lua_doscript,
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
    .onevent = NULL,
    .noconcurrency = 0
};

