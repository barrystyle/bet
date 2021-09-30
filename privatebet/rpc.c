#include "bet.h"
#include "rpc.h"

#include <curl.h>

char rpc_addrstr[128][8];
char rpc_authstr[128][8];

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

void rpc_init(const char* rpc_user, const char* rpc_pass, const char* rpc_ipaddr, const char* rpc_port, int instance)
{
    memset(rpc_addrstr[instance], 0, sizeof(rpc_addrstr[instance]));
    memset(rpc_authstr[instance], 0, sizeof(rpc_authstr[instance]));
    sprintf(rpc_addrstr[instance], "http://%s:%s/", rpc_ipaddr, rpc_port);
    sprintf(rpc_authstr[instance], "%s:%s", rpc_user, rpc_pass);
}

bool rpc_parseconfig(int instance)
{
    //! set config path depending on instance
    char instance_path[128];
    memset(instance_path, 0, sizeof(instance_path));
    if (instance == CHIPS)
        sprintf(instance_path, "config/chips.conf");
    else if (instance == LIGHTNING)
        sprintf(instance_path, "config/lightning.conf");
    else
        return false;

    //! read config file
    dictionary *ini = NULL;
    ini = iniparser_load(instance_path);
    if (!ini) {
        dlg_info("could not read %s\n", instance_path);
        exit(-1);
    }

    //! collect required rpc details
    const char *rpc_user = iniparser_getstring(ini, ":rpcuser", NULL);
    const char *rpc_pass = iniparser_getstring(ini, ":rpcpassword", NULL);
    const char *rpc_ipaddr = iniparser_getstring(ini, ":rpcbind", NULL);
    const char *rpc_port = iniparser_getstring(ini, ":rpcport", NULL);
    if (!rpc_user || !rpc_pass || !rpc_ipaddr || !rpc_port) {
        dlg_info("%s missing rpc details\n", instance_path);
        exit(-1);
    }
    rpc_init(rpc_user, rpc_pass, rpc_ipaddr, rpc_port, instance);
    dlg_info("%s describes daemon @ %s (%s)\n", instance_path, rpc_addrstr[instance], rpc_authstr[instance]);

    //! attempt basic comms with daemon
    char query[128], response[128];
    memset(query, 0, sizeof(query));
    memset(response, 0, sizeof(response));
    sprintf(query, "getblockcount");

    //! return errorlevel
    rpc_perform(query, NULL, response, instance);
    if (strlen(response) > 0) {
        return true;
    }

    return false;
}

void rpc_perform(const char* rpc_method, const char* rpc_param, char* rpc_response, int instance)
{
    CURL* curl = curl_easy_init();
    struct curl_slist* headers = NULL;

    if (curl) {

        //! allocate memory for response
        struct MemoryStruct chunk;
        chunk.size = 0;
        chunk.memory = malloc(65536);

        //! allocate and set our request
        char payload[1024];
        memset(payload, 0, sizeof(payload));

        if (rpc_param == NULL)
            sprintf(payload, "{\"jsonrpc\": \"1.0\", \"id\": null, \"method\": \"%s\", \"params\": []}", rpc_method);
        else
            sprintf(payload, "{\"jsonrpc\": \"1.0\", \"id\": null, \"method\": \"%s\", \"params\": [%s]}", rpc_method, rpc_param);

        //! configure curl object and run request
        headers = curl_slist_append(headers, "content-type: application/json;");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_URL, rpc_addrstr[instance]);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(payload));
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
        curl_easy_setopt(curl, CURLOPT_USERPWD, rpc_authstr[instance]);
        curl_easy_setopt(curl, CURLOPT_USE_SSL, false);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
        curl_easy_perform(curl);

        if (chunk.size) {
            memcpy(rpc_response, chunk.memory, chunk.size);
        }

        curl_easy_cleanup(curl);
        free(chunk.memory);
    }

    return;
}
