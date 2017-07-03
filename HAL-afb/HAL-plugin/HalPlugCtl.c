/*
 * Copyright (C) 2016 "IoT.bzh"
 * Author Fulup Ar Foll <fulup@iot.bzh>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 */


#include <stdio.h>
#include <sys/ioctl.h>
#include <alsa/asoundlib.h>
#include <alsa/control_external.h>
#include <linux/soundcard.h>

typedef struct afbHalPlug {
	snd_ctl_ext_t ext;
	char *device;
	unsigned int num_vol_ctls;
	unsigned int num_rec_items;
	unsigned int vol_ctl[SOUND_MIXER_NRDEVICES];
	unsigned int rec_item[SOUND_MIXER_NRDEVICES];
} afbHalPlug_t;

static const char *const vol_devices[SOUND_MIXER_NRDEVICES] = {
	[SOUND_MIXER_VOLUME]   = "Master Playback Volume",
	[SOUND_MIXER_BASS]     = "Tone Control - Bass",
	[SOUND_MIXER_TREBLE]   = "Tone Control - Treble",
	[SOUND_MIXER_SYNTH]    = "Synth Playback Volume",
	[SOUND_MIXER_PCM]      = "PCM Playback Volume",
	[SOUND_MIXER_SPEAKER]  = "PC Speaker Playback Volume",
	[SOUND_MIXER_LINE]     = "Line Playback Volume",
	[SOUND_MIXER_MIC]      = "Mic Playback Volume",
	[SOUND_MIXER_CD]       = "CD Playback Volume",
	[SOUND_MIXER_IMIX]     = "Monitor Mix Playback Volume",
	[SOUND_MIXER_ALTPCM]   = "Headphone Playback Volume",
	[SOUND_MIXER_RECLEV]   = "Capture Volume",
	[SOUND_MIXER_IGAIN]    = "Capture Volume",
	[SOUND_MIXER_OGAIN]    = "Playback Volume",
	[SOUND_MIXER_LINE1]    = "Aux Playback Volume",
	[SOUND_MIXER_LINE2]    = "Aux1 Playback Volume",
	[SOUND_MIXER_LINE3]    = "Line1 Playback Volume",
	[SOUND_MIXER_DIGITAL1] = "IEC958 Playback Volume",
	[SOUND_MIXER_DIGITAL2] = "Digital Playback Volume",
	[SOUND_MIXER_DIGITAL3] = "Digital1 Playback Volume",
	[SOUND_MIXER_PHONEIN]  = "Phone Playback Volume",
	[SOUND_MIXER_PHONEOUT] = "Master Mono Playback Volume",
	[SOUND_MIXER_VIDEO]    = "Video Playback Volume",
	[SOUND_MIXER_RADIO]    = "Radio Playback Volume",
	[SOUND_MIXER_MONITOR]  = "Monitor Playback Volume",
};

static const char *const rec_devices[SOUND_MIXER_NRDEVICES] = {
	[SOUND_MIXER_VOLUME]   = "Mix Capture Switch",
	[SOUND_MIXER_SYNTH]    = "Synth Capture Switch",
	[SOUND_MIXER_PCM]      = "PCM Capture Switch",
	[SOUND_MIXER_LINE]     = "Line Capture Switch",
	[SOUND_MIXER_MIC]      = "Mic Capture Switch",
	[SOUND_MIXER_CD]       = "CD Capture Switch",
	[SOUND_MIXER_LINE1]    = "Aux Capture Switch",
	[SOUND_MIXER_LINE2]    = "Aux1 Capture Switch",
	[SOUND_MIXER_LINE3]    = "Line1 Capture Switch",
	[SOUND_MIXER_DIGITAL1] = "IEC958 Capture Switch",
	[SOUND_MIXER_DIGITAL2] = "Digital Capture Switch",
	[SOUND_MIXER_DIGITAL3] = "Digital1 Capture Switch",
	[SOUND_MIXER_PHONEIN]  = "Phone Capture Switch",
	[SOUND_MIXER_VIDEO]    = "Video Capture Switch",
	[SOUND_MIXER_RADIO]    = "Radio Capture Switch",
};	

static const char *const rec_items[SOUND_MIXER_NRDEVICES] = {
	[SOUND_MIXER_VOLUME]   = "Mix",
	[SOUND_MIXER_SYNTH]    = "Synth",
	[SOUND_MIXER_PCM]      = "PCM",
	[SOUND_MIXER_LINE]     = "Line",
	[SOUND_MIXER_MIC]      = "Mic",
	[SOUND_MIXER_CD]       = "CD",
	[SOUND_MIXER_LINE1]    = "Aux",
	[SOUND_MIXER_LINE2]    = "Aux1",
	[SOUND_MIXER_LINE3]    = "Line1",
	[SOUND_MIXER_DIGITAL1] = "IEC958",
	[SOUND_MIXER_DIGITAL2] = "Digital",
	[SOUND_MIXER_DIGITAL3] = "Digital1",
	[SOUND_MIXER_PHONEIN]  = "Phone",
	[SOUND_MIXER_VIDEO]    = "Video",
	[SOUND_MIXER_RADIO]    = "Radio",
};	




static snd_ctl_ext_key_t AfbHalElemFind(snd_ctl_ext_t *ext,
				       const snd_ctl_elem_id_t *id) {
       
	snd_ctl_hal_t *ctlHal = ext->private_data;
	const char *name;
	unsigned int i, key, numid;

	numid = snd_ctl_elem_id_get_numid(id);
	if (numid > 0) {
		numid--;
		if (numid < afbHalPlug->num_vol_ctls)
			return afbHalPlug->vol_ctl[numid];
		numid -= afbHalPlug->num_vol_ctls;
		if (afbHalPlug->exclusive_input) {
			if (!numid)
				return OSS_KEY_CAPTURE_MUX;
		} else if (numid < afbHalPlug->num_rec_items)
			return afbHalPlug->rec_item[numid] |
				OSS_KEY_CAPTURE_FLAG;
	}

	name = snd_ctl_elem_id_get_name(id);
	if (! strcmp(name, "Capture Source")) {
		if (afbHalPlug->exclusive_input)
			return OSS_KEY_CAPTURE_MUX;
		else
			return SND_CTL_EXT_KEY_NOT_FOUND;
	}
	for (i = 0; i < afbHalPlug->num_vol_ctls; i++) {
		key = afbHalPlug->vol_ctl[i];
		if (! strcmp(name, vol_devices[key]))
			return key;
	}
	for (i = 0; i < afbHalPlug->num_rec_items; i++) {
		key = afbHalPlug->rec_item[i];
		if (! strcmp(name, rec_devices[key]))
			return key | OSS_KEY_CAPTURE_FLAG;
	}
	return SND_CTL_EXT_KEY_NOT_FOUND;
}

static int AfbHalGetAttrib(snd_ctl_ext_t *ext, snd_ctl_ext_key_t key,
			     int *type, unsigned int *acc, unsigned int *count) {
	return 0;
}

static int AfbHalGetInfo(snd_ctl_ext_t *ext ATTRIBUTE_UNUSED,
				snd_ctl_ext_key_t key ATTRIBUTE_UNUSED,
				long *imin, long *imax, long *istep) {

	return 0;
}

static int AfbHalGetEnumInfo(snd_ctl_ext_t *ext,
				   snd_ctl_ext_key_t key ATTRIBUTE_UNUSED,
				   unsigned int *items) {

	return 0;
}

static int AfbHalGetEnumName(snd_ctl_ext_t *ext,
				   snd_ctl_ext_key_t key ATTRIBUTE_UNUSED,
				   unsigned int item, char *name,
				   size_t name_max_len) {

	return 0;
}

static int AfbHalReadInt(snd_ctl_ext_t *ext, snd_ctl_ext_key_t key, long *value) {

	return 0;
}

static int AfbHalReadEnumerate(snd_ctl_ext_t *ext,
			       snd_ctl_ext_key_t key ATTRIBUTE_UNUSED,
			       unsigned int *items) {

	return 0;
}

static int AfbHalWriteInt(snd_ctl_ext_t *ext, snd_ctl_ext_key_t key, long *value) {

    return 0;
}

static int AfbHalWriteEnum(snd_ctl_ext_t *ext,
				snd_ctl_ext_key_t key ATTRIBUTE_UNUSED,
				unsigned int *items) {

    return 0;
}

static int AfbHalEventRead(snd_ctl_ext_t *ext ATTRIBUTE_UNUSED,
			  snd_ctl_elem_id_t *id ATTRIBUTE_UNUSED,
			  unsigned int *event_mask ATTRIBUTE_UNUSED)
{
	return -EAGAIN;
}

static snd_ctl_ext_callback_t afbHalCBs = {
	.close               = AfbHalClose,
	.elem_count          = AfbHalElemCount,
	.elem_list           = AfbHalElemList,
	.find_elem           = AfbHalElemFind,
	.get_attribute       = AfbHalGetAttrib,
	.get_integer_info    = AfbHalGetInfo,
	.get_enumerated_info = AfbHalGetEnumInfo,
	.get_enumerated_name = AfbHalGetEnumName,
	.read_integer        = AfbHalReadInt,
	.read_enumerated     = AfbHalReadEnumerate,
	.write_integer       = AfbHalWriteInt,
	.write_enumerated    = AfbHalWriteEnum,
	.read_event          = AfbHalEventRead,
};


SND_CTL_PLUGIN_DEFINE_FUNC(afb_hal) {

    snd_config_iterator_t it, next;
    afbHalPlug_t *afbHalPlug;
    int err;
	
	snd_config_for_each(it, next, conf) {
		snd_config_t *n = snd_config_iterator_entry(it);
		const char *id;
		if (snd_config_get_id(n, &id) < 0)
			continue;
		if (strcmp(id, "comment") == 0 || strcmp(id, "type") == 0 || strcmp(id, "hint") == 0)
			continue;
		if (strcmp(id, "slave") == 0) {
			if (snd_config_get_string(n, &device) < 0) {
				SNDERR("Invalid type for %s", id);
				return -EINVAL;
			}
			continue;
		}
		SNDERR("Unknown field %s", id);
		return -EINVAL;
	}

        // Create ALSA control plugin structure
	afbHalPlug->ext.version = SND_CTL_EXT_VERSION;
	afbHalPlug->ext.card_idx = 0; /* FIXME */
	strcpy(afbHalPlug->ext.id, "AFB-HAL-CTL");
	strcpy(afbHalPlug->ext.driver, "AFB-HAL");
	strcpy(afbHalPlug->ext.name, "AFB-HAL Control Plugin");
	strcpy(afbHalPlug->ext.mixername, "AFB-HAL Mixer Plugin");
	strcpy(afbHalPlug->ext.longname, "Automotive-Linux Sound Abstraction Control Plugin");
	afbHalPlug->ext.poll_fd      = -1;
	afbHalPlug->ext.callback     = &afbHalCBs;
	afbHalPlug->ext.private_data = afbHalPlug;


	err = snd_ctl_ext_create(&afbHalPlug->ext, name, mode);
	if (err < 0) goto OnErrorExit;

        // Plugin register controls update handlep before exiting
	*handlep = afbHalPlug->ext.handle;
	return 0;

OnErrorExit: 
	free(afbHalPlug);
	return -1;
}

SND_CTL_PLUGIN_SYMBOL(afb_hal);
