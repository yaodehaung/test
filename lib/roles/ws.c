#include "ws.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define WS_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

typedef struct {
    uint32_t state[5];
    uint64_t bit_count;
    unsigned char buffer[64];
} Sha1Ctx;

static uint32_t rol32(uint32_t value, int bits)
{
    return (value << bits) | (value >> (32 - bits));
}

static int ascii_tolower(int c)
{
    if (c >= 'A' && c <= 'Z') {
        return c + ('a' - 'A');
    }
    return c;
}

static int ascii_strncasecmp(const char *a, const char *b, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        int ca = ascii_tolower((unsigned char)a[i]);
        int cb = ascii_tolower((unsigned char)b[i]);

        if (ca != cb || ca == '\0') {
            return ca - cb;
        }
    }
    return 0;
}

static const char *find_header(const char *request, const char *name)
{
    size_t name_len = strlen(name);
    const char *line = strstr(request, "\r\n");

    if (!line) {
        return NULL;
    }
    line += 2;

    while (*line && line[0] != '\r' && line[1] != '\n') {
        const char *line_end = strstr(line, "\r\n");

        if (!line_end) {
            return NULL;
        }

        if ((size_t)(line_end - line) > name_len &&
            ascii_strncasecmp(line, name, name_len) == 0 &&
            line[name_len] == ':') {
            const char *value = line + name_len + 1;

            while (*value == ' ' || *value == '\t') {
                value++;
            }
            return value;
        }

        line = line_end + 2;
    }

    return NULL;
}

static int header_contains_token(const char *request, const char *name,
                                 const char *token)
{
    const char *value = find_header(request, name);
    size_t token_len = strlen(token);

    if (!value) {
        return 0;
    }

    while (*value && value[0] != '\r' && value[1] != '\n') {
        while (*value == ' ' || *value == '\t' || *value == ',') {
            value++;
        }

        if (ascii_strncasecmp(value, token, token_len) == 0) {
            char next = value[token_len];

            if (next == '\r' || next == '\n' || next == ',' ||
                next == ' ' || next == '\t') {
                return 1;
            }
        }

        while (*value && *value != ',' &&
               !(value[0] == '\r' && value[1] == '\n')) {
            value++;
        }
    }

    return 0;
}

static int copy_header_value(const char *request, const char *name, char *out,
                             size_t out_len)
{
    const char *value = find_header(request, name);
    size_t len = 0;

    if (!value || out_len == 0) {
        return 0;
    }

    while (value[len] && !(value[len] == '\r' && value[len + 1] == '\n')) {
        len++;
    }

    while (len > 0 && (value[len - 1] == ' ' || value[len - 1] == '\t')) {
        len--;
    }

    if (len >= out_len) {
        return 0;
    }

    memcpy(out, value, len);
    out[len] = '\0';
    return 1;
}

static void sha1_transform(Sha1Ctx *ctx, const unsigned char block[64])
{
    uint32_t w[80];
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
    uint32_t e;

    for (int i = 0; i < 16; i++) {
        w[i] = ((uint32_t)block[i * 4] << 24) |
               ((uint32_t)block[i * 4 + 1] << 16) |
               ((uint32_t)block[i * 4 + 2] << 8) |
               (uint32_t)block[i * 4 + 3];
    }

    for (int i = 16; i < 80; i++) {
        w[i] = rol32(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    }

    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];

    for (int i = 0; i < 80; i++) {
        uint32_t f;
        uint32_t k;
        uint32_t temp;

        if (i < 20) {
            f = (b & c) | ((~b) & d);
            k = 0x5A827999;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        }

        temp = rol32(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = rol32(b, 30);
        b = a;
        a = temp;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
}

static void sha1_init(Sha1Ctx *ctx)
{
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xC3D2E1F0;
    ctx->bit_count = 0;
}

static void sha1_update(Sha1Ctx *ctx, const unsigned char *data, size_t len)
{
    size_t used = (size_t)((ctx->bit_count / 8) % 64);

    ctx->bit_count += (uint64_t)len * 8;

    while (len > 0) {
        size_t free_space = 64 - used;
        size_t copy_len = len < free_space ? len : free_space;

        memcpy(ctx->buffer + used, data, copy_len);
        used += copy_len;
        data += copy_len;
        len -= copy_len;

        if (used == 64) {
            sha1_transform(ctx, ctx->buffer);
            used = 0;
        }
    }
}

static void sha1_final(Sha1Ctx *ctx, unsigned char digest[20])
{
    unsigned char pad = 0x80;
    unsigned char zero = 0x00;
    unsigned char len_bytes[8];
    uint64_t bits = ctx->bit_count;
    size_t used;

    for (int i = 0; i < 8; i++) {
        len_bytes[7 - i] = (unsigned char)(bits >> (i * 8));
    }

    sha1_update(ctx, &pad, 1);
    used = (size_t)((ctx->bit_count / 8) % 64);

    while (used != 56) {
        sha1_update(ctx, &zero, 1);
        used = (size_t)((ctx->bit_count / 8) % 64);
    }

    sha1_update(ctx, len_bytes, sizeof(len_bytes));

    for (int i = 0; i < 5; i++) {
        digest[i * 4] = (unsigned char)(ctx->state[i] >> 24);
        digest[i * 4 + 1] = (unsigned char)(ctx->state[i] >> 16);
        digest[i * 4 + 2] = (unsigned char)(ctx->state[i] >> 8);
        digest[i * 4 + 3] = (unsigned char)ctx->state[i];
    }
}

static void base64_encode_20(const unsigned char in[20], char out[29])
{
    static const char table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int o = 0;

    for (int i = 0; i < 18; i += 3) {
        out[o++] = table[in[i] >> 2];
        out[o++] = table[((in[i] & 0x03) << 4) | (in[i + 1] >> 4)];
        out[o++] = table[((in[i + 1] & 0x0f) << 2) | (in[i + 2] >> 6)];
        out[o++] = table[in[i + 2] & 0x3f];
    }

    out[o++] = table[in[18] >> 2];
    out[o++] = table[((in[18] & 0x03) << 4) | (in[19] >> 4)];
    out[o++] = table[(in[19] & 0x0f) << 2];
    out[o++] = '=';
    out[o] = '\0';
}

const char *ws_role_name(void)
{
    return "ws";
}

const char *ws_alpn_id(void)
{
    return "http/1.1";
}

int ws_is_upgrade_request(const char *request)
{
    return request && header_contains_token(request, "Connection", "upgrade") &&
           header_contains_token(request, "Upgrade", "websocket") &&
           find_header(request, "Sec-WebSocket-Key") != NULL;
}

int ws_build_handshake_response(const char *request, char *out, size_t out_len)
{
    char key[128];
    char accept_src[sizeof(key) + sizeof(WS_GUID)];
    unsigned char digest[20];
    char accept[29];
    Sha1Ctx sha1;

    if (!copy_header_value(request, "Sec-WebSocket-Key", key, sizeof(key))) {
        return 0;
    }

    snprintf(accept_src, sizeof(accept_src), "%s%s", key, WS_GUID);
    sha1_init(&sha1);
    sha1_update(&sha1, (const unsigned char *)accept_src, strlen(accept_src));
    sha1_final(&sha1, digest);
    base64_encode_20(digest, accept);

    return snprintf(out, out_len,
                    "HTTP/1.1 101 Switching Protocols\r\n"
                    "Upgrade: websocket\r\n"
                    "Connection: Upgrade\r\n"
                    "Sec-WebSocket-Accept: %s\r\n\r\n",
                    accept) < (int)out_len;
}
