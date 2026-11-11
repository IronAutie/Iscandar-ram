class Status:
    """System status codes -- the ONE canonical home for them.

    Rule: a new status is added here, one line, and nowhere else.
    gen_status.py turns exactly these lines into status.h for the
    C gears, so the two languages can never drift apart. Never
    hand-edit status.h, and never hand-write a status value in
    Gears.py or in a C file: import or include it instead.

    Engine / mode statuses (used by Python callers):
    """

    SUCCESS   = 0
    FAIL      = 1
    NANO_FAIL = 2
    KEYRANGE  = 3  # index/key out of range (views raise IndexError;
                   # the engine boundary converts it to this status)
    FAIL_OVERLAY = 4  # check-then-set failure on the commit path: a
                      # mark landing on occupied bits (or an unmark
                      # clearing clear ones) -- a misdesign, never
                      # tolerated silently
    FAIL_GAMMA_ROW = 5  # Gamma ladder: a row counter disagrees with
                        # the popcount of its row word
    FAIL_GAMMA_PLANE = 6  # Gamma ladder: a plane counter disagrees
                          # with the sum of its row counters
    FAIL_GAMMA_OVERALL = 7  # Gamma ladder: the overall counter
                            # disagrees with the sum of plane counters
    TINY_FAIL = 9  # Tiny probe acquittal: no Left run of at least 9
                   # reaching the request, and no sufficient tail run
    SMALL_FAIL = 10  # Small probe acquittal: no tail + head pair
                     # reaching the request (either edge missing,
                     # or P + Q < request). Not the paper's APL
                     # FAIL_SMALL 6, which collides with
                     # FAIL_GAMMA_PLANE.
    LARGE_FAIL = 11  # Large probe acquittal: no core blank
                     # sequence with a fitting head/tail
                     # scenario. Not the paper's APL FAIL_LARGE
                     # 7, which collides with FAIL_GAMMA_OVERALL.
    HUGE_FAIL = 12  # Huge probe acquittal (general site):
                    # a Huge candidate failed for a reason with
                    # no more specific Huge code below.
    HUGE_GATE_TRAILING_PLANE_GATE_FAIL = 13  # Huge no leading:
                    # the plane following the core cannot
                    # satisfy the remaining request (gate).
                    # Named, per the blackboard, to set a
                    # breakpoint on this exact site.
    HUGE_TRAILING_ROW_GATE_FAIL = 14  # Huge no leading: a
                    # required trailing row in the following
                    # plane does not have a zero count.
                    # Named for breakpointing.
    HUGE_NO_LEAD_TRAILER_FAIL = 15  # Huge no leading: the
                    # following row's oracle check for the
                    # final sectors fails. Named to set a
                    # breakpoint.
    HUGE_NO_TRAILER_ROW_FAIL = 16  # Huge no trailing: a row
                    # reversed into while checking lead
                    # padding rows does not have a zero count.
                    # Named for breakpointing.
    HUGE_ROW_DRIVER_TRAILING_ROW_FAIL = 17  # Normal Huge:
                    # a row in the trailing padding function
                    # body does not have a zero count. Named
                    # for breakpointing.
    HUGE_ROW_DRIVER_TRAILER_PADDING_FAIL = 18  # Normal Huge:
                    # the final trailing row's Left oracle
                    # check does not have room for the
                    # remaining padding sectors. Named for
                    # breakpointing.
    HUGE_SPLIT_FAIL = 19  # Huge second sweep (2026-10-26
                    # amendment): a split candidate (core - 1
                    # empty planes flanked by both edge planes)
                    # is acquitted: an allegedly empty core
                    # plane is not empty, or the two edges
                    # together cannot cover the remainder
                    # plus one plane. Named for breakpointing.
    POINTER_ADOPT_WORD_FAIL = 20  # MPUPointer adoption: the
                    # word is outside 0..MAX_WORD. Named for
                    # breakpointing (per-site pointer codes,
                    # 2026-11-10: the foundation tranche returned
                    # generic FAIL at every pointer refusal).
    POINTER_LOAD_RESERVED_FAIL = 21  # MPUPointer.load: the
                    # reserved field is not 0 or 1. Named for
                    # breakpointing.
    POINTER_LOAD_START_FAIL = 22  # MPUPointer.load: the start
                    # is outside 0..MAX_START. Named for
                    # breakpointing.
    POINTER_LOAD_COUNT_FAIL = 23  # MPUPointer.load: the count
                    # is outside 1..MAX_COUNT. Named for
                    # breakpointing.
    POINTER_LOAD_ORDER_FAIL = 24  # MPUPointer.load: the order
                    # is neither MOTOROLA nor INTEL. Named for
                    # breakpointing.
    POINTER_UNLOAD_ORDER_FAIL = 25  # MPUPointer.unload: the
                    # order is neither MOTOROLA nor INTEL (also
                    # the refusal mpupointer_coordinate
                    # propagates). Named for breakpointing.

    """Gear statuses (used across the C boundary by Gears.py and
    the gears themselves; separate names, and a nonzero success,
    so a gear outcome can never be mistaken for a mode outcome,
    not even numerically):
    """

    GEAR_SUCCESS = 8
    GEAR_FAIL    = 1
    GEAR_EXHAUST = 2
