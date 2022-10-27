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
#include "3rdparty/cJSON/cJSON.h"


#define FINISH_STATUS_SZ 32
#define ETH_ADDRESS_SZ 42
#define MAX_ENDPOINT_SZ 16
#define MAX_NOTICE_SZ 64000
#define RES_BUFFER 1000

char req_body[MAX_NOTICE_SZ+1];
char res_body[RES_BUFFER+1];

char *url_end;
char *rollups_url;
char rollup_address[ETH_ADDRESS_SZ+1];

int status_code = 0;    // HTTP response status code


// HTTP event handler function
static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
    if (ev == MG_EV_CONNECT) {
        // Connected to server. Extract host name from URL
        struct mg_str host = mg_url_host(rollups_url);
    
        // Send request
        int content_length = req_body ? strlen(req_body) : 0;
        mg_printf(c,
                "%s %s HTTP/1.0\r\n"
                "Host: %.*s\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: %d\r\n"
                "\r\n",
                req_body ? "POST" : "GET", mg_url_uri(rollups_url), (int) host.len,
                host.ptr, content_length);
        mg_send(c, req_body, content_length);
    
    } else if (ev == MG_EV_HTTP_MSG) {
        // Response is received. Get status-code and body
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        status_code = mg_http_status(hm);

        strncpy(res_body, hm->body.ptr, RES_BUFFER);
        
        c->is_closing = 1;         // Tell mongoose to close this connection
        *(bool *) fn_data = true;  // Tell event loop to stop
    
    } else if (ev == MG_EV_ERROR) {
        *(bool *) fn_data = true;  // Error, tell event loop to stop
    }
}

void do_request(const char *uri) {
    struct mg_mgr mgr;                              // Event management structure
    bool done = false;                              // connection done?

    mg_mgr_init(&mgr);                              // Init manager

    strcat(rollups_url, uri);                       // build url (url+uri)

    mg_http_connect(&mgr, rollups_url, fn, &done);  // Create client connection
    while (!done) mg_mgr_poll(&mgr, 50);            // connection loop
    mg_mgr_free(&mgr);                              // release resources

    *url_end = '\0';                                // clear uri
    req_body[0] = '\0';                             // clear req body
}

char *handle_advance(cJSON *data) {
    char *data_str = cJSON_Print(data);

    printf("Received advance request data %s\n", data_str);
    printf("Adding notice\n");

    cJSON *payload = cJSON_GetObjectItemCaseSensitive(data, "payload");
    char *payload_str = cJSON_Print(payload);
    
    sprintf(req_body, "{\"payload\": %s}", payload_str);
    do_request("/notice");

    printf("Received notice status %d body %s\n", status_code, res_body);
    return "accept";
}

char *handle_inspect(cJSON *data) {
    char *data_str = cJSON_Print(data);

    printf("Received inspect request data %s\n", data_str);
    printf("Adding report\n");

    cJSON *payload = cJSON_GetObjectItemCaseSensitive(data, "payload");
    char *payload_str = cJSON_Print(payload);
    
    sprintf(req_body, "{\"payload\": %s}", payload_str);
    do_request("/report");

    printf("Received report status %d body %s\n", status_code, res_body);
    return "accept";
}




int main() {
    // get rollups url
    char *aux = getenv("ROLLUP_HTTP_SERVER_URL");
    char s[strlen(aux)+MAX_ENDPOINT_SZ+1];
    strcpy(s, aux);
    rollups_url = s;
    url_end = rollups_url+strlen(aux);
    char finish_status[FINISH_STATUS_SZ+1];
    strcpy(finish_status, "accept");
    
    while (1) {
        sprintf(req_body, "{\"status\": \"%s\"}", finish_status);
        printf("Sending finish\n");
        do_request("/finish");

        if (status_code == 202) {
            printf("No pending rollup request, trying again\n");
        } else {
            cJSON *json = cJSON_Parse(res_body);
            cJSON *data = cJSON_GetObjectItemCaseSensitive(json, "data");
            cJSON *metadata = cJSON_GetObjectItemCaseSensitive(data, "metadata");

            if (metadata) {
                cJSON *epoch_index = cJSON_GetObjectItemCaseSensitive(metadata, "epoch_index");
                cJSON *input_index = cJSON_GetObjectItemCaseSensitive(metadata, "input_index");

                int epoch_index_int = atoi(cJSON_Print(epoch_index));
                int input_index_int = atoi(cJSON_Print(input_index));

                if (epoch_index_int == 0 && input_index_int == 0) {
                    cJSON *msg_sender = cJSON_GetObjectItemCaseSensitive(metadata, "msg_sender");
                    char *msg_sender_str = cJSON_Print(msg_sender);
                    
                    // remove ""
                    msg_sender_str[strlen(msg_sender_str)-2] = '\0';
                    msg_sender_str++;

                    strncpy(rollup_address, msg_sender_str, ETH_ADDRESS_SZ);
                    printf("Captured rollup address: %s\n", rollup_address);
                    continue;
                }
            }

            cJSON *request_type = cJSON_GetObjectItemCaseSensitive(json, "request_type");
            char *request_type_str = cJSON_Print(request_type);

            if (strcmp(request_type_str, "\"advance_state\"") == 0) {
                strcpy(finish_status, handle_advance(data));
            } else if (strcmp(request_type_str, "\"inspect_state\"") == 0) {
                strcpy(finish_status, handle_inspect(data));
            }
        }
    }
}