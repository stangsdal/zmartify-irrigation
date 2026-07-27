#include "http_auth.h"

#include <string.h>

static bool http_auth_constant_time_equal(const uint8_t *left,
                                          const uint8_t *right,
                                          size_t length)
{
    uint8_t diff = 0;

    if (left == NULL || right == NULL) {
        return false;
    }

    for (size_t index = 0; index < length; ++index) {
        diff |= (uint8_t)(left[index] ^ right[index]);
    }
    return diff == 0;
}

bool http_auth_verifier_configured(const uint8_t verifier[HTTP_AUTH_SHA256_LEN])
{
    uint8_t any = 0;

    if (verifier == NULL) {
        return false;
    }

    for (size_t index = 0; index < HTTP_AUTH_SHA256_LEN; ++index) {
        any |= verifier[index];
    }
    return any != 0;
}

http_auth_result_t http_auth_check_bearer(
    const char *authorization_header,
    const uint8_t verifier[HTTP_AUTH_SHA256_LEN],
    http_auth_sha256_fn_t sha256_fn)
{
    static const char prefix[] = "Bearer ";
    uint8_t digest[HTTP_AUTH_SHA256_LEN] = {0};

    if (!http_auth_verifier_configured(verifier) || sha256_fn == NULL) {
        return HTTP_AUTH_RESULT_NOT_CONFIGURED;
    }
    if (authorization_header == NULL || authorization_header[0] == '\0') {
        return HTTP_AUTH_RESULT_MISSING;
    }
    if (strncmp(authorization_header, prefix, sizeof(prefix) - 1u) != 0) {
        return HTTP_AUTH_RESULT_MALFORMED;
    }

    const char *token = authorization_header + sizeof(prefix) - 1u;
    size_t token_len = strlen(token);
    if (token_len == 0u || token_len > HTTP_AUTH_TOKEN_MAX_LEN) {
        return HTTP_AUTH_RESULT_MALFORMED;
    }
    for (size_t index = 0; index < token_len; ++index) {
        unsigned char ch = (unsigned char)token[index];
        if (ch <= 0x20u || ch >= 0x7fu) {
            return HTTP_AUTH_RESULT_MALFORMED;
        }
    }

    if (!sha256_fn((const uint8_t *)token, token_len, digest)) {
        return HTTP_AUTH_RESULT_DENIED;
    }
    return http_auth_constant_time_equal(digest, verifier, HTTP_AUTH_SHA256_LEN)
        ? HTTP_AUTH_RESULT_AUTHORIZED
        : HTTP_AUTH_RESULT_DENIED;
}

const char *http_auth_result_name(http_auth_result_t result)
{
    switch (result) {
    case HTTP_AUTH_RESULT_AUTHORIZED:
        return "authorized";
    case HTTP_AUTH_RESULT_NOT_CONFIGURED:
        return "not_configured";
    case HTTP_AUTH_RESULT_MISSING:
        return "missing";
    case HTTP_AUTH_RESULT_MALFORMED:
        return "malformed";
    case HTTP_AUTH_RESULT_DENIED:
        return "denied";
    default:
        return "unknown";
    }
}