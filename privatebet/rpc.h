#ifndef RPC_H
#define RPC_H

enum daemon_type {
    CHIPS,
    LIGHTNING
};

bool rpc_parseconfig(int instance);
void rpc_init(const char* rpc_user, const char* rpc_pass, const char* rpc_ipaddr, const char* rpc_port, int instance);
void rpc_perform(const char* rpc_method, const char* rpc_param, char* rpc_response, int instance);

#endif // RPC_H
