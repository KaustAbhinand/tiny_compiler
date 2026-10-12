#!/bin/sh
trap 'echo; echo "bye"; exit 0' INT

echo "TINY environment (Ctrl+C to exit)"
printf "tiny> "
read -r f || exit 0
[ -z "$f" ] && exit 0

./tiny "$f" -o /tmp/tiny_env.tbc || exit 1

while true; do
    printf "tiny> "
    ./tinyvm /tmp/tiny_env.tbc
done
