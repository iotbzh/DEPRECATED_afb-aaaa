
static const char _afb_description_v2_polctl[] =
    "{\"openapi\":\"3.0.0\",\"$schema\":\"http:iot.bzh/download/openapi/schem"
    "a-3.0/default-schema.json\",\"info\":{\"description\":\"\",\"title\":\"p"
    "olctl\",\"version\":\"1.0\",\"x-binding-c-generator\":{\"api\":\"polctl\""
    ",\"version\":2,\"prefix\":\"polctl_\",\"postfix\":\"\",\"start\":null,\""
    "onevent\":null,\"init\":\"polctl_init\",\"scope\":\"\",\"private\":false"
    "}},\"servers\":[{\"url\":\"ws://{host}:{port}/api/polctl\",\"description"
    "\":\"Unicens2 API.\",\"variables\":{\"host\":{\"default\":\"localhost\"}"
    ",\"port\":{\"default\":\"1234\"}},\"x-afb-events\":[{\"$ref\":\"#/compon"
    "ents/schemas/afb-event\"}]}],\"components\":{\"schemas\":{\"afb-reply\":"
    "{\"$ref\":\"#/components/schemas/afb-reply-v2\"},\"afb-event\":{\"$ref\""
    ":\"#/components/schemas/afb-event-v2\"},\"afb-reply-v2\":{\"title\":\"Ge"
    "neric response.\",\"type\":\"object\",\"required\":[\"jtype\",\"request\""
    "],\"properties\":{\"jtype\":{\"type\":\"string\",\"const\":\"afb-reply\""
    "},\"request\":{\"type\":\"object\",\"required\":[\"status\"],\"propertie"
    "s\":{\"status\":{\"type\":\"string\"},\"info\":{\"type\":\"string\"},\"t"
    "oken\":{\"type\":\"string\"},\"uuid\":{\"type\":\"string\"},\"reqid\":{\""
    "type\":\"string\"}}},\"response\":{\"type\":\"object\"}}},\"afb-event-v2"
    "\":{\"type\":\"object\",\"required\":[\"jtype\",\"event\"],\"properties\""
    ":{\"jtype\":{\"type\":\"string\",\"const\":\"afb-event\"},\"event\":{\"t"
    "ype\":\"string\"},\"data\":{\"type\":\"object\"}}}},\"x-permissions\":{\""
    "monitor\":{\"permission\":\"urn:AGL:permission:audio:public:monitor\"},\""
    "multimedia\":{\"permission\":\"urn:AGL:permission:audio:public:monitor\""
    "},\"navigation\":{\"permission\":\"urn:AGL:permission:audio:public:monit"
    "or\"},\"emergency\":{\"permission\":\"urn:AGL:permission:audio:public:em"
    "ergency\"}},\"responses\":{\"200\":{\"description\":\"A complex object a"
    "rray response\",\"content\":{\"application/json\":{\"schema\":{\"$ref\":"
    "\"#/components/schemas/afb-reply\"}}}}}},\"paths\":{\"/monitor\":{\"desc"
    "ription\":\"Subcribe Audio Agent Policy Control End\",\"get\":{\"x-permi"
    "ssions\":{\"$ref\":\"#/components/x-permissions/monitor\"},\"parameters\""
    ":[{\"in\":\"query\",\"name\":\"event_patern\",\"required\":true,\"schema"
    "\":{\"type\":\"string\"}}],\"responses\":{\"200\":{\"$ref\":\"#/componen"
    "ts/responses/200\"}}}},\"/event_test\":{\"description\":\"Pause Resume T"
    "est\",\"get\":{\"x-permissions\":{\"$ref\":\"#/components/x-permissions/"
    "monitor\"},\"parameters\":[{\"in\":\"query\",\"name\":\"delay\",\"requir"
    "ed\":false,\"schema\":{\"type\":\"interger\"}},{\"in\":\"query\",\"name\""
    ":\"count\",\"required\":false,\"schema\":{\"type\":\"interger\"}}],\"res"
    "ponses\":{\"200\":{\"$ref\":\"#/components/responses/200\"}}}},\"/naviga"
    "tion\":{\"description\":\"Request Access to Navigation Audio Channel.\","
    "\"get\":{\"x-permissions\":{\"$ref\":\"#/components/x-permissions/naviga"
    "tion\"},\"parameters\":[{\"in\":\"query\",\"name\":\"zone\",\"required\""
    ":false,\"schema\":{\"type\":\"string\"}}],\"responses\":{\"200\":{\"$ref"
    "\":\"#/components/responses/200\"}}}}}}"
;

static const struct afb_auth _afb_auths_v2_polctl[] = {
	{ .type = afb_auth_Permission, .text = "urn:AGL:permission:audio:public:monitor" }
};

 void polctl_monitor(struct afb_req req);
 void polctl_event_test(struct afb_req req);
 void polctl_navigation(struct afb_req req);

static const struct afb_verb_v2 _afb_verbs_v2_polctl[] = {
    {
        .verb = "monitor",
        .callback = polctl_monitor,
        .auth = &_afb_auths_v2_polctl[0],
        .info = NULL,
        .session = AFB_SESSION_NONE_V2
    },
    {
        .verb = "event_test",
        .callback = polctl_event_test,
        .auth = &_afb_auths_v2_polctl[0],
        .info = NULL,
        .session = AFB_SESSION_NONE_V2
    },
    {
        .verb = "navigation",
        .callback = polctl_navigation,
        .auth = &_afb_auths_v2_polctl[0],
        .info = NULL,
        .session = AFB_SESSION_NONE_V2
    },
    { .verb = NULL }
};

const struct afb_binding_v2 afbBindingV2 = {
    .api = "polctl",
    .specification = _afb_description_v2_polctl,
    .info = NULL,
    .verbs = _afb_verbs_v2_polctl,
    .preinit = NULL,
    .init = polctl_init,
    .onevent = NULL,
    .noconcurrency = 0
};

