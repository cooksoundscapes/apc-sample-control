# Sample Grid Control for Akai APC Mini

Sends/receives MIDI messages to/from Akai APC Mini grid controller;
Sends/receives OSC messages for host program;

Sample chopping, drum machine, rec/play/save/load playing patterns, parameter controls.

## Parameters by control type:

control type Slider implies that the USER doesn't need to know the exact value of the param,
min or max values and change curve. Precision doesn't define the physical device, but implies
that the user MUST know the parameter value exactly. It can be arrows, rotary encoders, etc.

Slider:
* grouped by track:
    * amp level
    * reverb send
* source + amp page:
    * amp overdrive
    * amp bitcrush
    * amp downsamp
* filter + mod page:
    * filter cutoff
    * filter Q
    * filter steepness (2p-4p)
    * modulation level
    * modulation intensity
    * modulation speed
    * modulation waveform (sine/tri)
* delay page:
    * delay level
    * delay feedback
    * delay lowpass
    * delay highpass
    * delay mod intensity
    * delay mod speed
    * delay mod waveform (sine/tri)
    * delay reverb send

Some toggles may be automatic?
source reverse part of performance (on nano pad AND grid line)
track type toggles loop;
hard-clip when overdrive at max;

Toggle switch
* source reverse
* source loop
* amp hard-clip
* amp interpolation
* delay reverse

Precision encoder (?)
* source speed
* source trim start
* source trim end
* source loop point
* delay time tempo ratios
* delay pitch
* delay voices
* random controls for delay

Undefined
* source file
* delay magic

----------------------

What can I do with a selected track?

    change parameters
    toggle loop mode - OK
    record audio
What can I do with a track (regardless of selection) ?

    fast mute (momentary)
    fast reverse (momentary)
    stop loop (holding stop clip button and pressing on loop)
What Else??

    shift button toggle pattern view
    when recording, could reserve 4 buttons to be a visual pre count
    think about parameters that demand precision (like pitch) and so on
    think about controls that should trigger on metronome (like audio record)
    think about sending only the necessary to PD;
