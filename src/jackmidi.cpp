#include "jackmidi.h"
#include <iostream>
#include <jack/jack.h>
#include <jack/midiport.h>

midi_client::midi_client()
{
    if((client = jack_client_open("grid-control", JackNullOption, NULL)) == NULL)
    {
        std::cerr << "\nJack server not running, program will exit." << std::endl;        
        exit(1);
    } 
    in_port = jack_port_register(client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
    out_port = jack_port_register(client, "midi_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);

    jack_set_process_callback(client, midi_client::static_process, this);

    if (jack_activate(client) != 0) {
        std::cout<<  "cannot activate client" << std::endl;
        return;
    }

    jack_connect(client, "a2j:APC MINI [24] (capture): APC MINI MIDI 1", "grid-control:midi_in");
    jack_connect(client, "grid-control:midi_out", "a2j:APC MINI [24] (playback): APC MINI MIDI 1");
}

midi_client::~midi_client() {
    if (!client) return;
    if(jack_deactivate(client)) {
        std::cerr << "Failed to close client;\n";
    }
}
void midi_client::register_midi_callback (midi_callback callback)
{
    this->callback = callback;
}

int midi_client::static_process(jack_nframes_t nframes, void* self)
{
    return static_cast<midi_client*>(self)->process(nframes);
}

int midi_client::process(jack_nframes_t nframes) {
    
    void* in_port_buf = jack_port_get_buffer(in_port, nframes);
    void* out_port_buf = jack_port_get_buffer(out_port, nframes);

    jack_midi_event_t event;

    jack_midi_clear_buffer(out_port_buf);

    /* process midi in */
    jack_nframes_t ev_count = jack_midi_get_event_count(in_port_buf);
    for (int i{0}; i < ev_count; i++) {
        jack_midi_event_get(&event, in_port_buf, i);
        if (event.size >= 3) {
            callback(
                *this, 
                event.buffer[0],
                event.buffer[1],
                event.buffer[0] == (NOTEOFF) ? 0 : event.buffer[2]
            );
        }
    }
    /* process midi out */
    for (int i{0}; i < nframes; i++) 
    {
        for (int e{0}; e < playback_index; e++)
        {
            u_int8_t* buffer = jack_midi_event_reserve(out_port_buf, 0, 3);
            if (buffer == 0) {
                std::cerr << "error writing midi event!\n";
            } else {
                buffer[0] = event_pool.at(e)[0]; 
                buffer[1] = event_pool.at(e)[1]; 
                buffer[2] = event_pool.at(e)[2];
            }
        }
        playback_index = 0;
    } 
    return 0;
}

void midi_client::output_note(u_int8_t key, u_int8_t velocity) 
{
    event_pool[playback_index] = {NOTEON, key, velocity};
    playback_index++;
}