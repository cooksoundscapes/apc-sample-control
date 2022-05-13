#include <array>
#include <sys/types.h>
#include <string>

//Akai APC Mini side buttons
constexpr int APC_UP = 64;
constexpr int APC_DOWN = 65;
constexpr int APC_LEFT = 66;
constexpr int APC_RIGHT = 67;
constexpr int APC_VOLUME = 68;
constexpr int APC_PAN = 69;
constexpr int APC_SEND = 70;
constexpr int APC_DEVICE = 71;
constexpr int APC_CLIP_STOP = 82;
constexpr int APC_SOLO = 83;
constexpr int APC_REC_ARM = 84;
constexpr int APC_MUTE = 85;
constexpr int APC_SELECT = 86;
constexpr int APC_USER_1 = 87;
constexpr int APC_USER_2 = 88;
constexpr int APC_STOP_ALL_CLIPS = 89;
constexpr int APC_SHIFT = 98;
constexpr int APC_FADER_CC = 48;

// program setup;
constexpr int line_size = 8;
constexpr int max_lines = 7;
constexpr int max_buttons = line_size * max_lines;
constexpr int track_line = 6; //track select line
constexpr int controls_per_page = 16;
constexpr int ctrl_page_count = 4;
constexpr int default_param_count = 48;

namespace grid_control {

enum light_color {
    OFF = 0,
    GRN = 1,
    RED = 3,
    YLW = 5
};

enum sample_type {
    SHORT,
    LOOP,
    LONG,
    MELODIC
};

enum track_state {
    STOPPED,
    PLAYING,
    RECORDING,
    ARMED_FOR_REC,
    WAITING_CUE
};

struct parameter {
    std::string name;
    enum GROUP {
        SOURCE,
        AMP,
        FILTER,
        MODULATION,
        DELAY,
        REVERB
    } group;
    enum UNIT {
        DB,
        CENT,
        HZ,
        GENERIC
    } unit;
    float min, max, value;
};

enum control_type {
    RANGE_7BIT = 127,
    RANGE_10BIT = 1023,
    TOGGLE_SW = 2,
    INCREMENTAL = 3
};

//keeps relations between one control and one parameter
struct param_control {
    int id; //or midi channel
    control_type ctrl_type;
    std::string p_name;
    parameter::GROUP group;
};

// size = number of controls per "page"
template<int S = controls_per_page>
using control_page = std::array<param_control, S>;

// actual grid size 
template<int Row = max_lines, int Column = line_size>
using grid_state_t = std::array<std::array<light_color, Column>, Row>; 

// size = number of params per track;
template<int ParamCount = default_param_count>
using track_params = std::array<parameter, ParamCount>;

struct track {
    sample_type type;
    track_params<> params;  
    int grid_line;
    int lit_line_button;
    track_state state;
};

}   