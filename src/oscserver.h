#pragma once
#include <iostream>
#include <lo/lo.h>
#include <lo/lo_types.h>
#include <vector>
#include <functional>

struct osc_server 
{
    osc_server(const char* port);
    ~osc_server();

    lo_server_thread server;

    static void error_handler(int, const char*, const char*);

    static std::function<void(int, int)> play_position_calback;
    static int play_position_handler(
        const char *path, 
        const char *types, 
        lo_arg ** argv,
        int argc, 
        void *data, 
        void *user_data
    );

    static std::function<void(int, int)> track_state_callback;
    static int track_state_handler(
        const char *path, 
        const char *types, 
        lo_arg ** argv,
        int argc, 
        void *data, 
        void *user_data
    );
};