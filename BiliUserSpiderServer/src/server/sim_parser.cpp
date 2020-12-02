/*
 * http_parser.cpp
 *
 *  Created on: Oct 26, 2014
 *      Author: liao
 */
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include "sim_parser.h"
#include "http-parser/http_parser.h"
#include "multipart-parser-c/multipart_parser.h"
#include "string_utils.h"
#include "ehttp_version.h"
#include "../util/url_code.h"
#include "../log/logger.h"

#define MAX_REQ_SIZE 10485760

std::string RequestParam::get_param(std::string &name) {
    std::multimap<std::string, std::string>::iterator i = this->_params.find(name);
    if (i == _params.end()) {
        return std::string();
    }
    return i->second;
}

void RequestParam::get_params(const std::string &name, std::vector<std::string> &params) {
    std::pair<std::multimap<std::string, std::string>::iterator, std::multimap<std::string, std::string>::iterator> ret = this->_params.equal_range(name);
    for (std::multimap<std::string, std::string>::iterator it=ret.first; it!=ret.second; ++it) {
        params.push_back(it->second);
    }
}

int RequestParam::parse_query_url(std::string &query_url) {
    std::stringstream query_ss(query_url);
    LDebug("start parse_query_url:{}", query_url.c_str());

    while(query_ss.good()) {
        std::string key_value;
        std::getline(query_ss, key_value, '&');
        LDebug("get key_value:{}", key_value.c_str());

        std::stringstream key_value_ss(key_value);
        while(key_value_ss.good()) {
            std::string key, value;
            std::getline(key_value_ss, key, '=');
            std::getline(key_value_ss, value, '=');
            _params.insert(std::pair<std::string, std::string>(key, util::urlDecode(value)));
        }
    }
    return 0;
}
    
int RequestParam::add_param_pair(const std::string &key, const std::string &value) {
    _params.insert(std::pair<std::string, std::string>(key, value));
    return 0;
}

std::string RequestLine::get_request_uri() {
    std::stringstream ss(this->get_request_url());
    std::string uri;
    std::getline(ss, uri, '?');
    return uri;
}

RequestParam &RequestLine::get_request_param() {
    return _param;
}

std::string RequestLine::to_string() {
    std::string ret = "method:";
    ret += _method;
    ret += ",";
    ret += "request_url:";
    ret += _request_url;
    ret += ",";
    ret += "http_version:";
    ret += _http_version;
    return ret;
}

int RequestLine::parse_request_url_params() {
    std::stringstream ss(_request_url);
    LDebug("start parse params which request_url:{}", _request_url.c_str());

    std::string uri;
    std::getline(ss, uri, '?');
    if(ss.good()) {
        std::string query_url;
        std::getline(ss, query_url, '?');

        _param.parse_query_url(query_url);
    }
    return 0;
}

std::string RequestLine::get_method() {
    return _method;
}

void RequestLine::set_method(std::string m) {
    _method = m;
}

std::string RequestLine::get_request_url() {
    return _request_url;
}

void RequestLine::set_request_url(std::string url) {
    _request_url = url;
}

void RequestLine::append_request_url(std::string p_url) {
    _request_url += p_url;
}

std::string RequestLine::get_http_version() {
    return _http_version;
}

void RequestLine::set_http_version(const std::string &v) {
    _http_version = v;
}

FileItem::FileItem() {
    _is_file = false;
    _parse_state = PARSE_MULTI_DISPOSITION;
}

bool FileItem::is_file() {
    return _is_file;
}

std::string *FileItem::get_fieldname() {
    return &_name;
}

std::string *FileItem::get_filename() {
    return &_filename;
}

std::string *FileItem::get_content_type() {
    return &_content_type;
}

std::string *FileItem::get_data() {
    return &_data;
}

void FileItem::set_is_file() {
    _is_file = true;
}

void FileItem::set_name(const std::string &name) {
    _name = name;    
}

void FileItem::set_filename(const std::string &filename) {
    _filename = filename;    
}

void FileItem::append_data(const char *c, size_t len) {
    _data.append(c, len);    
}

void FileItem::set_content_type(const char *c, int len) {
    _content_type.assign(c, len);    
}

bool FileItem::get_parse_state() {
    return _parse_state;
}

void FileItem::set_parse_state(int state) {
    _parse_state = state;
}

std::string RequestBody::get_param(std::string name) {
    return _req_params.get_param(name);
}

void RequestBody::get_params(const std::string &name, std::vector<std::string> &params) {
    return _req_params.get_params(name, params);
}

std::string *RequestBody::get_raw_string() {
    return &_raw_string;
}

RequestParam *RequestBody::get_req_params() {
    return &_req_params;
}

int RequestBody::parse_multi_params() {
    for (size_t i = 0; i < _file_items.size(); i++) {
        FileItem &item = _file_items[i];
        std::string *field_name = item.get_fieldname();
        std::string *field_value = item.get_data();
        _req_params.add_param_pair(*field_name, *field_value);
        LDebug("request body add param name:{}, data:{}", 
                field_name->c_str(), field_value->c_str());
    }
    return 0;
}

// parse multipart data like "Content-Type:application/json;"
int RequestBody::parse_json_params() {
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(_raw_string, root, false)) {
        return 1;
    }

    Json::Value::Members mems = root.getMemberNames();
    for (auto iter = mems.begin(); iter != mems.end(); ++iter) {
        auto& item = root[*iter];
        auto type = item.type();
        switch (type) {
            case Json::ValueType::objectValue:
            {
                break;
            }
            case Json::ValueType::arrayValue:
            {
                int cnt = item.size();
                std::string str;
                for (int i = 0; i < cnt; ++i) {
                    switch (item[i].type()) {
                    case Json::ValueType::stringValue:
                    case Json::ValueType::booleanValue:
                    case Json::ValueType::intValue:
                    case Json::ValueType::uintValue:
                    case Json::ValueType::realValue:
                        str.append(item[i].asString());
                        str.append(",");
                        break;
                    default:
                        break;
                    }
                }
                if (!str.empty()) {
                    str.pop_back();
                }
                _req_params.add_param_pair(*iter, str);
                break;
            }
            case Json::ValueType::stringValue:
            case Json::ValueType::booleanValue:
            case Json::ValueType::intValue:
            case Json::ValueType::uintValue:
            case Json::ValueType::realValue:
            {
                _req_params.add_param_pair(*iter, item.asString());
                break;
            }
            default: {
                 break;
            }
        }
    }

    return 0;
}

std::vector<FileItem> *RequestBody::get_file_items() {
    return &_file_items;
}

std::string Request::get_param(std::string name) {
    if (_line.get_method() == "GET") {
        return _line.get_request_param().get_param(name);
    }
    if (_line.get_method() == "POST") {
        return _body.get_param(name);
    }
    return "";
}

#define ishex(in) ((in >= 'a' && in <= 'f') || \
        (in >= 'A' && in <= 'F') || \
        (in >= '0' && in <= '9'))


int unescape(std::string &param, std::string &unescape_param) {
    int write_index = 0;
    for (unsigned i = 0; i < param.size(); i++) {
        if (('%' == param[i]) && ishex(param[i+1]) && ishex(param[i+2])) {
            std::string temp;
            temp += param[i+1];
            temp += param[i+2];
            char *ptr;
            unescape_param.append(1, (unsigned char) strtol(temp.c_str(), &ptr, 16));
            i += 2;
        } else if (param[i] == '+') {
            unescape_param.append(" ");
        } else {
            unescape_param.append(1, param[i]);
        }
        write_index++;
    }
    return 0;
}

std::string Request::get_unescape_param(std::string name) {
    std::string param = this->get_param(name);
    if (param.empty()) {
        return param;
    }
    std::string unescape_param;
    unescape(param, unescape_param);
    return unescape_param;
}

void Request::get_params(const std::string &name, std::vector<std::string> &params) {
    if (_line.get_method() == "GET") {
        _line.get_request_param().get_params(name, params);
    }
    if (_line.get_method() == "POST") {
        _body.get_params(name, params);
    }
}

void Request::add_header(std::string &name, std::string &value) {
    //LError("{}: {}", name.c_str(), value.c_str());
    ss_str_tolower(name); // field is case-insensitive
    this->_headers[name] = value;
}

void Request::add_cookie(std::string& name, std::string& value) {
    ss_str_trim(name);
    this->_cookies[name] = value;
}

const std::string& Request::get_header(std::string name) {
    ss_str_tolower(name); // field is case-insensitive
    return this->_headers[name];
}

const std::string& Request::get_cookie(std::string name) {
    return this->_cookies[name];
}

std::string Request::get_request_uri() {
    return _line.get_request_uri();
}

std::string Request::get_request_url() {
    return _line.get_request_url();
}

int ss_on_url(http_parser *p, const char *buf, size_t len) {
    Request *req = (Request *) p->data;
    std::string url;
    url.assign(buf, len);
    req->_line.append_request_url(url);
    return 0;
}

int ss_on_header_field(http_parser *p, const char *buf, size_t len) {
    Request *req = (Request *) p->data;
    if (req->_parse_part == PARSE_REQ_LINE) {
        req->_line.set_method(http_method_str((enum http_method)p->method));
        int ret = req->_line.parse_request_url_params();
        if (ret != 0) {
            return ret; 
        }
        if (p->http_major == 1 && p->http_minor == 0) {
            req->_line.set_http_version("HTTP/1.0");
        }
        if (p->http_major == 1 && p->http_minor == 1) {
            req->_line.set_http_version("HTTP/1.1");
        }
        req->_parse_part = PARSE_REQ_HEAD;
    }

    std::string field;
    field.assign(buf, len);
    if (req->_last_was_value) { 
        req->_header_fields.push_back(field);
        req->_last_was_value = false;
    } else {
        req->_header_fields[req->_header_fields.size() - 1] += field;
    }
    LDebug("GET field:{}", field.c_str());
    return 0;
}

int ss_on_header_value(http_parser *p, const char *buf, size_t len) {
    Request *req = (Request *) p->data;

    std::string value;
    value.assign(buf, len);
    if (!req->_last_was_value) {
        req->_header_values.push_back(value); 
    } else {
        req->_header_values[req->_header_values.size() - 1] += value; 
    } 
    LDebug("GET value:{}", value.c_str());
    req->_last_was_value = true;
    return 0;
}

int ss_on_headers_complete(http_parser *p) {
    Request *req = (Request *) p->data;
    if (req->_header_fields.size() != req->_header_values.size()) {
        LError("header field size:{} != value size:{}",
                req->_header_fields.size(), req->_header_values.size());
        return -1;
    }
    for (size_t i = 0; i < req->_header_fields.size(); i++) {
        req->add_header(req->_header_fields[i], req->_header_values[i]); 
    }

    // get and set real client ip
    std::string forwarded = req->get_header("X-Forwarded-For");
    req->set_client_real_ip(ss_str_trim(forwarded));
    if (!forwarded.empty()) {
        auto pos = forwarded.find_first_of(",");
        if (pos == std::string::npos) {
            req->set_client_real_ip(ss_str_trim(forwarded));
        }
        else {
            std::string real_ip = forwarded.substr(0, pos);
            req->set_client_real_ip(ss_str_trim(real_ip));
        }
    }

    // cookies
    auto cookies = req->get_header("Cookie");
    if (!cookies.empty()) {
        std::string::size_type last = 0;
        std::string::size_type index = cookies.find_first_of(";", last);
        while (index != std::string::npos) {
            std::string keyValue = cookies.substr(last, index - last);
            auto pos = keyValue.find_first_of("=");
            auto key = keyValue.substr(0, pos);
            auto value = keyValue.substr(pos + 1);
            req->add_cookie(key, value);
            last = index + 1;
            index = cookies.find_first_of(";", last);
        }
        if (index - last > 0) {
            std::string keyValue = cookies.substr(last, index - last);
            auto pos = keyValue.find_first_of("=");
            auto key = keyValue.substr(0, pos);
            auto value = keyValue.substr(pos + 1);
            req->add_cookie(key, value);
        }
    }

    req->_parse_part = PARSE_REQ_HEAD_OVER;
    LDebug("HEADERS COMPLETE! which field size:{}, value size:{}",
            req->_header_fields.size(), req->_header_values.size());
    if (req->get_method() == "POST" && req->get_header("Content-Length").empty()) {
        req->_parse_err = PARSE_LEN_REQUIRED;
        LError("parse req error, Content-Length not found");
        return -1;
    }
    return 0;
}

int ss_on_body(http_parser *p, const char *buf, size_t len) {
    Request *req = (Request *) p->data;
    req->get_body()->get_raw_string()->append(buf, len);
    req->_parse_part = PARSE_REQ_BODY;
    LDebug("GET body len:{}", len);
    return 0;
}

int ss_on_multipart_name(multipart_parser* p, const char *at, size_t length) {
    std::string s;
    s.assign(at, length);
    Request *req = (Request *)multipart_parser_get_data(p);
    std::vector<FileItem> *items = req->get_body()->get_file_items();
    if (s == "Content-Disposition") {
        FileItem item;
        item.set_parse_state(PARSE_MULTI_DISPOSITION);
        items->push_back(item);
    }
    if (s == "Content-Type") {
        CHECK_ERR(items->empty(), "items is empty! len:{}", length); 
        FileItem &item = (*items)[items->size() - 1];
        item.set_parse_state(PARSE_MULTI_CONTENT_TYPE);
    }
    LDebug("get multipart_name:{}", s.c_str());
    return 0;
}

int ss_parse_disposition_value(const std::string &input, std::string &output) {
    std::vector<std::string> name_tokens;
    ss_split_str(input, '=', name_tokens);
    if (name_tokens.size() != 2) {
        LError("ss_parse_multi_disposition_name(in name) err, input:{}", input.c_str());
        return -1;
    } 
    output = name_tokens[1];
    if (output == "\"\"") {
        LDebug("filename is empty, input:{}", input.c_str());
        output = "";
        return 0;
    }
    output = output.substr(1, output.size() - 2); // remove ""
    if (output.empty()) {
        LError("ss_parse_multi_disposition_name(in name) err, name is empty");
        return -1;
    }
    return 0;
}

// parse data like "form-data; name="files"; filename="test_thread_cancel.cc""
int ss_parse_multi_disposition(const std::string &input, FileItem &item) {
    std::vector<std::string> pos_tokens;
    ss_split_str(input, ';', pos_tokens);
    if (pos_tokens.size() < 2) {
        LError("ss_parse_multi_disposition_name err, input:{}", input.c_str());
        return -1;
    }
    std::string fieldname;
    int ret = ss_parse_disposition_value(pos_tokens[1], fieldname); 
    CHECK_RET(ret, "parse disposition fieldname error!, input:{}", input.c_str());
    item.set_name(fieldname);
    
    if (pos_tokens.size() >=3) {
        item.set_is_file();
        std::string filename;
        ret = ss_parse_disposition_value(pos_tokens[2], filename); 
        CHECK_RET(ret, "parse disposition filename error!, input:{}", input.c_str());
        item.set_filename(filename);
    }
    return 0;
}

int ss_on_multipart_value(multipart_parser* p, const char *at, size_t length) {
    std::string s;
    s.assign(at, length);
    Request *req = (Request *)multipart_parser_get_data(p);
    std::vector<FileItem> *items = req->get_body()->get_file_items();
    if (items->empty()) {
        LError("items is empty in parse multi value!");
        return -1;
    }
    FileItem &item = (*items)[items->size() - 1];
    if (item.get_parse_state() == PARSE_MULTI_DISPOSITION) {
        int ret = ss_parse_multi_disposition(s, item);
        CHECK_RET(ret, "parse multi dispostion err:{}, input:{}", ret, s.c_str());
    }
    if (item.get_parse_state() == PARSE_MULTI_CONTENT_TYPE) {
        item.set_content_type(at, length);
    }
    LDebug("get multipart_value:{}", s.c_str());
    return 0;
}

int ss_on_multipart_data(multipart_parser* p, const char *at, size_t length) {
    if (length == 0) {
        LDebug("multipart data is empty, len:{}", length);
        return 0;
    }
    
    Request *req = (Request *)multipart_parser_get_data(p);
    std::vector<FileItem> *items = req->get_body()->get_file_items();
    CHECK_ERR(items->empty(), "items is empty!, length:{}", length); 

    FileItem &item = (*items)[items->size() - 1];
    item.append_data(at, length);

    LDebug("on multipart data for name:{}, len:{}", 
            item.get_fieldname()->c_str(), length);
    return 0;
}

int ss_on_multipart_data_over(multipart_parser* p) {
    Request *req = (Request *)multipart_parser_get_data(p);
    std::vector<FileItem> *items = req->get_body()->get_file_items();
    CHECK_ERR(items->empty(), "items is empty!, items size:{}", items->size()); 
    
    FileItem &item = (*items)[items->size() - 1];
    item.set_parse_state(PARSE_MULTI_OVER); 
    return 0;
}

int ss_on_multipart_body_end(multipart_parser* p) {
    Request *req = (Request *)multipart_parser_get_data(p);
    LDebug("get multipart_body end, params size:{}", req->get_body()->get_file_items()->size());
    return req->get_body()->parse_multi_params();
}


// parse multipart data like "Content-Type:multipart/form-data; boundary=----GKCBY7qhFd3TrwA"
int ss_parse_multipart_data(Request *req) {
    std::string *body = req->get_body()->get_raw_string();
    if (body->empty()) {
        LDebug("multipart data is empty, url:{}", req->get_request_url().c_str());
        return 0;
    }
    // parse boundary
    std::string ct = req->get_header("Content-Type");
    std::vector<std::string> ct_tokens;
    int ret = ss_split_str(ct, ';', ct_tokens);
    if (ret != 0 || ct_tokens.size() != 2) {
        LError("parse multipart data content type err:{}, ct:{}", ret, ct.c_str());
        return ret;
    }
    std::vector<std::string> boundary_tokens;
    ret = ss_split_str(ct_tokens[1], '=', boundary_tokens);
    if (ret != 0 || boundary_tokens.size() != 2) {
        LError("parse multipart data(boundary) content type err:{}, ct:{}", ret, ct.c_str());
        return ret;
    }
    std::string boundary = "--" + boundary_tokens[1];
    LDebug("get url:{}, bounday:{}", req->get_request_url().c_str(), boundary.c_str());

    multipart_parser_settings mp_settings;
    memset(&mp_settings, 0, sizeof(multipart_parser_settings));
    mp_settings.on_header_field = ss_on_multipart_name;
    mp_settings.on_header_value = ss_on_multipart_value;
    mp_settings.on_part_data = ss_on_multipart_data;
    mp_settings.on_part_data_end = ss_on_multipart_data_over;
    mp_settings.on_body_end = ss_on_multipart_body_end;

    multipart_parser* parser = multipart_parser_init(boundary.c_str(), &mp_settings);
    multipart_parser_set_data(parser, req);
    size_t parsed = multipart_parser_execute(parser, body->c_str(), body->size());
    int parse_ret = 0;
    if (parsed != body->size()) {
        LWarn("parse multipart data err, parsed:{}, total:{}, url:{}", 
                parsed, body->size(), req->get_request_url().c_str());
        parse_ret = -1;
    }
    LDebug("multipart_parser_execute, parsed:{}, total:{}", 
                parsed, body->size());
    multipart_parser_free(parser);

    return parse_ret;
}

int ss_on_message_complete(http_parser *p) {
    Request *req = (Request *) p->data;
    std::string ct_header = req->get_header("Content-Type");
    // parse body params
    int parse_ret = 0;
    if (ct_header.find("application/x-www-form-urlencoded") != std::string::npos) {
        std::string *raw_str = req->get_body()->get_raw_string();
        if (raw_str->size()) {
            req->get_body()->get_req_params()->parse_query_url(*raw_str);
        }
    }
    else if (ct_header.find("application/json") != std::string::npos) {
        parse_ret = req->get_body()->parse_json_params();
        if (parse_ret != 0) {
            req->_parse_err = PARSE_APPLICATION_JSON_ERR;
        }
    }
    else if (ct_header.find("multipart/form-data") != std::string::npos) {
        LDebug("start parse multipart data! content type:{}", ct_header.c_str());
        parse_ret = ss_parse_multipart_data(req);
        if (parse_ret != 0) {
            req->_parse_err = PARSE_MULTIPART_ERR;
        }
    }
    req->_parse_part = PARSE_REQ_OVER;
    LDebug("msg COMPLETE! parse_err:{}", req->_parse_err);
    return parse_ret;
}

Request::Request() {
    _parse_part = PARSE_REQ_LINE;
    _total_req_size = 0;
    _last_was_value = true; // add new field for first
    _parse_err = 0;
    _client_ip = NULL;

    http_parser_settings_init(&_settings);
    _settings.on_url = ss_on_url;
    _settings.on_header_field = ss_on_header_field;
    _settings.on_header_value = ss_on_header_value;
    _settings.on_headers_complete = ss_on_headers_complete;
    _settings.on_body = ss_on_body;
    _settings.on_message_complete = ss_on_message_complete;

    http_parser_init(&_parser, HTTP_REQUEST);
    _parser.data = this;
}

Request::~Request() {}

int Request::parse_request(const char *read_buffer, int read_size) {
    _total_req_size += read_size;
    if (_total_req_size > MAX_REQ_SIZE) {
        LInfo("TOO BIG REQUEST WE WILL REFUSE IT! MAX_REQ_SIZE:{}", MAX_REQ_SIZE);
        return -1;
    }
    LTrace("read from client: size:{}, content:{}", read_size, read_buffer);
    ssize_t nparsed = http_parser_execute(&_parser, &_settings, read_buffer, read_size);
    if (nparsed != read_size) {
        std::string err_msg = "unkonw";
        if (_parser.http_errno) {
            err_msg = http_errno_description(HTTP_PARSER_ERRNO(&_parser));
        }
        LError("parse request error, nparsed:{}, input size:{}! msg:{}", 
                nparsed, read_size, err_msg.c_str());
        return -1;
    }

    if (_parse_err) {
        return _parse_err;
    }
    if (_parse_part != PARSE_REQ_OVER) {
        return NEED_MORE_STATUS;
    }
    return 0;
}

RequestBody *Request::get_body() {
    return &_body;
}

std::string Request::get_method() {
    return _line.get_method();
}

std::string *Request::get_client_ip() {
    return _client_ip;
}

const std::string& Request::get_real_ip() const {
    return _client_real_ip;
}

void Request::set_client_ip(std::string *ci) {
    this->_client_ip = ci;
}

void Request::set_client_real_ip(const std::string& ip) {
    this->_client_real_ip = ip;
}

Response::Response(CodeMsg status_code) {
    this->_code_msg = status_code;
    this->_is_writed = 0;
}

void Response::set_head(const std::string &name, const std::string &value) {
    this->_headers[name] = value;
}

void Response::set_body(Json::Value &body) {
    Json::FastWriter writer;
    std::string str_value = writer.write(body);
    this->_body = str_value;
}

int Response::gen_response(const std::string &http_version, bool is_keepalive) {
    LDebug("START gen_response code:{}, msg:{}", _code_msg.status_code, _code_msg.msg.c_str());
    _res_bytes << http_version << " " << _code_msg.status_code << " " << _code_msg.msg << "\r\n";
    _res_bytes << "Server: ehttp/" << EHTTP_VERSION << "\r\n";
    if (_headers.find("Content-Type") == _headers.end()) {
        _res_bytes << "Content-Type: application/json; charset=UTF-8" << "\r\n";
    }
    if (_headers.find("Content-Length") == _headers.end()) {
        _res_bytes << "Content-Length: " << _body.size() << "\r\n";
    }

    std::string con_status = "Connection: close";
    if(is_keepalive) {
        con_status = "Connection: Keep-Alive";
    }
    _res_bytes << con_status << "\r\n";

    for (std::map<std::string, std::string>::iterator it = _headers.begin(); it != _headers.end(); ++it) {
        _res_bytes << it->first << ": " << it->second << "\r\n";
    }
    // header end
    _res_bytes << "\r\n";
    _res_bytes << _body;

    LDebug("gen response context:{}", _res_bytes.str().c_str());
    return 0;
}

int Response::readsome(char *buffer, int buffer_size, int &read_size) {
    _res_bytes.read(buffer, buffer_size);
    read_size = _res_bytes.gcount();

    if (!_res_bytes.eof()) {
        return NEED_MORE_STATUS;
    }
    return 0;
}

int Response::rollback(int num) {
    if (_res_bytes.eof()) {
        _res_bytes.clear();
    }
    int rb_pos = (int) _res_bytes.tellg() - num;
    _res_bytes.seekg(rb_pos);
    return _res_bytes.good() ? 0 : -1;
}

HttpContext::HttpContext(int fd) {
    this->_fd = fd;
    _req = new Request();
    _res = new Response();
}

HttpContext::~HttpContext() {
    delete_req_res();
}

int HttpContext::record_start_time() {
    gettimeofday(&_start, NULL);
    return 0;
}

int HttpContext::get_cost_time() {
    timeval end;
    gettimeofday(&end, NULL);
    int cost_time = (end.tv_sec - _start.tv_sec) * 1000000 + (end.tv_usec - _start.tv_usec);
    return cost_time;
}

int HttpContext::print_access_log(const std::string &client_ip) {
    std::string http_method = this->_req->_line.get_method();
    std::string request_url = this->_req->_line.get_request_url();
    if (http_method.empty() || request_url.empty()) {
        LError("http method or request url is empty!");
        return -1;
    }
    int cost_time = get_cost_time();
    //LInfo("access_log {} {} status_code:{} cost_time:{} us, body_size:{}, client_ip:{}",
    //        http_method.c_str(), request_url.c_str(), _res->_code_msg.status_code,
    //        cost_time, _res->_body.size(), client_ip.c_str());
    return 0;
}

inline void HttpContext::delete_req_res() {
    if (_req != NULL) {
        delete _req;
        _req = NULL;
    }
    if (_res != NULL) {
        delete _res;
        _res = NULL;
    }
}

void HttpContext::clear() {
    delete_req_res();
    _req = new Request();
    _res = new Response();
}

Response &HttpContext::get_res() {
    return *_res;
}

Request &HttpContext::get_request() {
    return *_req;
}
