#include "gridcontrol.h"
//#include "midiclient.h"
#include "jackmidi.h"
#include "oscserver.h"

#include <lo/lo.h>
#include <lo/lo_types.h>

#include <iostream>
#include <signal.h>
#include <string>
#include <vector>
#include <cmath>
#include <functional>

using namespace grid_control;

/* global variables */

grid_state_t<> current_state;

std::vector<grid_state_t<>> stored_states;

std::array<track, line_size> tracks;

std::array<control_page<>, ctrl_page_count> control_pages; 

void populate_params(track_params<>&);

void populate_pages();

int selected_track{0}, active_ctrl_page{0};

enum FADERS_TARGET {
    NONE,
    ONE_TRACK,
    VOL_MIXER,
    VERB_SEND
} faders_target = NONE;

/* OSC function overloads */

lo_address osc_dest;

void send_osc(const char* address, void* data)
{
    lo_blob blob = lo_blob_new(sizeof(data), data);
    lo_send(osc_dest, address, "b", blob);
}

void send_osc(const char* address, const char* str_value)
{
    lo_send(osc_dest, address, "s", str_value);
}

void send_osc(const char* address, int i_value) {
    lo_send(osc_dest, address, "i", i_value);
}

void send_osc(const char* address, float f_value) {
    lo_send(osc_dest, address, "f", f_value);
}

void send_osc(const char* address, float f_value, const char* str_value) {
    lo_send(osc_dest, address, "fs", f_value, str_value);
}

/* midi out events for setting lights */

void set_light(midi_client& client, int line, int col, light_color color) {
    u_int8_t key = line * line_size + col;
    client.output_note(key, (u_int8_t)color);
    current_state[line][col] = color;
}

void set_line_light(midi_client& client, int line, light_color color)
{   
    for (int i{0}; i < line_size; i++)
        set_light(client, line, i, color);
}

void set_column_light(midi_client& client, int column, light_color color)
{
    for (int i{0}; i < max_lines; i++) {
        set_light(client, i, column, color);
    }   
}

void turn_all_lights_off(midi_client& client)
{
    int grid_size = line_size * line_size;
    for (int i{0}; i < grid_size; i++) {
        client.output_note(i, 0);
        current_state[i / line_size][i % line_size] = OFF;
    }
    for (int i{grid_size+1}; i < APC_SHIFT; i++) {
        client.output_note(i, 0);
    }
}

/* routines for grid controls */

void select_track(midi_client& client, int track)
{
    //when de-selecting, fallback to green or off;
    light_color fallback_light{GRN};
    if (tracks[selected_track].type == LOOP)
        fallback_light = OFF;

    set_light(client, track_line, selected_track, fallback_light);
    if (tracks[selected_track].type == LOOP) {
        set_line_light(client, tracks[selected_track].grid_line, fallback_light);
    }
    selected_track = track;

    set_light(client, track_line, selected_track, YLW);
    if (tracks[selected_track].type == LOOP) {
        set_line_light(client, tracks[selected_track].grid_line, YLW);
    }
}

bool toggle_loop_track(midi_client& client, int line, int column, int vel) 
{
    static int track_pressed{-1};

    if (line == track_line) {
        if (vel == 0)
            track_pressed = -1;
        else
            track_pressed = column;
    } else {
        if (track_pressed >= 0 && vel > 0) {
            auto& track = tracks[track_pressed];
            if (track.type == LOOP) {
                set_line_light(client, track.grid_line, OFF);
                if (track.grid_line == line) {
                    track.type = SHORT;
                    std::string addr{ "/track/"+std::to_string(track_pressed) + "/type"};
                    send_osc(addr.c_str(), "short");
                    track.grid_line = -1;
                    return track_pressed >= 0;
                }
            } else {
                track.type = LOOP;
                std::string addr{ "/track/"+std::to_string(track_pressed) + "/type"};
                send_osc(addr.c_str(), "loop");
            }
            track.grid_line = line;
            set_line_light(client, line, YLW);
        }
    }
    return track_pressed >= 0;
}

void set_faders_target(midi_client& client, int key, int vel) {
    switch(key) {
        case APC_VOLUME:
            if (faders_target == VOL_MIXER) {
                faders_target = NONE;
                client.output_note(key, 0);
            } else {
                faders_target = VOL_MIXER;
                client.output_note(key, 1);
                client.output_note(APC_DEVICE, 0);
                client.output_note(APC_SEND, 0);
            }
            break;
        case APC_SEND:
            if (faders_target == VERB_SEND) {
                faders_target = NONE;
                client.output_note(key, 0);
            } else {
                faders_target = VERB_SEND;
                client.output_note(key, 1);
                client.output_note(APC_VOLUME, 0);
                client.output_note(APC_DEVICE, 0);
            }
            break;
        case APC_DEVICE:
            if (faders_target == ONE_TRACK) {
                faders_target = NONE;
                client.output_note(key, 0);
            } else {
                faders_target = ONE_TRACK;
                client.output_note(key, 1);
                client.output_note(APC_VOLUME, 0);
                client.output_note(APC_SEND, 0);
            }
            break;
    }
}

void set_parameter_max_value
(int track, std::string p_name, parameter::GROUP group, int max)
{
    for (auto& param : tracks[track].params)
        if (param.name == p_name && param.group == group)
            param.max = max;
}

void set_parameter
(int track, std::string p_name, parameter::GROUP group, control_type type, int value)
{
    for (auto& param : tracks[track].params) {
        if (param.name == p_name && param.group == group) {
            float real_value;
            switch (type) {
                case RANGE_7BIT:
                    real_value = ((float)value / (float)RANGE_7BIT);
                    if (param.unit == parameter::DB)
                        real_value = sqrt(real_value);
                    else if (param.unit == parameter::HZ)
                        real_value = pow(real_value, 3);
                    real_value = real_value * (param.max - param.min) + param.min;
                    break;
                case RANGE_10BIT:
                    real_value = ((float)value / (float)RANGE_10BIT);
                    if (param.unit == parameter::DB)
                        real_value = sqrt(real_value);
                    else if (param.unit == parameter::HZ)
                        real_value = pow(real_value, 3);
                    real_value = real_value * (param.max - param.min) + param.min;
                    break;
                case INCREMENTAL:
                    real_value = std::max(param.min, std::min(param.max, param.value + value));
                case TOGGLE_SW:
                    real_value = !param.value;
                
            }
            std::string unit{""}, group;
            switch (param.unit) {
                case parameter::DB:
                    unit = "db"; break;
                case parameter::CENT:
                    unit = "cent"; 
                    real_value = trunc(real_value);
                    break;
                case parameter::HZ:
                    unit = "hz"; break;
                default:
                    break;
            }
            switch (param.group) {
                case parameter::SOURCE:
                    group = "source"; break;
                case parameter::AMP:
                    group = "amp"; break;
                case parameter::FILTER:
                    group = "filter"; break;
                case parameter::MODULATION:
                    group = "modulation"; break;
                case parameter::DELAY:
                    group = "delay"; break;
                case parameter::REVERB:
                    group = "reverb"; break;
            }
            std::string addr{ "/track/" + std::to_string(track) + "/" + group + "/" + p_name };
            send_osc(addr.c_str(), real_value);
            break;
        }
    }
}

void use_control_page(int channel, int value)
{
    for (const auto& control : control_pages[active_ctrl_page])
        if (control.id == channel)
            set_parameter(
                selected_track, 
                control.p_name, 
                control.group,  
                control.ctrl_type,
                value
            );
}

void change_track_volume(int channel, int value) {
    set_parameter(channel - APC_FADER_CC, "level", parameter::AMP, control_type::RANGE_7BIT, value);
}

void change_track_reverb(int channel, int value) {
    set_parameter(channel - APC_FADER_CC, "send", parameter::REVERB, control_type::RANGE_7BIT, value);
}

void play_slice(int line, int column, int velocity) 
{    
    for (int i{0}; i < line_size; i++)
        if (tracks[i].grid_line == line) {
            std::string addr{ "/track/" + std::to_string(i) + "/slice/" + std::to_string(column) };
            send_osc(addr.c_str(), velocity);
        }
}

/* main hub for actions */
void process_midi_input(midi_client& client, int ev_type, int addr, int value)
{
    int line, column;
    if (ev_type == midi_client::E_TYPE::NOTEON || ev_type == midi_client::E_TYPE::NOTEOFF) {
        if (addr < max_buttons) {
            line = addr / line_size;
            column = addr % line_size;
            //routines
            if (line == track_line && value > 0)
                select_track(client, column);
            bool track_is_pressed = toggle_loop_track(client, line, column, value);
            if (!track_is_pressed && line != track_line)
                play_slice(line, column, value);
        }
        else {
            if (value > 0) 
                set_faders_target(client, addr, value);
        }
    }
    else if (ev_type == midi_client::E_TYPE::CONTROL) {
        switch(faders_target) {
            case ONE_TRACK:
                use_control_page(addr, value); break;
            case VOL_MIXER:
                change_track_volume(addr, value); break;
            case VERB_SEND:
                change_track_reverb(addr, value); break;
                break;
            default:
                break;
        }
    }
}

/*Main program execution*/
midi_client client;

/* reactive functions for upcoming OSC messages */

void update_track_position(int track, int position) {
    if (track < tracks.size()) {
        auto& target = tracks[track];
        if (target.grid_line >= 0 && target.lit_line_button >= 0) {
            light_color fallback_light = OFF;
            if (selected_track == track) fallback_light = YLW;
            set_light(client, target.grid_line, target.lit_line_button, fallback_light);
        }
        target.lit_line_button = position;
        set_light(client, target.grid_line, target.lit_line_button, GRN);
    }
}

void update_track_state(int track, int state) {
    if (track < tracks.size()) {
        auto& target = tracks[track];
        if (state == STOPPED && target.grid_line >= 0 && target.lit_line_button >= 0) {
            light_color fallback_light = OFF;
            if (selected_track == track) fallback_light = YLW;
            set_light(client, target.grid_line, target.lit_line_button, fallback_light);
        }
        target.state = (track_state)state;
        target.lit_line_button = -1;
    }
}

void exit_handler(sig_atomic_t s) {
    turn_all_lights_off(client);
    exit(0);
}

int main() 
{ 
    /* initialize tracks */
    for (auto& track : tracks) {
        track.type = SHORT;
        track.grid_line = -1;
        track.lit_line_button = -1;
        track.state = STOPPED;
        populate_params(track.params);
    }   

    /* set OSC ports */
    osc_dest = lo_address_new(NULL, "4444");
    /* osc_serv should be able to Modify Track array */
    osc_server osc_serv{"4445"};

    osc_serv.play_position_calback = update_track_position;
    osc_serv.track_state_callback = update_track_state;

    midi_client::midi_callback process_midi = process_midi_input;
    client.register_midi_callback(process_midi);

    populate_pages();

    set_line_light(client, track_line, GRN);

    signal(SIGINT, exit_handler);

    /* enter main loop */
    while (1) {}

    return 0;
}

/* hard coding parameters */
void populate_params(track_params<>& params)
{
    int i{0};
    //all slider controls
    params[i++] = {"level", parameter::AMP, parameter::DB, -100, 12, 0 };
    params[i++] = {"overdrive", parameter::AMP, parameter::GENERIC, 0, 80, 0 };
    params[i++] = {"bitcrush", parameter::AMP, parameter::GENERIC, 0, 1, 0 };
    params[i++] = {"downsamp", parameter::AMP, parameter::GENERIC, 0, 1, 0 };
    params[i++] = {"pitch", parameter::SOURCE, parameter::CENT, -12, 12, 0 };
    params[i++] = {"detune", parameter::SOURCE, parameter::CENT, -50, 50, 0 };
    params[i++] = {"cutoff", parameter::FILTER, parameter::HZ, 0, 21000, 0 };
    params[i++] = {"q", parameter::FILTER, parameter::GENERIC, 0.75, 4, 0 };
    params[i++] = {"steepness", parameter::FILTER, parameter::GENERIC, 0, 1, 0 };
    params[i++] = {"level", parameter::MODULATION, parameter::GENERIC, 0, 1, 0 };
    params[i++] = {"intensity", parameter::MODULATION, parameter::HZ, 0, 16, 0 };
    params[i++] = {"rate", parameter::MODULATION, parameter::HZ, 0, 12, 0 };
    params[i++] = {"waveform", parameter::MODULATION, parameter::GENERIC, 0, 1, 0 };
    params[i++] = {"level", parameter::DELAY, parameter::DB, -100, 0, 0 };
    params[i++] = {"feedback", parameter::DELAY, parameter::DB, -100, 0, 0 };
    params[i++] = {"lowpass", parameter::DELAY, parameter::HZ, 0, 21000, 0 };
    params[i++] = {"highpass", parameter::DELAY, parameter::HZ, 0, 21000, 0 };
    params[i++] = {"mod-intensity", parameter::DELAY, parameter::CENT, 0, 200, 0 };
    params[i++] = {"mod-rate", parameter::DELAY, parameter::HZ, 0, 12, 0 };
    params[i++] = {"mod-waveform", parameter::DELAY, parameter::GENERIC, 0, 1, 0 };
    params[i++] = {"send", parameter::REVERB, parameter::DB, -100, 0, 0 };
    params[i++] = {"delay", parameter::REVERB, parameter::DB, -100, 0, 0 };
    //all toggle controls
    params[i++] = {"reverse", parameter::SOURCE, parameter::GENERIC, 0, 1, 0 };
    params[i++] = {"loop", parameter::SOURCE, parameter::GENERIC, 0, 1, 0 };
    params[i++] = {"hard-clip", parameter::AMP, parameter::GENERIC, 0, 1, 0 };
    params[i++] = {"interpolation", parameter::AMP, parameter::GENERIC, 0, 1, 0 };
    params[i++] = {"reverse", parameter::DELAY, parameter::GENERIC, 0, 1, 0 };
    //all precision 
    params[i++] = {"file", parameter::SOURCE, parameter::GENERIC, 0, 1, 0 };
    params[i++] = {"trim-start", parameter::SOURCE, parameter::GENERIC, 0, 1, 0 };
    params[i++] = {"trim-end", parameter::SOURCE, parameter::GENERIC, 0, 1, 0 };
    params[i++] = {"loop-point", parameter::SOURCE, parameter::GENERIC, 0, 1, 0 };
    params[i++] = {"pitch", parameter::DELAY, parameter::CENT, -12, 12, 0 };
    params[i++] = {"detune", parameter::DELAY, parameter::CENT, -50, 50, 0 };
    params[i++] = {"tempo", parameter::DELAY, parameter::GENERIC, 0.125, 2, 0 };
}

void populate_pages() 
{
    int i{0}, midi_ch{APC_FADER_CC};
    control_pages[0][i++] = {midi_ch++, RANGE_7BIT, "cutoff", parameter::FILTER};
    control_pages[0][i++] = {midi_ch, RANGE_7BIT, "steepness", parameter::FILTER};
    control_pages[0][i++] = {midi_ch++, RANGE_7BIT, "q", parameter::FILTER};
    control_pages[0][i++] = {midi_ch++, RANGE_7BIT, "downsamp", parameter::AMP};
    control_pages[0][i++] = {midi_ch++, RANGE_7BIT, "overdrive", parameter::AMP};
    control_pages[0][i++] = {midi_ch++, RANGE_7BIT, "level", parameter::MODULATION};
    control_pages[0][i++] = {midi_ch++, RANGE_7BIT, "intensity", parameter::MODULATION};
    control_pages[0][i++] = {midi_ch++, RANGE_7BIT, "rate", parameter::MODULATION};
    control_pages[0][i++] = {midi_ch++, RANGE_7BIT, "pitch", parameter::SOURCE};

    i = 0;
    midi_ch = APC_FADER_CC;
    control_pages[1][i++] = {midi_ch++, RANGE_7BIT, "level", parameter::DELAY};
    control_pages[1][i++] = {midi_ch++, RANGE_7BIT, "feedback", parameter::DELAY};
    control_pages[1][i++] = {midi_ch++, RANGE_7BIT, "time", parameter::DELAY};
    control_pages[1][i++] = {midi_ch++, RANGE_7BIT, "lowpass", parameter::DELAY};
    control_pages[1][i++] = {midi_ch++, RANGE_7BIT, "mod-intensity", parameter::DELAY};
    control_pages[1][i++] = {midi_ch++, RANGE_7BIT, "mod-rate", parameter::DELAY};
    control_pages[1][i++] = {midi_ch++, RANGE_7BIT, "delay", parameter::REVERB};
}