#pragma once
#include <iostream>
#include <lo/lo.h>
#include <lo/lo_types.h>
#include <vector>

struct osc_server 
{
    osc_server(const char* port, int track_count);
    ~osc_server();

    lo_server_thread server;

    static std::vector<int> track_positions;

    static int track_position(int);

    static void error_handler(int, const char*, const char*);

    static int play_position_handler(
        const char *path, 
        const char *types, 
        lo_arg ** argv,
        int argc, 
        void *data, 
        void *user_data
    );
};