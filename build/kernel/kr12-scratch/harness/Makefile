# iscandar_cycles_harness -- out-of-tree build for KR12.
#
# Build on telesart, in the build tree of record, AFTER the
# 0013 kernel build has completed there (the module's vermagic
# must match the running kernel exactly; building against the
# tree that produced the bzImage is what guarantees it):
#
#	cd ~/linux-6.12.116-kr
#	make M=<this directory> modules
#
# Expected: iscandar_cycles_harness.ko beside this Makefile,
# no warnings. Check before carry-in:
#
#	modinfo -F vermagic iscandar_cycles_harness.ko
#	# must read: 6.12.116-... SMP preempt mod_unload
#	# (the release suffix follows the tree's git state; what
#	# matters is that it equals `uname -r` in the guest)
#
# The module calls only exported stock symbols (vmalloc,
# vfree, sort) and reads /proc/iscandar for its boot label;
# it links against no Iscandar symbol, exported or otherwise.
# The same .ko serves both KR12 boots, which run the same
# kernel binary; Boot B differs only by the iscandar_hold
# boot parameter.

obj-m += iscandar_cycles_harness.o
