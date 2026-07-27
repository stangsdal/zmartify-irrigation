#include "http_auth.h"

#include <assert.h>
#include <string.h>

static bool fake_sha256(const uint8_t *input,
                        size_t input_len,
                        uint8_t output[HTTP_AUTH_SHA256_LEN])
{
    memset(output, 0, HTTP_AUTH_SHA256_LEN);
    if (input == NULL || input_len == 0u) {
        return false;
    }
    for (size_t index = 0; index < input_len; ++index) {
        output[index % HTTP_AUTH_SHA256_LEN] ^= input[index];
        output[(index * 7u) % HTTP_AUTH_SHA256_LEN] += (uint8_t)(input[index] + index);
    }
    return true;
}

static void make_verifier(const char *token, uint8_t verifier[HTTP_AUTH_SHA256_LEN])
{
    assert(fake_sha256((const uint8_t *)token, strlen(token), verifier));
}

static void test_requires_configured_verifier(void)
{
    uint8_t verifier[HTTP_AUTH_SHA256_LEN] = {0};
    assert(!http_auth_verifier_configured(verifier));
    assert(http_auth_check_bearer("Bearer admin-token", verifier, fake_sha256) ==
           HTTP_AUTH_RESULT_NOT_CONFIGURED);
}

static void test_accepts_matching_bearer_token(void)
{
    uint8_t verifier[HTTP_AUTH_SHA256_LEN];
    make_verifier("admin-token", verifier);

    assert(http_auth_verifier_configured(verifier));
    assert(http_auth_check_bearer("Bearer admin-token", verifier, fake_sha256) ==
           HTTP_AUTH_RESULT_AUTHORIZED);
}

static void test_rejects_missing_malformed_and_wrong_tokens(void)
{
    uint8_t verifier[HTTP_AUTH_SHA256_LEN];
    make_verifier("admin-token", verifier);

    assert(http_auth_check_bearer(NULL, verifier, fake_sha256) == HTTP_AUTH_RESULT_MISSING);
    assert(http_auth_check_bearer("", verifier, fake_sha256) == HTTP_AUTH_RESULT_MISSING);
    assert(http_auth_check_bearer("Basic abc", verifier, fake_sha256) == HTTP_AUTH_RESULT_MALFORMED);
    assert(http_auth_check_bearer("Bearer ", verifier, fake_sha256) == HTTP_AUTH_RESULT_MALFORMED);
    assert(http_auth_check_bearer("Bearer admin token", verifier, fake_sha256) ==
           HTTP_AUTH_RESULT_MALFORMED);
    assert(http_auth_check_bearer("Bearer other-token", verifier, fake_sha256) ==
           HTTP_AUTH_RESULT_DENIED);
}

int main(void)
{
    test_requires_configured_verifier();
    test_accepts_matching_bearer_token();
    test_rejects_missing_malformed_and_wrong_tokens();
    return 0;
}