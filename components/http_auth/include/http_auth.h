#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HTTP_AUTH_SHA256_LEN 32u
#define HTTP_AUTH_TOKEN_MAX_LEN 96u

typedef bool (*http_auth_sha256_fn_t)(const uint8_t *input,
                                      size_t input_len,
                                      uint8_t output[HTTP_AUTH_SHA256_LEN]);

typedef enum {
    HTTP_AUTH_RESULT_AUTHORIZED = 0,
    HTTP_AUTH_RESULT_NOT_CONFIGURED,
    HTTP_AUTH_RESULT_MISSING,
    HTTP_AUTH_RESULT_MALFORMED,
    HTTP_AUTH_RESULT_DENIED,
} http_auth_result_t;

bool http_auth_verifier_configured(const uint8_t verifier[HTTP_AUTH_SHA256_LEN]);

http_auth_result_t http_auth_check_bearer(
    const char *authorization_header,
    const uint8_t verifier[HTTP_AUTH_SHA256_LEN],
    http_auth_sha256_fn_t sha256_fn);

const char *http_auth_result_name(http_auth_result_t result);