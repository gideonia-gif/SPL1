#!/usr/bin/env bash
# Compile everything in src/ and run a class with a main method.
# Usage: ./run.sh            -> just compile
#        ./run.sh TestBalance -> compile and run TestBalance
set -e
cd "$(dirname "$0")"
rm -rf bin && mkdir -p bin
javac -encoding UTF-8 -d bin $(find src -name '*.java')
echo "Compiled OK."
if [ -n "$1" ]; then
  java -cp bin "$1"
fi
