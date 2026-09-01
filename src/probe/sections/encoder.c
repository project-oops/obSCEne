/*
 * The encoder, at the reachability level: can an unsigned payload see libSceVencCore at all?
 *
 * # Why this section exists, and what it deliberately does not do
 *
 * `libSceVideoRecording` (105-record) is the high-level path to the console's hardware video
 * encoder; `libSceVencCore` is the encoder itself. Porthole - the remote-play stand-in in
 * prosperous/docs/VIDEO.md - rests on one question: can an unsigned payload reach that encoder?
 * If it can, Porthole is a few hundred lines at each end; if it cannot, the answer is raw frame
 * grabs and a stand-in is not worth building.
 *
 * This section answers the *first half* of that question, and only the first half: **are the key
 * encoder symbols present and callable from this build?** It does not call them. Their arities are
 * unconfirmed and `sceVencCoreCreateEncoder` / `SetInputFrame` take structures whose layouts
 * nobody here has - guessing one puts garbage in a register and crashes somewhere unrelated,
 * taking the whole report with it (Principle 2, D008). So the symbols are declared as **data**,
 * the type system forbids calling them, and the harness's own guard does the measuring: it skips a
 * check whose symbol did not resolve and runs it when it did. Reaching a check body therefore *is*
 * the fact - the symbol is present - and the section's pass/skip split is the reachability answer.
 *
 * The second half - opening a session and pulling one encoded access unit - is protocol-harness
 * work, where a fault is recorded as `died` rather than lost, exactly as 105-record says of its
 * own pointer-taking calls. When a sequence is established there, it can come back here as a check.
 *
 * Provenance: these names and their library are in the mined corpus from several public sources
 * (aerolib, ps4libdoc, fpPS4); no vendor header is used, and no signature is claimed here beyond
 * "it resolves".
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/sections.h"

/*
 * Declared as `const char`, so they can be addressed for a presence probe and never called - the
 * census's own rule, for the census's own reason. The three that matter to Porthole:
 *   - create the encoder session (the entry to everything)
 *   - pull an encoded access unit (what Porthole would stream)
 *   - query how much memory a session needs (the safest question, the one 105-record's sibling
 *     already answers for the recording path)
 */
extern OBS_WEAK const char sceVencCoreCreateEncoder;
extern OBS_WEAK const char sceVencCoreGetAuData;
extern OBS_WEAK const char sceVencCoreQueryMemorySize;

/* ---- 106-encoder ----------------------------------------------------------- */

/*
 * Each check is reached only when its symbol resolved (the harness skips it otherwise, reporting
 * "the loader did not resolve this symbol for this build" - which for a payload build is as likely
 * to be an unbound import table as a truly absent encoder, and is worth reading with that in mind).
 * So the body has one honest thing to say: it is here, therefore it is present and callable. What
 * it cannot say - whether an unsigned payload may actually drive it - is the protocol-harness half.
 */
static obs_result check_encoder_create_present(void) {
    return obs_pass();
}

static obs_result check_encoder_getaudata_present(void) {
    return obs_pass();
}

static obs_result check_encoder_query_present(void) {
    return obs_pass();
}

static const obs_check encoder_checks[] = {
    {"106-encoder/create-present", "libSceVencCore", "sceVencCoreCreateEncoder",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceVencCoreCreateEncoder,
     check_encoder_create_present, OBS_FROM_ASSUMED},
    {"106-encoder/getaudata-present", "libSceVencCore", "sceVencCoreGetAuData",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceVencCoreGetAuData,
     check_encoder_getaudata_present, OBS_FROM_ASSUMED},
    {"106-encoder/query-present", "libSceVencCore", "sceVencCoreQueryMemorySize",
     OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceVencCoreQueryMemorySize,
     check_encoder_query_present, OBS_FROM_ASSUMED},
};

const obs_section obs_section_encoder = {
    "106-encoder",
    "The hardware video encoder, reached",
    "Whether libSceVencCore's key entry points resolve from this build - the first half of "
    "Porthole's go/no-go. Presence only; driving the encoder is protocol-harness work.",
    encoder_checks,
    OBS_COUNT(encoder_checks),
};
