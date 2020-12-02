#pragma once
#include "../server/sim_parser.h"

void client_connect(Request& req, Response& res);

void client_join(Request& req, Response& res);

void client_quit(Request& req, Response& res);

void client_heartbeat(Request& req, Response& res);

