#pragma once
#include <jack/jack.h>
#include <jack/types.h>
#include <functional>
#include <vector>
#include <array>

struct midi_client 
{
    midi_client();
    ~midi_client();

    jack_client_t* client;
    jack_port_t* in_port;
    jack_port_t* out_port;

    using midi_callback = std::function<void(midi_client&, int, int, int)>;

    using midi_event = std::array<u_int8_t, 3>;

    std::array<midi_event, 128> event_pool;

    size_t playback_index{0};

    midi_callback callback;

    static int static_process(jack_nframes_t, void*);
    int process(jack_nframes_t); 

    enum E_TYPE {
        NOTEON = 144,
        NOTEOFF = 128,
        CONTROL = 176
    };

    void register_midi_callback(midi_callback callback);

    int get_midi_event(int& ev_type, int& addr, int& value);
    void output_note(unsigned char key, unsigned char velocity);
    void connect(int sender_id, int sender_port, int dest_id, int dest_port);
};