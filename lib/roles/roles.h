#ifndef ROLES_ROLES_H
#define ROLES_ROLES_H

typedef enum {
    ROLE_HTTP1,
    ROLE_HTTP2,
    ROLE_HTTP3,
    ROLE_WS,
    ROLE_UNKNOWN
} Role;

const char *role_to_string(Role role);
Role role_from_string(const char *value);

#endif
