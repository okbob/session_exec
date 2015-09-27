# session_exec
PostgreSQL extension that introduce login function

## Configuration (`postgresql.conf`)

1. set `session_preload_libraries` to `session_exec`
2. set `session_exec.login_name` to `name of your login function`

## Behave

* it raise warning when function doesn't exist
* it disallow to connect, when login function fails
