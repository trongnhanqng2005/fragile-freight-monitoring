# Conventions

## General

- Simplicity first; avoid over-engineering.
- Keep changes scoped to the requested task.
- Do not add unnecessary dependencies.
- Inspect existing code and configuration before modifying them.
- Keep `backend/` and `firmware/` as separate modules.
- Do not create nested Git repositories.
- Build or test the affected module after changes.
- Prefer clear naming over excessive abstraction.
- Do not introduce infrastructure or frameworks without a concrete requirement.

## Naming

- **Java packages and classes:** Use the existing reverse-domain package root `vn.edu.huit.fragilefreight`; packages are lowercase and classes use `PascalCase` with descriptive names.
- **REST endpoints:** Use lowercase, plural resource names and consistent path segments (for example, `/api/readings` when a readings resource is implemented). Do not add endpoints without a defined need.
- **MQTT topics:** Use lowercase, slash-separated hierarchical names. Agree on the actual topic hierarchy and payload contract before implementation; no topic names are defined yet.
- **Firmware source files:** Use descriptive names and the existing PlatformIO `firmware/src/` location. Use `.cpp` for C++ source and `.h` for shared headers.
