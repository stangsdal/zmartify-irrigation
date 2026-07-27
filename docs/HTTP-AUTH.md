# HTTP Control-Plane Authentication

The local HTTP service protects administrative endpoints with an `Authorization: Bearer` token.
The firmware stores only a SHA-256 verifier in `main/http_auth.local.h`; the token itself must not
be committed, logged or placed in MQTT payloads.

Protected endpoints:

| Endpoint | Method | Required role |
|---|---|---|
| `/ota` | `POST` | administrator |
| `/reboot` | `POST` | administrator |
| `/config/network` | `POST` | administrator |
| `/weather` | `POST` | administrator |

Read-only endpoints remain unauthenticated for local diagnostics: `GET /health`, `GET /logs`,
`GET /storage/sd-card` and `GET /config/network`. `GET /config/network` reports whether the MQTT
password is configured, but never returns that password.

## Provisioning

Generate a high-entropy token with an approved password manager or local secret generator. Then
create the ignored verifier header:

```sh
ZIC_HTTP_ADMIN_TOKEN='replace-with-generated-token' ./scripts/configure-http-auth.sh
```

The script writes `main/http_auth.local.h` with mode `0600` subject to the current filesystem. If
the file is absent or contains an all-zero verifier, every protected endpoint fails closed with
HTTP 503.

## Use

Send the token as a bearer credential:

```sh
curl -H "Authorization: Bearer $ZIC_HTTP_ADMIN_TOKEN" \
  -H "Content-Type: application/json" \
  --data-binary @weather.json \
  http://192.168.10.113/weather
```

OTA tooling uses the same environment variable:

```sh
ZIC_HTTP_ADMIN_TOKEN='replace-with-generated-token' \
  ./scripts/ota-direct.sh 192.168.10.113 build/zmartify_irrigation.bin
```

Invalid, missing or malformed credentials receive HTTP 401 with a bearer challenge. Authorization
failures are logged by reason category only; the received credential is never logged.
After five failed authentication attempts within one minute, additional failed attempts receive
HTTP 429 until the window resets. A valid administrator token clears the failure counter.

Protected mutating endpoints also enforce content type before payload parsing: `/ota` requires
`application/octet-stream`, while `/config/network` and `/weather` require `application/json`.

## Rotation

Create a new token, rerun `scripts/configure-http-auth.sh`, build and deploy a signed image using
the current valid token. After the new image boots and passes OTA health confirmation, replace the
stored token in service tooling and revoke the old token from password-manager access records.