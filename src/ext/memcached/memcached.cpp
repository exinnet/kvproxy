#include "memcached.h"

PARSE_REQ(memcached){
    int ret;
    req_t req_item;
    bool is_agin = false; 
    bool is_binary;
    static bool init_use_binary = false;
    static int use_binary = true;
    string str_proto;
    memcached_header header;
    int header_len = 0;
    
    if(init_use_binary == false){
        str_proto = get_conf("memcached", "proto");
        if(str_proto == "text"){
            use_binary = false;
        }
        init_use_binary = true;
    }

    is_binary = is_binary_protocol(origin_req, size);
    if(use_binary != is_binary){
        return ERROR;
    }
    if(is_binary == true){
        header_len = 24;
    }

    while(size-*position > 0){
        is_agin = false;
        if(is_binary){
            ret = try_read_request_binary(origin_req+*position, size-*position, &header);
        }else{
            ret = try_read_request_text(origin_req+*position, size-*position, &header);
        }
        if (ret == -1){
            return ERROR;
        }else if(ret == 0){
            return AGAIN;
        }
        
        req_item.each = false;
        req_item.update = false;
        req_item.ext[0] = '\0';

        if (header.opcode == PROTOCOL_BINARY_CMD_SET){
            req_item.update = true;
        }else if (header.opcode == PROTOCOL_BINARY_CMD_GET){
            req_item.update = false;
        }else if (header.opcode == PROTOCOL_BINARY_CMD_GETKQ){
            req_item.update = false;
            is_agin = true;
        }else if(header.opcode == PROTOCOL_BINARY_CMD_NOOP){
            req_item.each = true;
            req_item.ext[0] = 'e';
        }else if (header.opcode == PROTOCOL_BINARY_CMD_ADD){
            req_item.update = true;
        }else if (header.opcode == PROTOCOL_BINARY_CMD_DELETE){
            req_item.update = true;
        }else if (header.opcode == PROTOCOL_BINARY_CMD_INCREMENT){
            req_item.update = true;
        }else if (header.opcode == PROTOCOL_BINARY_CMD_DECREMENT){
            req_item.update = true;
        }else if (header.opcode == PROTOCOL_BINARY_CMD_APPEND){
            req_item.update = true; 
        }else if (header.opcode == PROTOCOL_BINARY_CMD_PREPEND){
            req_item.update = true;
        }else if (header.opcode == PROTOCOL_BINARY_CMD_REPLACE){
            req_item.update = true;
        }else if(header.opcode == PROTOCOL_BINARY_CMD_QUIT){
            return IGNORE;
        }else{
            return ERROR;
        }
    
        if (size < header_len + header.bodylen){
            return AGAIN;
        }

        req_item.key = origin_req + (*position) + header_len + header.extlen;
        req_item.key_len = header.keylen;
        req_item.content = origin_req + (*position);
        req_item.content_len = header_len + header.bodylen;
        (*req_data).push_back(req_item);
        *position = (*position) + header_len + header.bodylen;
        if(is_agin != true){
            return SUCCESS;
        }
    }

    if(is_agin == true){
        return AGAIN;
    }else{
        return SUCCESS;
    }
}

CREATE_REQ(memcached){
    req_ptr_list_t::iterator it;
    const char * end_str = NULL;
    int end_len = 0;
    for (it = req_data->begin(); it != req_data->end(); it++){
        if((*it)->ext[0] != 'e'){
            memcpy(req_data_to_backend + (*length), (*it)->content, (*it)->content_len);
            *length += (*it)->content_len;
        }else{
            end_str = (*it)->content;
            end_len = (*it)->content_len;    
        }
    }
    if(end_len != 0){
        memcpy(req_data_to_backend + (*length), end_str, end_len);
        *length += end_len;
    }
    return SUCCESS;
}

CREATE_REQ_ASYNC(memcached){
    comm_list_t::iterator it;
    const char * end_str = NULL;
    int end_len = 0;
    for (it = req_data->begin(); it != req_data->end(); it++){
        if(it->ext[0] != 'e'){
            memcpy(req_data_to_backend + (*length), it->content.c_str(), it->content.size());
            *length += it->content.size();
        }else{
            end_str = it->content.c_str();
            end_len = it->content.size(); 
        }
    }
    if(end_len != 0){
        memcpy(req_data_to_backend + (*length), end_str, end_len);
        *length += end_len;
    }
    return SUCCESS;
}

PARSE_RESP(memcached){
    int ret;
    bool is_agin;
    bool is_binary;
    resp_t item;
    memcached_header header;
    int header_len  = 0;
    
    is_binary = is_binary_protocol(origin_resp, size);
    if(is_binary == true){
        header_len = 24;
    }

    while(size - *position > 0){
        is_agin = false;
        if(is_binary == true){
            ret = try_read_response_binary(origin_resp+*position, size-*position, &header);
        }else{
            ret = try_read_response_text(origin_resp+*position, size-*position, &header);
        }
        if (ret == -1){
            return ERROR;
        }else if(ret == 0){
            return AGAIN;
        }
        if (size < header_len + header.bodylen){
            return AGAIN;
        }
        item.ext[0] = '\0';
        item.content = origin_resp + (*position);
        item.content_len = header_len + header.bodylen;
        if(header.opcode == PROTOCOL_BINARY_CMD_GETKQ){
            is_agin = true;
        }else if(header.opcode == PROTOCOL_BINARY_CMD_NOOP){
            item.ext[0] = 'e';
        }
        (*resp_data).push_back(item);
        *position = (*position) + header_len + header.bodylen;
    }
    if(is_agin == true){
        return AGAIN;
    }else{
        return SUCCESS;
    }
}

CREATE_RESP(memcached){
    resp_list_t::iterator it;
    const char * end_str = NULL;
    int end_len = 0;

    for(it = resp_data->begin(); it != resp_data->end(); it++){
        if(it->ext[0] != 'e'){
            if((*length) + it->content_len > *size){
                return ERROR;
            }
            memcpy(resp_data_to_client + (*length), it->content, it->content_len);
            *length += it->content_len;
        }else{
            end_str = it->content;
            end_len = it->content_len;    
        }
    }
    
    if(end_len != 0){
        if((*length) + end_len > *size){
            return ERROR;
        }
        memcpy(resp_data_to_client + (*length), end_str, end_len);
        *length += end_len;
    }
    return SUCCESS;
}

EXT_VERSION(memcached){
    return EXT_VER;
}

bool is_binary_protocol(const char * data, int size){
    if((uint8_t)*data == PROTOCOL_BINARY_REQ || (uint8_t)*data == PROTOCOL_BINARY_RES){
        return true;
    }else{
        return false;
    }
}

int try_read_request_binary(const char * req_data, int size, memcached_header * header){
    if (size < 24){
        return 0;
    }
    protocol_binary_request_header* req;
    if ((unsigned char)*req_data != (unsigned char)PROTOCOL_BINARY_REQ) {
        return -1;
    }
    req = (protocol_binary_request_header*)req_data;
    header->keylen = ntohs(req->request.keylen);
    header->bodylen = ntohl(req->request.bodylen);
    header->extlen = (uint8_t)req->request.extlen;
    header->opcode = req->request.opcode;
    return 1;
}

int try_read_response_binary(const char * resp_data, int size, memcached_header * header){
    if (size < 24){
        return 0;
    }
    protocol_binary_response_header * resp;
    if ((unsigned char)*resp_data != (unsigned char)PROTOCOL_BINARY_RES) {
        return -1;
    }
    resp = (protocol_binary_response_header*)resp_data;
    header->keylen = ntohs(resp->response.keylen);
    header->bodylen = ntohl(resp->response.bodylen);
    header->extlen = (uint8_t)resp->response.extlen;
    header->opcode = resp->response.opcode;
    return 1;
}

int try_read_request_text(const char * req_data, int size, memcached_header * header){
    int i;
    int split_counter = 0;
    bool have_noreply = false;
    int cmd_len = 0;
    int key_len = 0;
    int content_len = 0;
    int data_len  = 0;
    int opcode;
    int chr_data_len_i = 0;
    char chr_data_len[10];

    for(i = 0; i < size; i++){
        if(cmd_len == 0 && *(req_data+i) == ' '){
           cmd_len = i;
           header->extlen = cmd_len + 1;
           split_counter++;
        }else if(cmd_len > 0 && key_len == 0 && *(req_data+i) == ' '){
            key_len = i - header->extlen;
            split_counter++;
        }else if(cmd_len > 0 && key_len == 0 && *(req_data+i) == '\r'){
            key_len = i - header->extlen;
        }else if(cmd_len > 0 && key_len > 0 && *(req_data+i) == '\n'){
            content_len = i+1;
            break;
        }else if(key_len > 0 && *(req_data+i) == 'n'){
            have_noreply = true;
            break;
        }else if(key_len > 0 && *(req_data+i) == ' ' && split_counter < 4){
            split_counter++;
        }else if(split_counter == 4 && isdigit(*(req_data+i)) && chr_data_len_i < 10){
            chr_data_len[chr_data_len_i] = *(req_data+i);
            chr_data_len_i++;
        }
    }
    chr_data_len[chr_data_len_i] = '\0';
    data_len = atoi(chr_data_len);
    header->opcode = get_opcode(req_data);
    header->keylen = key_len;
    if(data_len > 0){
        header->bodylen = content_len + data_len + 2;
    }else{
        header->bodylen = content_len + data_len;
    }
    return 1;
}

int try_read_response_text(const char * resp_data, int size, memcached_header * header){
    int value_len = 0;
    int content_len = 0;
    int body_len = 0;
    int i;
    int split_counter = 0;
    int chr_data_len_i = 0;
    char chr_data_len[10];

    if(size < 5 || strncmp(resp_data, "VALUE", 5) != 0){
        header->bodylen = size;
        header->opcode = 0;
        return 1;
    }
    for(i = 0; i < size; i++){
        if(*(resp_data+i) == ' '){
            split_counter++;
        }else if(*(resp_data+i) == '\r'){
            content_len = i+2;
            break;
        }else if(split_counter == 3 && isdigit(*(resp_data+i)) && chr_data_len_i < 10){
            chr_data_len[chr_data_len_i] = *(resp_data+i);
            chr_data_len_i++;
        }
    }
    chr_data_len[chr_data_len_i] = '\0';
    value_len = atoi(chr_data_len);
    body_len = content_len + value_len + 7;
    if(size < body_len){
        return 0;
    }
    header->bodylen = body_len;
    header->opcode = 0;
    return 1;
}

uint32_t get_opcode(const char * data){
    if(strncmp(data, "get", 3) == 0){
        return PROTOCOL_BINARY_CMD_GET;
    }else if(strncmp(data, "set", 3) == 0){
        return PROTOCOL_BINARY_CMD_SET;
    }else if(strncmp(data, "add", 3) == 0){
        return PROTOCOL_BINARY_CMD_ADD;
    }else if(strncmp(data, "replace", 7) == 0){
        return PROTOCOL_BINARY_CMD_REPLACE;
    }else if(strncmp(data, "delete", 6) == 0){
        return PROTOCOL_BINARY_CMD_DELETE;
    }else if(strncmp(data, "incr", 4) == 0){
        return PROTOCOL_BINARY_CMD_INCREMENT;
    }else if(strncmp(data, "decr", 4) == 0){
        return PROTOCOL_BINARY_CMD_DECREMENT;
    }else if(strncmp(data, "quit", 4) == 0){
        return PROTOCOL_BINARY_CMD_QUIT;
    }else if(strncmp(data, "append", 6) == 0){
        return PROTOCOL_BINARY_CMD_APPEND;
    }else if(strncmp(data, "prepend", 7) == 0){
        return PROTOCOL_BINARY_CMD_PREPEND;
    }else{
        return 0;
    }
}
