#include "midiclient.h"
#include <alsa/seq_event.h>
#include <alsa/seqmid.h>
#include <iostream>

void note_printout(snd_seq_event_t* event);
void midi_printout(snd_seq_event_t* event);

client::client() {
    snd_seq_open(&seq_handle, "default", SND_SEQ_OPEN_DUPLEX, 0);
    snd_seq_set_client_name(seq_handle, "Grid Control");
    midi_in = snd_seq_create_simple_port(
        seq_handle,
        "midi_in",    
        SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
        SND_SEQ_PORT_TYPE_APPLICATION
    );
    midi_out = snd_seq_create_simple_port(
        seq_handle, 
        "midi_out", 
        SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,
        SND_SEQ_PORT_TYPE_APPLICATION
    );
}

client::~client() {
    snd_seq_close(seq_handle);
}

void client::connect(int sender_id, int sender_port, int dest_id, int dest_port)
{
    snd_seq_addr_t sender, dest;
    snd_seq_port_subscribe_t* subs;
    dest.client = dest_id;
    dest.port = dest_port;
    sender.client = sender_id;
    sender.port = sender_port;
    snd_seq_port_subscribe_alloca(&subs);
    snd_seq_port_subscribe_set_sender(subs, &sender);
    snd_seq_port_subscribe_set_dest(subs, &dest);
    snd_seq_subscribe_port(seq_handle, subs);
}

int client::get_midi_event(int& ev_type, int &addr, int &value)
{
    int count;
    snd_seq_event_t* event = NULL;
    count = snd_seq_event_input(seq_handle, &event);

    switch(event->type) {
        case SND_SEQ_EVENT_NOTEON:
            addr = event->data.note.note;
            value = event->data.note.velocity;
            ev_type = NOTE;
            break;
        case SND_SEQ_EVENT_NOTEOFF:
            addr = event->data.note.note;
            value = 0;
            ev_type = NOTE;
            break;
        case SND_SEQ_EVENT_CONTROLLER:
            addr = event->data.control.param;
            value = event->data.control.value;
            ev_type = CONTROL;
            break;
        default:
            count = -1;
            break;
    }
    return count;
}

void client::output_note(unsigned char key, unsigned char velocity)
{
    snd_seq_event_t event;
    snd_seq_ev_clear(&event);
    snd_seq_ev_set_source(&event, midi_out);
    snd_seq_ev_set_subs(&event);
    snd_seq_ev_set_direct(&event);
    snd_seq_ev_set_noteon(&event, 0, key, velocity);
    snd_seq_event_output_direct(seq_handle, &event);
}

/***************************************************************************
**** the below functions are for reference only, and should not be used ****
****************************************************************************/

void midi_through(snd_seq_t* seq, int out_port)
{
    snd_seq_event_t* event = NULL;
    while (snd_seq_event_input(seq, &event) >= 0)
    {   
        snd_seq_ev_set_source(event, out_port);
        snd_seq_ev_set_subs(event);
        snd_seq_ev_set_direct(event);
        /*
            Why ? 
            APC Mini probably have a simple MIDI interface, that only receives note-ons.
            When AlsaSeq interprets note-offs (coming from the very same APC), APC needs 
            another note-on with 0 velocity!
        */
        if (event->type == SND_SEQ_EVENT_NOTEOFF)
            snd_seq_ev_set_noteon(
                event, 
                event->data.note.channel, 
                event->data.note.note, 
                0 //velocity
            );
        snd_seq_event_output_direct(seq, event);
        snd_seq_free_event(event);
    }
}

const char* get_event_type(snd_seq_event_t* event)
{
    const char* type;
    switch(event->type) {
        case SND_SEQ_EVENT_NOTEON:
            type = "note on"; break;
        case SND_SEQ_EVENT_NOTEOFF:
            type = "note off"; break;
        case SND_SEQ_EVENT_CONTROLLER:
            type = "controller"; break;
        case SND_SEQ_EVENT_NOTE:
            type = "note?"; break;
        case SND_SEQ_EVENT_PITCHBEND:
            type = "pitch bend"; break;
        case SND_SEQ_EVENT_KEYPRESS:
            type = "keypress?"; break;
        default:
            type = "none listed"; break;
    }
    return type;
}

void midi_printout(snd_seq_event_t* event)
{   
    std::cout<< "----------------\nMIDI Event Info:\n"
        << "source client: " << (int)event->source.client << '\n'
        << "source port: " << (int)event->source.port << '\n'
        << "dest. client: " << (int)event->dest.client << '\n'
        << "dest. port: " << (int)event->dest.port << '\n'
        << "flags: " << (int)event->flags << '\n'
        << "queue: " << (int)event->queue << '\n'
        << "tag: " << (int)event->tag << '\n'
        << "time sec: " << event->time.time.tv_sec << '\n'
        << "time nsec: " << event->time.time.tv_nsec << '\n'
        << "tick: " << event->time.tick << '\n'
        << "type: " << get_event_type(event) << '\n';
}

void note_printout(snd_seq_event_t* event)
{
    std::cout<< "----------------\nMIDI Note Info:\n"
        << "type: " << get_event_type(event) << '\n'
        << "note: " << (int)event->data.note.note << '\n'
        << "velocity: " << (int)event->data.note.velocity << '\n'
        << "off-velocity: " << (int)event->data.note.off_velocity << '\n'
        << "channel: " << (int)event->data.note.channel << '\n';
}