#pragma once
#include "../server/sim_parser.h"

void config_database_set(Request& req, Response& res);

void config_database_get(Request& req, Response& res);

void config_client_set(Request& req, Response& res);

void config_client_get(Request& req, Response& res);

void config_proxy_set(Request& req, Response& res);

void config_proxy_get(Request& req, Response& res);

void config_server_set(Request& req, Response& res);

void config_server_get(Request& req, Response& res);

void config_advance_set(Request& req, Response& res);

void config_advance_get(Request& req, Response& res);

