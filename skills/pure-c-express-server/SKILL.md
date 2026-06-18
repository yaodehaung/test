---
name: pure-c-express-server
description: Project workflow for the pure C dependency-free mini Express server in this repository. Use when Codex is asked to modify, explain, build, run, debug, or commit changes involving main.c, Makefile, the epoll HTTP server, mini router, JSON field parser, in-memory hash map cache, or Linux-kernel-style commit messages.
---

# Pure C Express Server

## Overview

Work on this repository as a small Linux-only C web server project. Preserve the dependency-free style: standard C/POSIX APIs, Linux sockets, epoll, a hand-written router, a simple JSON field extractor, and an in-memory hash map cache.

## Project Shape

- `main.c` contains the full server implementation.
- `Makefile` builds `main.c` into `mini_express`.
- `CLAUDE.md` documents the architecture and expected behavior.
- `.vscode/` is local editor configuration. Do not stage or commit it unless the user explicitly asks for it.

## Build And Run

Use the Makefile targets:

```bash
make
make run
make debug
make clean
```

The code uses `sys/epoll.h`, so it must be compiled on Linux. On macOS, expect `make` to stop with the repository's Linux-only message. Do not treat that as a code regression unless the user asked for macOS portability.

## Coding Guidelines

- Keep the project dependency-free unless the user explicitly asks to add a library.
- Prefer focused edits inside `main.c`; avoid splitting files unless the user requests refactoring.
- Use standard C/POSIX patterns already present in the project.
- Be careful with non-blocking sockets: partial reads/writes, `EAGAIN`, and `EWOULDBLOCK` matter.
- Remember the current JSON parser is intentionally minimal and only extracts quoted string fields.
- Keep API responses JSON-shaped and update `Content-Length` through existing response formatting.
- If changing behavior, update `CLAUDE.md` when its documented routes, limitations, or build instructions become stale.

## Expected Routes

- `GET /` returns a welcome JSON response.
- `POST /api/cache/set` reads `{"key":"...","value":"..."}` from the request body and stores it.
- `GET /api/cache/get` currently reads `{"key":"..."}` from the request body and returns the cached value.

## Verification

When relevant, run:

```bash
make
```

If on Linux and the server can run safely, use:

```bash
make run
curl http://127.0.0.1:8080/
curl -X POST http://127.0.0.1:8080/api/cache/set -H "Content-Type: application/json" -d '{"key":"name","value":"Nick"}'
curl -X GET http://127.0.0.1:8080/api/cache/get -H "Content-Type: application/json" -d '{"key":"name"}'
```

On macOS, report that full compile/run verification requires Linux.

## Git Workflow

Before committing:

- Run `git status --short`.
- Stage only files the user asked to include.
- Leave `.vscode/` untracked unless explicitly requested.
- Write commit messages in Linux kernel style when the user asks for a commit or a commit message.

Kernel-style commit format:

```text
subsystem: concise imperative subject

Explain the bug, behavior change, or motivation in complete paragraphs.
Describe why the change is needed before describing what changed. Wrap body
lines around 72 columns when practical.

Describe the fix and important affected paths. Mention validation or limits
when relevant.

Fixes: <sha> ("original subject")
Cc: stable@vger.kernel.org
Reported-by: Name <email>
Signed-off-by: Name <email>
Link: https://example.invalid/patch-or-report
```

Use trailers only when they are true and provided by the repo history, user, or patch context. Do not invent `Fixes`, `Reported-by`, `Cc`, `Link`, or `Signed-off-by` lines. For this project, choose subjects such as `build: add Linux-only Makefile guard`, `docs: document project skill workflow`, or `net: preserve cache route semantics` depending on the touched area.
