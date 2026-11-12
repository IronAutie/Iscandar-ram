#!/bin/sh
# link-check.sh: verify the Paper 3 build's linkage, end to
# end, on this machine. One PASS or FAIL line per check; the
# exit status is the number of failures, capped at 1. Run
# from anywhere; paths resolve from this script's folder.
#
# The checks:
#   build            the layer and the exerciser compile
#   conf-idempotent  the constants file regenerates unchanged
#   header-vs-map    every shared-header constant's value
#                    appears in the compile-time map document
#   xml-manifest     every repo path the ELBE image names
#                    exists
#   xml-copies       every inline body in the XML matches its
#                    file of record byte for byte
#   self-test        the layer's replay checks pass
#   exerciser        the standard run matches its prediction
#                    sheet (5,072 / 5,072, peak 9,216, end zero)
#   no-duplicates    no bridge constant is defined anywhere
#                    outside the shared header
#   patch-coverage   every reconciled center is named in the
#                    staged patch set, with hunks where the
#                    reconciliation requires them
#   not-patched      every named non-served family is listed
#                    in NOT-PATCHED.md
#   boot-order       the units order payload, layer, raise,
#                    exerciser, finish as the design requires
set -u
cd "$(dirname "$0")"
BUILDD=$(pwd)
REPO=..
FAILS=0
pass() { echo "PASS $1"; }
fail() { echo "FAIL $1: $2"; FAILS=1; }

# --- build -------------------------------------------------
if make >/tmp/iscandar-linkcheck-make.log 2>&1; then
  pass build
else
  fail build "make failed; see /tmp/iscandar-linkcheck-make.log"
  echo "LINK-CHECK: build failed, stopping"
  exit 1
fi

# --- conf-idempotent ---------------------------------------
cp constants/iscandar-build.conf /tmp/iscandar-conf-before.txt
./gen-build-conf.sh >/dev/null 2>&1
if diff -q /tmp/iscandar-conf-before.txt constants/iscandar-build.conf >/dev/null; then
  pass conf-idempotent
else
  fail conf-idempotent "regeneration changed the constants file"
fi

# --- header-vs-map -----------------------------------------
MAP=$(tr -d ',' < compile-time-map.md)
HVM=ok
while IFS='=' read -r name value; do
  case "$name" in ''|\#*) continue;; esac
  case "$value" in ''|*[!0-9]*) continue;; esac
  if ! printf '%s' "$MAP" | grep -q "$name"; then HVM="map lacks $name"; break; fi
  if ! printf '%s' "$MAP" | grep -q "$value"; then HVM="map lacks value $value for $name"; break; fi
done < constants/iscandar-build.conf
if [ "$HVM" = ok ]; then pass header-vs-map; else fail header-vs-map "$HVM"; fi

# --- xml-manifest and xml-copies (one python pass) ---------
python3 - << 'PYEOF'
import pathlib, re, sys
import xml.etree.ElementTree as ET

repo = pathlib.Path("..").resolve()
build = repo / "build"
xml_path = build / "elbe/fubuki-iscandar.xml"
text = xml_path.read_text()
fails = []

m = re.search(r"ISCANDAR-PAYLOAD-MANIFEST.*?-->", text, re.S)
if not m:
    fails.append(("xml-manifest", "manifest comment not found"))
else:
    paths = re.findall(r"(?:build|exerciser)/[A-Za-z0-9_./-]+", m.group(0))
    missing = [p for p in paths if not (repo / p).exists()]
    if missing:
        fails.append(("xml-manifest", "missing: " + ", ".join(missing)))
    else:
        print("PASS xml-manifest")

try:
    root = ET.parse(xml_path)
except Exception as e:
    fails.append(("xml-copies", f"XML does not parse: {e}"))
    root = None

if root is not None:
    ns = {"n": "http://www.elbe.org/schema/rootfilesystem-image/3.4"}
    files = {el.get("dst"): (el.text or "")
             for el in root.iter()
             if el.tag.endswith("file") and el.get("dst")}
    pairs = {
        "/usr/local/share/iscandar/iscandar-build.conf":
            "build/constants/iscandar-build.conf",
        "/usr/local/sbin/iscandar-payload-install":
            "build/units/iscandar-payload-install",
        "/etc/systemd/system/iscandar-payload.service":
            "build/units/iscandar-payload.service",
        "/etc/systemd/system/iscandar-layer.service":
            "build/units/iscandar-layer.service",
        "/etc/systemd/system/iscandar-raise.service":
            "build/units/iscandar-raise.service",
        "/etc/systemd/system/iscandar-exerciser.service":
            "exerciser/iscandar-exerciser.service",
        "/usr/local/sbin/iscandar-finish":
            "build/units/iscandar-finish",
        "/etc/systemd/system/iscandar-finish.service":
            "build/units/iscandar-finish.service",
    }
    bad = []
    for dst, rel in pairs.items():
        want = (repo / rel).read_text()
        got = files.get(dst)
        if got is None:
            bad.append(dst + " absent")
        elif got != want:
            bad.append(dst + " differs from " + rel)
    if bad:
        fails.append(("xml-copies", "; ".join(bad)))
    else:
        print("PASS xml-copies")

for name, why in fails:
    print(f"FAIL {name}: {why}")
sys.exit(1 if fails else 0)
PYEOF
[ $? -eq 0 ] || FAILS=1

# --- self-test ---------------------------------------------
OUT=$(./bin/iscandar-layer --self-test 2>&1)
if [ $? -eq 0 ] && printf '%s' "$OUT" | grep -q "SELF-TEST PASS"; then
  pass self-test
else
  fail self-test "layer self-test did not pass"
fi

# --- exerciser ---------------------------------------------
TMPD=$(mktemp -d)
( cd "$TMPD" && "$BUILDD/bin/iscandar-exerciser" > run.log 2>&1 )
EXRC=$?
if [ $EXRC -eq 0 ] \
   && grep -q "allocations 5072, frees 5072" "$TMPD/run.log" \
   && grep -q "peak live 9216 pages" "$TMPD/run.log" \
   && grep -q "end state 0 pages" "$TMPD/run.log"; then
  pass exerciser
else
  fail exerciser "standard run differs from the prediction sheet"
fi
rm -rf "$TMPD"

# --- no-duplicates -----------------------------------------
DUP=$(grep -rn --include='*.c' --include='*.h' --include='*.sh' \
  -E '#define[[:space:]]+ISCANDAR_(PAGE_BYTES|CUBE_PAGES|CUBE_BYTES|ARENA_CUBES|ARENA_PAGES|ARENA_BYTES|FIXTURE_PAGES_PER_CUBE|FIXTURE_LEDGER_PAGES|FIXTURE_MAPS_PAGES|FIXTURE_REGISTRY_PAGES|FIXTURE_PAGES_TOTAL|NET_SERVED_PAGES|REGISTRY_CAP|REGISTRY_ENTRY_BYTES|REGISTRY_PAGES|CHANNEL_PAGES|CHANNEL_BYTES|LAYER_RESERVE_PAGES|RING_RECORDS|RECORD_BYTES|PREFIX_LOG_ENTRIES|PREFIX_ENTRY_BYTES)' \
  . | grep -v 'bridge/iscandar_bridge.h' || true)
if [ -z "$DUP" ]; then
  pass no-duplicates
else
  fail no-duplicates "duplicate definitions: $DUP"
fi

# --- patch-coverage ----------------------------------------
COV=ok
for fn in slab_alloc_node __do_kmalloc_node __kmalloc_large_noprof \
          kfree kmem_cache_free kmem_cache_free_bulk \
          __kmem_cache_create_args kmem_cache_destroy krealloc_noprof \
          vm_area_alloc_pages vfree mempool_alloc_noprof \
          pcpu_alloc_noprof gen_pool_alloc_algo_owner \
          gen_pool_free_owner alloc_slab_page; do
  grep -rq "$fn" patch/ || { COV="center $fn not named in the patch set"; break; }
done
if [ "$COV" = ok ]; then
  grep -q slab_alloc_node patch/0001-slab-centers.patch || COV="1.1 hunk missing"
  grep -q __kmalloc_large_noprof patch/0001-slab-centers.patch || COV="1.3 hunk missing"
  grep -q kfree patch/0001-slab-centers.patch || COV="1.4 hunk missing"
  grep -q __kmem_cache_create_args patch/0002-cache-setup.patch || COV="1.5 hunk missing"
  grep -q vm_area_alloc_pages patch/0003-vmalloc-backing.patch || COV="2.3 hunk missing"
  grep -q vfree patch/0003-vmalloc-backing.patch || COV="2.4 hunk missing"
  grep -q pcpu_alloc_noprof patch/0004-second-ring.patch || COV="4.2 hunk missing"
  grep -q gen_pool_alloc_algo_owner patch/0004-second-ring.patch || COV="4.3 hunk missing"
  grep -q iscandar_bridge_call patch/0005-bridge-core.patch || COV="bridge core missing"
fi
if [ "$COV" = ok ]; then pass patch-coverage; else fail patch-coverage "$COV"; fi

# --- not-patched -------------------------------------------
NP=ok
for name in __alloc_pages_noprof __free_pages free_pages \
            __dma_direct_alloc_pages __get_vm_area_node alloc_vmap_area \
            memblock_alloc_internal vmap cma_alloc CONFIG_CMA; do
  grep -q "$name" patch/NOT-PATCHED.md || { NP="$name not listed in NOT-PATCHED.md"; break; }
done
if [ "$NP" = ok ]; then pass not-patched; else fail not-patched "$NP"; fi

# --- boot-order --------------------------------------------
BO=ok
grep -q "Before=iscandar-exerciser.service" units/iscandar-layer.service || BO="layer not Before the exerciser"
grep -q "ConditionPathExists=/usr/local/sbin/iscandar-layer" units/iscandar-layer.service || BO="layer lacks its payload condition"
grep -q "Before=iscandar-layer.service" units/iscandar-payload.service || BO="payload not Before the layer"
grep -q "After=iscandar-layer.service" units/iscandar-raise.service || BO="raise not After the layer"
grep -q "Before=iscandar-exerciser.service" units/iscandar-raise.service || BO="raise not Before the exerciser"
grep -q "After=iscandar-exerciser.service" units/iscandar-finish.service || BO="finish not After the exerciser"
if [ "$BO" = ok ]; then pass boot-order; else fail boot-order "$BO"; fi

if [ $FAILS -eq 0 ]; then
  echo "LINK-CHECK: all checks PASS"
else
  echo "LINK-CHECK: FAILURES present"
fi
exit $FAILS
