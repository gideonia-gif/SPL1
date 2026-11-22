# SPL – Assignment 1 (Systems Programming, BGU 2026-27)

Environment: the official course dev container (`ghcr.io/bguspl/student-env:1.4`) –
GCC 13 / C++, Java 25, Python 3.12, valgrind, gdb, cmake.

## Open the environment
1. Docker Desktop running.
2. VS Code → open this folder → bottom-left `><` → **Reopen in Container**
   (or "Rebuild Container" if you had opened it before with the old setup).
3. Check it works: in the VS Code terminal run `./env_check/run.sh`
   → should print Hello from C++, Java and Python.

## Files
- `config_example.json` – example input config given with the assignment
- `env_check/` – the course's smoke test
- Assignment PDF: kept locally, not committed (see .gitignore)
