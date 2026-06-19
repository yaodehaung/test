#include "../lib/roles/ws.h"

#include <stdio.h>
#include <string.h>

static int expect_bytes(const unsigned char *got, const unsigned char *want,
                        size_t len, const char *label)
{
    if (memcmp(got, want, len) == 0) {
        return 1;
    }

    fprintf(stderr, "%s did not match expected bytes\n", label);
    for (size_t i = 0; i < len; i++) {
        fprintf(stderr, "  offset %zu: got 0x%02x expected 0x%02x\n",
                i, got[i], want[i]);
    }
    return 0;
}

static int test_fragmented_text_message(void)
{
    const char *parts[] = {
        "hello ",
        "from ",
        "fragmented websocket"
    };
    unsigned char frames[128];
    const unsigned char expected[] = {
        0x01, 0x06, 'h', 'e', 'l', 'l', 'o', ' ',
        0x00, 0x05, 'f', 'r', 'o', 'm', ' ',
        0x80, 0x14, 'f', 'r', 'a', 'g', 'm', 'e', 'n', 't', 'e', 'd',
        ' ', 'w', 'e', 'b', 's', 'o', 'c', 'k', 'e', 't'
    };
    size_t len = ws_build_text_fragments(parts, 3, (char *)frames,
                                         sizeof(frames));

    if (len != sizeof(expected)) {
        fprintf(stderr, "fragment length was %zu, expected %zu\n",
                len, sizeof(expected));
        return 0;
    }

    return expect_bytes(frames, expected, sizeof(expected),
                        "fragmented text message");
}

static int test_close_frame(void)
{
    unsigned char frame[8];
    const unsigned char expected[] = {0x88, 0x00};
    size_t len = ws_build_close_frame((char *)frame, sizeof(frame));

    if (len != sizeof(expected)) {
        fprintf(stderr, "close frame length was %zu, expected %zu\n",
                len, sizeof(expected));
        return 0;
    }

    return expect_bytes(frame, expected, sizeof(expected), "close frame");
}

static int test_rejects_long_demo_payload(void)
{
    char long_payload[126];
    const char *parts[] = {long_payload};
    char out[256];

    memset(long_payload, 'x', sizeof(long_payload));

    if (ws_build_text_fragments(parts, 1, out, sizeof(out)) != 0) {
        fprintf(stderr, "expected payload longer than 125 bytes to fail\n");
        return 0;
    }

    return 1;
}

int main(void)
{
    if (!test_fragmented_text_message() ||
        !test_close_frame() ||
        !test_rejects_long_demo_payload()) {
        return 1;
    }

    puts("WebSocket fragment unit test passed");
    return 0;
}
