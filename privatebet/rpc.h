#ifndef RPC_H
#define RPC_H

void rpc_init(const char* rpc_user, const char* rpc_pass, const char* rpc_ipaddr, const char* rpc_port);
bool rpc_perform(const char* rpc_method, const char* rpc_param, char* rpc_response);

#endif // RPC_H