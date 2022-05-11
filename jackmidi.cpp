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

    if (jack_connect(client, "APC MINI[24](capture):APC MINI MIDI 1", "grid-control:midi_in") != 0)
        std::cout << "failed to connect \n";
    //jack_connect(client, "grid-control:midi_out", "APC MINI[24](playback):APC MINI MIDI 1");
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

    return 0;
}

int midi_client::get_midi_event(int &ev_type, int &addr, int &value) {
    return -1;
}

void midi_client::connect(int sender_id, int sender_port, int dest_id, int dest_port) {

}

void midi_client::output_note(unsigned char key, unsigned char velocity) {
    
}