#include "global.h"
#include <string.h>

SaveContext gSaveContext;

void SaveContext_Init(void) {
    memset(&gSaveContext, 0, sizeof(gSaveContext));
    gSaveContext.seqId = (u8)NA_BGM_DISABLED;
    gSaveContext.natureAmbienceId = NATURE_ID_DISABLED;
    gSaveContext.forcedSeqId = NA_BGM_GENERAL_SFX;
    gSaveContext.nextCutsceneIndex = CVarGetInteger(CVAR_CHEAT("BetaQuestWorld"), 0xFFEF);
    gSaveContext.cutsceneTrigger = 0;
    gSaveContext.chamberCutsceneNum = 0;
    gSaveContext.nextDayTime = 0xFFFF;
    gSaveContext.skyboxTime = 0;
    gSaveContext.dogIsLost = true;
    gSaveContext.nextTransitionType = TRANS_NEXT_TYPE_DEFAULT;
    gSaveContext.unk_13EE = 50;
}
