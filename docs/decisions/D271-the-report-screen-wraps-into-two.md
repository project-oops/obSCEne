# D271 - The report screen wraps into two columns before it overruns the totals


The on-screen summary draws one row per section - identifier, proportion bar, four counts - and sizes the row height to fit every section above a reserved footer. That fit has a floor: below `OBS_ROW_H_MIN` (24px) the identifiers stop being readable across a room, which is the only thing the screen does better than the text stream. At **37 sections** a single column reaches the floor and still does not fit - `37 × 24 = 888px` against `740px` available - so `obs_display_rect` clipped the last rows straight over the totals and the footer. That is the confident-but-incomplete failure the screen's computed spacing was added to avoid, returning at a larger section count.

The fix is a second column rather than smaller rows. `obs_draw_summary_row` now draws one row within a given column `[x0, x0 + col_w)`, and the summary picks one column while the sections fit and two once they would overrun (`rows × OBS_ROW_H_MIN > available`). The left column fills first, top to bottom, so the order still reads down and then across; the row height is computed from the per-column count, not the total.

Two columns was chosen over the alternative the code's own comment had named - "fewer rows on screen", i.e. paginating the summary - because the summary's whole value is that **one photograph is the entire answer**. A paged summary needs two photographs to say what happened, and an unattended capture lands on one page. Two columns keep the single-photograph property and hold about sixty sections; a third is the next step and not needed yet. The single-column geometry is unchanged - the helper reproduces it pixel for pixel at full width - so every emulator's report and its committed screenshot render exactly as before; only a 30-plus-section run (hardware today) wraps.

Status: **done**.

