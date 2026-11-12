#!/bin/sh
# gen-build-conf.sh -- generate the boot constants file from
# the ONE shared header. The conf is a generated copy, never
# hand-edited: a throwaway C program includes the header and
# prints the values, so the compiler does the arithmetic and
# the conf cannot drift from the header. build/link-check.sh
# regenerates it and diffs.
set -eu
cd "$(dirname "$0")"

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

cat > "$TMP/gen.c" <<'EOF'
#include <stdio.h>
#include "iscandar_bridge.h"
#define P(name) printf(#name "=%llu\n", (unsigned long long)(name))
int main(void)
{
    P(ISCANDAR_PAGE_BYTES);
    P(ISCANDAR_CUBE_PAGES);
    P(ISCANDAR_ARENA_CUBES);
    P(ISCANDAR_ARENA_PAGES);
    P(ISCANDAR_ARENA_BYTES);
    P(ISCANDAR_FIXTURE_PAGES_TOTAL);
    P(ISCANDAR_NET_SERVED_PAGES);
    P(ISCANDAR_REGISTRY_CAP);
    P(ISCANDAR_REGISTRY_PAGES);
    P(ISCANDAR_CHANNEL_PAGES);
    P(ISCANDAR_CHANNEL_BYTES);
    P(ISCANDAR_LAYER_RESERVE_PAGES);
    return 0;
}
EOF

cc -std=c99 -Ibridge -o "$TMP/gen" "$TMP/gen.c"
mkdir -p constants
{
    echo "# iscandar-build.conf -- GENERATED from build/bridge/iscandar_bridge.h"
    echo "# by build/gen-build-conf.sh. Do not hand-edit; the link-check diffs it."
    "$TMP/gen"
} > constants/iscandar-build.conf
echo "wrote constants/iscandar-build.conf"
