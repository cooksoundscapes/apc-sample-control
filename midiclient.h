#pragma once
#include <alsa/asoundlib.h>
#include <array>

struct midi_client 
{
    snd_seq_t* seq_handle;
    int midi_in, midi_out;

    midi_client();
    ~midi_client();

    enum E_TYPE {
        NOTE,
        CONTROL
    };

    int get_midi_event(int& ev_type, int& addr, int& value);
    void output_note(unsigned char key, unsigned char velocity);

    void connect(int sender_id, int sender_port, int dest_id, int dest_port);
};