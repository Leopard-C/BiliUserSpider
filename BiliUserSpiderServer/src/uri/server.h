#pragma once

#include "../server/sim_parser.h"

void server_connect(Request& req, Response& res);

void server_status(Request& req, Response& res);

void server_start(Request& req, Response& res);

void server_stop(Request& req, Response& res);

void server_shutdown(Request& req, Response& res);

void server_get_clients_info(Request& req, Response& res);

void server_quit_client(Request& req, Response& res);

void server_get_task_progress(Request& req, Response& res);

void server_set_task_range(Request& req, Response& res);

