// Copyright 2022 Cartesi Pte. Ltd.

// SPDX-License-Identifier: Apache-2.0
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use
// this file except in compliance with the License. You may obtain a copy of the
// License at http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.


#include <stdio.h>
#include <stdlib.h>

// 3rdparty libs
#include "3rdparty/mongoose/mongoose.h"
#include "3rdparty/cJSON/cjson.h"


#define FINISH_STATUS_SZ 32
#define ETH_ADDRESS_SZ 42
#define MAX_ENDPOINT_SZ 16
#define NOTICE_SZ 1000

char s_post_body[NOTICE_SZ+1];

char *aux;
char *rollups_url;
char rollup_address[ETH_ADDRESS_SZ+1];

struct mg_mgr mgr;  // Event management structure
int status_code;    // HTTP status code


// HTTP event handler function
static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
    if (ev == MG_EV_CONNECT) {
        // Connected to server. Extract host name from URL
        struct mg_str host = mg_url_host(rollups_url);
    
        // Send request
        int content_length = s_post_body ? strlen(s_post_body) : 0;
        mg_printf(c,
                "%s %s HTTP/1.0\r\n"
                "Host: %.*s\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: %d\r\n"
                "\r\n",
                s_post_body ? "POST" : "GET", mg_url_uri(rollups_url), (int) host.len,
                host.ptr, content_length);
        mg_send(c, s_post_body, content_length);
    
    } else if (ev == MG_EV_HTTP_MSG) {
        // Response is received. Get status-code and body
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        status_code = mg_http_status(hm);

        strncpy(s_post_body, hm->message.ptr, (int) hm->message.len);      
        
        c->is_closing = 1;         // Tell mongoose to close this connection
        *(bool *) fn_data = true;  // Tell event loop to stop
    
    } else if (ev == MG_EV_ERROR) {
        *(bool *) fn_data = true;  // Error, tell event loop to stop
    }
}

void do_request(const char *uri, const char *req_body) {
    bool done = false;                              // connection done?
    strcpy(s_post_body, req_body);

    mg_mgr_init(&mgr);                              // Init manager

    strcat(rollups_url, uri);                  // build url (url+uri)

    mg_http_connect(&mgr, rollups_url, fn, &done);  // Create client connection
    while (!done) mg_mgr_poll(&mgr, 50);            // connection loop
    mg_mgr_free(&mgr);                              // release resources

    *aux = '\0';                                    // clear uri
    s_post_body[0] = '\0';                          // clear req body
}

char *handle_advance(cJSON *data) {

}

char *handle_inspect(cJSON *data) {

}




int main() {
    // get rollups url
    aux = getenv("ROLLUP_HTTP_SERVER_URL");
    char s[strlen(aux)+MAX_ENDPOINT_SZ+1];
    strcpy(s, aux);
    rollups_url = s;
    aux = rollups_url+strlen(aux);
    char finish_status[FINISH_STATUS_SZ+1];
    strcpy(finish_status, "accept")
    
    while (1) {
        sprintf("{\"status\": \"%s\"}", finish_status);
        do_request("/finish", finish_status);

        if (status_code == 202) {
            printf("No pending rollup request, trying again\n");
        } else {
            cJSON *json = cJSON_Parse(s_post_body);
            cJSON *data = cJSON_GetObjectItemCaseSensitive(json, "data");
            cJSON *metadata = cJSON_GetObjectItemCaseSensitive(data, "metadata");
            if (metadata) {
                cJSON *epoch_index = cJSON_GetObjectItemCaseSensitive(metadata, "epoch_index");
                cJSON *input_index = cJSON_GetObjectItemCaseSensitive(metadata, "input_index");

                int epoch_index = atoi(cJSON_Print(epoch_index));
                int input_index = atoi(cJSON_Print(input_index));

                if (epoch_index == 0 && input_index == 0) {
                    cJSON *msg_sender = JSON_GetObjectItemCaseSensitive(metadata, "msg_sender");
                    char *msg_sender_str = cJSON_Print(json);
                    
                    strncpy(rollup_address, msg_sender_str, ETH_ADDRESS_SZ);
                    printf("Captured rollup address: %s\n", rollup_address);
                    continue;
                }

                cJSON *handler = JSON_GetObjectItemCaseSensitive(json, "request_type");
                char *handler_str = cJSON_Print(handler);

                cJSON *data = JSON_GetObjectItemCaseSensitive(json, "data");

                if (strcmp(handler_str, "/advance") == 0) {
                    strcpy(finish_status, handle_advance(data));
                } else if (strcmp(handler_str, "/inspect") == 0) {
                    strcpy(finish_status, handle_inspect(data));
                } else {
                    printf("Invalid request_type!\n");
                    strcpy(finish_status, "reject");
                }
            }
        }
    }
}