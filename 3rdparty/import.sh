#!/usr/bin/env bash

# run this script from project root
# Make sure you used export script from your old project to export to
# this project ./tmp/gitmodules.txt

set -e

while read sha path url; do
    echo "→ Processing $path"
    mkdir -p "$(dirname "$path")"
    if [ ! -d "$path/.git" ]; then
        echo "   Cloning $url into $path"
        git clone "$url" "$path"
    else
        echo "   Already cloned, skipping clone"
    fi
    cd "$path" || exit 1
    echo "   Fetching origin"
    git fetch origin
    echo "   Checking out $sha"
    git checkout "$sha"
    cd - >/dev/null || exit 1
done < ./tmp/submodules.txt

while read sha path url; do
    echo "→ Adding submodule $path ($url)"
    git submodule add "$url" "$path"
done < ./tmp/submodules.txt
