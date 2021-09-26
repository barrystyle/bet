#include "bet.h"

#include <curl.h>

bool curl_init = false;
char rpc_addrstr[128];
char rpc_authstr[128];

struct MemoryStruct {
    char* memory;
    size_t size;
};

static size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct* mem = (struct MemoryStruct*)userp;

    char* ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

void rpc_init(const char* rpc_user, const char* rpc_pass, const char* rpc_ipaddr, const char* rpc_port)
{
    memset(rpc_addrstr, 0, sizeof(rpc_addrstr));
    memset(rpc_authstr, 0, sizeof(rpc_authstr));
    sprintf(rpc_addrstr, "http://%s:%s/", rpc_ipaddr, rpc_port);
    sprintf(rpc_authstr, "%s:%s", rpc_user, rpc_pass);
    curl_init = true;
}

bool rpc_perform(const char* rpc_method, const char* rpc_param, char* rpc_response)
{
    CURL* curl = curl_easy_init();
    struct curl_slist* headers = NULL;

    if (curl && curl_init) {

        //! allocate memory for response
        struct MemoryStruct chunk;
        chunk.size = 0;
        chunk.memory = malloc(65536);

        //! allocate and set our request
        char payload[1024];
        memset(payload, 0, sizeof(payload));
        sprintf(payload, "{\"jsonrpc\": \"1.0\", \"id\": null, \"method\": \"%s\", \"params\": [\"%s\"]}", rpc_method, rpc_param);

        //! configure curl object and run request
        headers = curl_slist_append(headers, "content-type: application/json;");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_URL, rpc_addrstr);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(payload));
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
        curl_easy_setopt(curl, CURLOPT_USERPWD, rpc_authstr);
        curl_easy_setopt(curl, CURLOPT_USE_SSL, false);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
        curl_easy_perform(curl);

        //! check if we got data
        bool success = false;
        if (chunk.size) {
            memcpy(rpc_response, chunk.memory, chunk.size);
            success = true;
        }

        curl_easy_cleanup(curl);
        free(chunk.memory);

        return success;
    }

    return false;
}
