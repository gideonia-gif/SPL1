# SPL Data Center – Assignment 1 (Systems Programming, BGU Fall 2026)

Skeleton from https://github.com/bguspl/spl27-ass1-students
Due: 26/11/2026 23:59, submitted in pairs on Moodle as `ID1_ID2.zip`.

## Environment
Docker Desktop running → VS Code → bottom-left `><` → **Reopen in Container**.
Do not modify `.devcontainer/`, `src/Main.cpp`, `include/json.hpp`, or the supplied
functions `readConfig`, `generateTokenEmbedding`, `mixByte`, `mixString`.

## Build & run (inside the container)
```
make clean && make
./bin/server config_example.json
valgrind --leak-check=full --show-reachable=yes ./bin/server config_example.json
```
Compile flags must be exactly: `g++ -Wall -Weffc++ -std=c++11 -c -Iinclude` (+ optional `-g`).
