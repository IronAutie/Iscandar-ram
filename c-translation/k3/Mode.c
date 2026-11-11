/* Mode.c -- see Mode.h. Transcription of Mode.py; the
 * Python is the specification. */

#include "Mode.h"

#include "Cube.h"
#include "Plane.h"

int mode_has_keying(const Mode *mode, uint32_t key)
{
    return key >= mode->lo && key < mode->hi;
}

int32_t mode_base_search(const ModeCtx *ctx, uint32_t request,
                         bullet_fn bullet,
                         uint32_t *plane_out, uint32_t *row_out,
                         uint32_t *sector_out)
{
    /* Mode.search: Gears.scan(count_planes, count_rows,
     * request, self.bullet) -- the planes gear with the
     * rows row handler, the bullet's ctx the words array,
     * exactly as Gears.scan wires it in Gears.py. */
    return planes_gear(ctx->count_planes, ctx->count_rows,
                       request, rows_row_handler, bullet,
                       (void *)ctx->words, sector_out, row_out,
                       plane_out);
}
