#ifndef ROLES_WS_H
#define ROLES_WS_H

#include <stddef.h>

const char *ws_role_name(void);
const char *ws_alpn_id(void);
int ws_is_upgrade_request(const char *request);
int ws_build_handshake_response(const char *request, char *out,
                                size_t out_len);

#endif
