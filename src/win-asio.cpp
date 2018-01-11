/*  Copyright (c) 2022 pkv <pkv@obsproject.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 */

#include "asio-loader.hpp"
OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("win-asio", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "ASIO audio input source";
}

ASIOAudioIODeviceList *list;

static const char *asio_input_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("AsioInput");
}

/* This creates the device if it hasn't been created by this or another source
  or retrieves its pointer if it already exists. The source is also added as
  a client of the device. */
static void attach_device(void *vptr, obs_data_t *settings)
{
	struct asio_data *data = (struct asio_data *)vptr;
	std::string name(obs_data_get_string(settings, "device_id"));
	for (int i = 0; i < list->deviceNames.size(); i++) {
		if (list->deviceNames[i] == name) {
			data->asio_device = list->attachDevice(name);
			if (!data->asio_device) {
				error("Failed to create device %s", name.c_str());
			} else {
				data->device_index = i;
				// increment the device client list if the source was never a client
				// & source ptr added as a client of asio device.
				//if (data->asio_client_index[i] < 0) {
				data->asio_client_index[i] = (int)data->asio_device->obs_clients.size();
				data->asio_device->obs_clients.push_back(data);
				data->asio_device->current_nb_clients++;
				//}
			}
			break;
		}
	}
}

static void detach_device(void *vptr, std::string name)
{
	struct asio_data *data = (struct asio_data *)vptr;
	int prev_dev_idx = list->getIndexFromDeviceName(name);
	int prev_client_idx = data->asio_client_index[prev_dev_idx];
	data->asio_device->obs_clients[prev_client_idx] = nullptr;
	data->asio_device->current_nb_clients--;
	if (data->asio_device->current_nb_clients == 0)
		data->asio_device->close();
}

static void asio_update(void *vptr, obs_data_t *settings)
{
	struct asio_data *data = (struct asio_data *)vptr;
	std::string err;
	bool swapping_device = false;
	const char *new_device = obs_data_get_string(settings, "device_id");
	std::string name(new_device);

	if (!new_device)
		return;

	// update the device data if we've swapped to a new one
	if (!data->device && new_device)
		data->device = bstrdup(new_device);

	if (!data->asio_device)
		attach_device(data, settings);
	else if (strcmp(data->asio_device->getName().c_str(), new_device) != 0) {
		detach_device(data, data->asio_device->getName());
		attach_device(data, settings);
		swapping_device = true;
	}

	ASIOAudioIODevice *asio_device = data->asio_device;
	if (!asio_device)
		return;
	if (!asio_device->isOpen())
		err = asio_device->open(asio_device->getCurrentSampleRate(), asio_device->getDefaultBufferSize());

	// update the routing
	for (int i = 0; i < data->out_channels; i++) {
		std::string route_str = "route " + std::to_string(i);
		if (data->route[i] != (int)obs_data_get_int(settings, route_str.c_str())) {
			data->route[i] = (int)obs_data_get_int(settings, route_str.c_str());
		}
	}
}

static void *asio_input_create(obs_data_t *settings, obs_source_t *source)
{
	struct asio_data *data = (struct asio_data *)bzalloc(sizeof(struct asio_data));
	data->source = source;
	data->asio_device = nullptr;
	for (int i = 0; i < maxNumASIODevices; i++)
		data->asio_client_index[i] = -1; // not a client if negative;
	data->out_channels = get_obs_output_channels();
	data->stopping = false;
	for (int i = 0; i < MAX_AUDIO_CHANNELS; i++) {
		data->route[i] = -1;
	}

	asio_update(data, settings);
	return data;
}

static void remove_client(void *vptr)
{
	struct asio_data *data = (struct asio_data *)vptr;
	if (data->asio_device) {
		data->stopping = true;
		data->asio_device = nullptr;
	}
}

static void asio_destroy(void *vptr)
{
	struct asio_data *data = (struct asio_data *)vptr;

	if (!data)
		return;
	/* delete the asio source from clients of asio device */
	if (data->device)
		bfree((void *)data->device);
	remove_client(data);

	bfree(data);
}

static bool fill_out_channels_modified(void *vptr, obs_properties_t *props, obs_property_t *chanlist,
				       obs_data_t *settings)
{
	UNUSED_PARAMETER(props);
	struct asio_data *data = (struct asio_data *)vptr;
	std::string name(obs_data_get_string(settings, "device_id"));

	obs_property_list_clear(chanlist);
	obs_property_list_add_int(chanlist, obs_module_text("Mute"), -1);
	if (!data->asio_device)
		return true;
	if (data->asio_client_index[data->device_index] >= 0) {
		std::vector<std::string> in_names = data->asio_device->getInputChannelNames();
		int input_channels = (int)in_names.size();
		for (int i = 0; i < input_channels; i++)
			obs_property_list_add_int(chanlist, in_names[i].c_str(), i);
		// store the number of input channels for the device
		data->in_channels = input_channels;
	}

	return true;
}

static bool asio_device_changed(void *vptr, obs_properties_t *props, obs_property_t *devlist, obs_data_t *settings)
{
	struct asio_data *data = (struct asio_data *)vptr;
	int i;
	int output_channels = data->out_channels;
	int max_channels = MAX_AUDIO_CHANNELS;
	const char *curDeviceId = obs_data_get_string(settings, "device_id");
	obs_property_t *panel = obs_properties_get(props, "ctrl");
	std::vector<obs_property_t *> route(max_channels, nullptr);

	int itemCount = (int)obs_property_list_item_count(devlist);
	bool itemFound = false;

	for (i = 0; i < itemCount; i++) {
		const char *DeviceId = obs_property_list_item_string(devlist, i);
		if (strcmp(DeviceId, curDeviceId) == 0) {
			itemFound = true;
			break;
		}
	}

	if (!itemFound) {
		obs_property_list_insert_string(devlist, 0, " ", curDeviceId);
		obs_property_list_item_disable(devlist, 0, true);
	} else {
		/* update the channel names */
		for (i = 0; i < max_channels; i++) {
			std::string name = "route " + std::to_string(i);
			route[i] = obs_properties_get(props, name.c_str());
			obs_property_list_clear(route[i]);
			obs_property_set_modified_callback2(route[i], fill_out_channels_modified, data);
			fill_out_channels_modified(data, props, route[i], settings);
			obs_property_set_visible(route[i], i < output_channels);
		}
	}

	if (data->asio_device) {
		obs_property_set_visible(panel, data->asio_device->hasControlPanel());
	}

	return true;
}

static bool asio_layout_changed(void *vptr, obs_properties_t *props, obs_property_t *list, obs_data_t *settings)
{
	UNUSED_PARAMETER(list);
	struct asio_data *data = (struct asio_data *)vptr;
	int max_channels = MAX_AUDIO_CHANNELS;
	speaker_layout layout = (speaker_layout)obs_data_get_int(settings, "speaker_layout");
	int recorded_channels = get_audio_channels(layout);
	int i = 0;
	for (i = 0; i < max_channels; i++) {
		std::string name = "route " + std::to_string(i);
		obs_property_t *r = obs_properties_get(props, name.c_str());
		obs_property_list_clear(r);
		obs_property_set_modified_callback2(r, fill_out_channels_modified, data);
		fill_out_channels_modified(data, props, r, settings);
		obs_property_set_visible(r, i < recorded_channels);
	}
	return true;
}

static bool show_panel(obs_properties_t *props, obs_property_t *property, void *vptr)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(property);

	if (!vptr)
		return false;
	struct asio_data *data = (struct asio_data *)vptr;
	ASIOAudioIODevice *device = data->asio_device;
	if (device && device->hasControlPanel())
		device->showControlPanel();
	return false;
}

static std::vector<speaker_layout> known_layouts = {
	SPEAKERS_MONO,    /**< Channels: MONO */
	SPEAKERS_STEREO,  /**< Channels: FL, FR */
	SPEAKERS_2POINT1, /**< Channels: FL, FR, LFE */
	SPEAKERS_4POINT0, /**< Channels: FL, FR, FC, RC */
	SPEAKERS_4POINT1, /**< Channels: FL, FR, FC, LFE, RC */
	SPEAKERS_5POINT1, /**< Channels: FL, FR, FC, LFE, RL, RR */
	SPEAKERS_7POINT1, /**< Channels: FL, FR, FC, LFE, RL, RR, SL, SR */
};

static std::vector<std::string> known_layouts_str = {"Mono", "Stereo", "2.1", "4.0", "4.1", "5.1", "7.1"};

static obs_properties_t *asio_input_properties(void *vptr)
{
	struct asio_data *data = (struct asio_data *)vptr;
	obs_properties_t *props = obs_properties_create();
	obs_property_t *devices;
	obs_property_t *format;
	obs_property_t *panel;
	int max_channels = MAX_AUDIO_CHANNELS;
	std::vector<obs_property_t *> route(max_channels, nullptr);

	props = obs_properties_create();

	devices = obs_properties_add_list(props, "device_id", obs_module_text("Device"), OBS_COMBO_TYPE_LIST,
					  OBS_COMBO_FORMAT_STRING);
	obs_property_set_modified_callback2(devices, asio_device_changed, data);

	/* list of asio devices */
	std::vector<std::string> DeviceNames = list->deviceNames;
	for (int i = 0; i < DeviceNames.size(); i++) {
		obs_property_list_add_string(devices, DeviceNames[i].c_str(), DeviceNames[i].c_str());
	}
	obs_property_set_long_description(devices, obs_module_text("ASIO Devices"));

	/* setting up the speaker layout on input */
	format = obs_properties_add_list(props, "speaker_layout", obs_module_text("Format"), OBS_COMBO_TYPE_LIST,
					 OBS_COMBO_FORMAT_INT);
	for (size_t i = 0; i < known_layouts.size(); i++)
		obs_property_list_add_int(format, known_layouts_str[i].c_str(), known_layouts[i]);
	obs_property_set_modified_callback2(format, asio_layout_changed, data);

	/* generating the list of obs channels */
	for (size_t i = 0; i < max_channels; i++) {
		route[i] = obs_properties_add_list(props, ("route " + std::to_string(i)).c_str(),
						   obs_module_text(("Route." + std::to_string(i)).c_str()),
						   OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
		obs_property_set_long_description(route[i],
						  obs_module_text(("Route.Desc." + std::to_string(i)).c_str()));
		if (i >= get_obs_output_channels())
			obs_property_set_visible(route[i], false);
	}

	panel = obs_properties_add_button2(props, "ctrl", obs_module_text("Control Panel"), show_panel, vptr);

	return props;
}

static void asio_defaults(obs_data_t *settings)
{
	struct obs_audio_info aoi;
	obs_get_audio_info(&aoi);
	obs_data_set_default_string(settings, "device_id", "default");
	obs_data_set_default_int(settings, "speaker_layout", aoi.speakers);
	int recorded_channels = get_audio_channels(aoi.speakers);

	for (int i = 0; i < recorded_channels; i++) {
		std::string name = "route " + std::to_string(i);
		obs_data_set_default_int(settings, name.c_str(), -1);
	}
	for (int i = recorded_channels; i < MAX_AUDIO_CHANNELS; i++) {
		std::string name = "route " + std::to_string(i);
		obs_data_set_default_int(settings, name.c_str(), -1);
	}
}

void register_asio_source()
{
	struct obs_source_info asio_input_capture = {};
	asio_input_capture.id = "asio_input_capture";
	asio_input_capture.type = OBS_SOURCE_TYPE_INPUT;
	asio_input_capture.output_flags = OBS_SOURCE_AUDIO | OBS_SOURCE_DO_NOT_DUPLICATE;
	asio_input_capture.get_name = asio_input_getname;
	asio_input_capture.create = asio_input_create;
	asio_input_capture.destroy = asio_destroy;
	asio_input_capture.update = asio_update, asio_input_capture.get_defaults = asio_defaults;
	asio_input_capture.get_properties = asio_input_properties;
	asio_input_capture.icon_type = OBS_ICON_TYPE_AUDIO_INPUT;
	obs_register_source(&asio_input_capture);
}

bool obs_module_load(void)
{
	list = new ASIOAudioIODeviceList();
	list->scanForDevices();
	register_asio_source();
	return true;
}

void obs_module_unload()
{
	delete list;
}
