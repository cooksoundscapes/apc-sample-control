#pragma once
#include <alsa/asoundlib.h>
#include <array>

struct client 
{
    snd_seq_t* seq_handle;
    int midi_in, midi_out;

    client();
    ~client();

    enum E_TYPE {
        NOTE,
        CONTROL
    };

    int get_midi_event(int& ev_type, int& addr, int& value);
    void output_note(unsigned char key, unsigned char velocity);

    void connect(int sender_id, int sender_port, int dest_id, int dest_port);
};