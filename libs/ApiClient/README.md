# ApiClient

Spec: <https://github.com/xlair-dev/server/blob/main/docs/openapi.yaml>

Auth: <https://github.com/xlair-dev/server/blob/main/docs/auth/device.md>

## Mock client

`MockClient` loads `sync.json` and `state.json` from a local directory. It uses the
same `IClient` interface as `HttpClient`; writes update only in-memory state.
Origin-relative asset URLs are copied from `assets/<URL path>`.

## Tests

From the repository root:

```sh
cmake --preset <platform>-debug
cmake --build --preset build-api-client-tests-<platform>-debug
ctest --preset test-api-client-<platform>-debug
```
