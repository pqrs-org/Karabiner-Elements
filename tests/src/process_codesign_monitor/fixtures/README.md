# Signed test fixtures

`process_codesign_test_helper_a.app` and `process_codesign_test_helper_b.app`
contain universal helper binaries signed with the same non-ad-hoc code-signing
identity.

Generate and sign both app fixtures:

```shell
make prepare-fixtures
```

`prepare-fixtures` uses `scripts/codesign.sh`, so
`PQRS_ORG_CODE_SIGN_IDENTITY` must refer to a valid code-signing identity.

Run the integration test:

```shell
make integration
```
