#include "oscserver.h"
#include <bits/types/FILE.h>
#include <cstddef>
#include <lo/lo_lowlevel.h>
#include <lo/lo_serverthread.h>
#include <lo/lo_types.h>
#include <sys/select.h>

std::function<void(int, int)> osc_server::play_position_calback;
std::function<void(int, int)> osc_server::track_state_callback;

osc_server::osc_server(const char* port)
{
    server = lo_server_thread_new(port, error_handler);

    lo_server_thread_add_method(
        server, 
        "/track/position", 
        "ff", 
        play_position_handler,
        NULL
    );
    lo_server_thread_add_method(
        server, 
        "/track/state", 
        "ff", 
        track_state_handler,
        NULL
    );
    lo_server_thread_start(server);
}

osc_server::~osc_server() {
    lo_server_thread_free(server);
}

void osc_server::error_handler(int num, const char* msg , const char* path)
{
    std::cout << "[OSC Server] " << num << ": '" << msg << "' at: " << path << '\n';
}

int osc_server::play_position_handler
(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data)
{
    if (argc < 2) return 1;
    float track = argv[0]->f;
    float pos = argv[1]->f;
    play_position_calback(track, pos);
    return 0;
}

int osc_server::track_state_handler
(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data)
{
    if (argc < 2) return 1;
    float track = argv[0]->f;
    float state = argv[1]->f;
    track_state_callback(track, state);
    return 0;
}
