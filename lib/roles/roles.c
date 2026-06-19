#include "roles.h"

#include "http1.h"
#include "http2.h"
#include "http3.h"
#include "ws.h"

#include <string.h>

const char *role_to_string(Role role)
{
    switch (role) {
    case ROLE_HTTP1:
        return http1_role_name();
    case ROLE_HTTP2:
        return http2_role_name();
    case ROLE_HTTP3:
        return http3_role_name();
    case ROLE_WS:
        return ws_role_name();
    case ROLE_UNKNOWN:
    default:
        return "unknown";
    }
}

Role role_from_string(const char *value)
{
    if (!value) {
        return ROLE_UNKNOWN;
    }

    if (strcmp(value, http1_role_name()) == 0) {
        return ROLE_HTTP1;
    }

    if (strcmp(value, http2_role_name()) == 0) {
        return ROLE_HTTP2;
    }

    if (strcmp(value, http3_role_name()) == 0) {
        return ROLE_HTTP3;
    }

    if (strcmp(value, ws_role_name()) == 0) {
        return ROLE_WS;
    }

    return ROLE_UNKNOWN;
}
