# ApiClient

Spec: <https://github.com/xlair-dev/server/blob/main/docs/openapi.yaml>

Auth: <https://github.com/xlair-dev/server/blob/main/docs/auth/device.md>

## Tests

From the repository root:

```sh
cmake --preset <platform>-debug
cmake --build --preset build-api-client-tests-<platform>-debug
ctest --preset test-api-client-<platform>-debug
```
