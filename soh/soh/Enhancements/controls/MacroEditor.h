#pragma once

#include "stdint.h"
#include <libultraship/libultraship.h>
#include <ImGui/imgui.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <set>

typedef int MacroEditorStatus; // -> enum MacroEditorStatus_

enum MacroEditorStatus_ {
    MacroEditorStatus_None             = 0 << 0,
    MacroEditorStatus_Recording        = 1 << 0,
    MacroEditorStatus_Saving           = 1 << 1,
    MacroEditorStatus_SavingFinished   = 1 << 2,
};

class MacroEditorWindow : public LUS::GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    ~MacroEditorWindow();


protected:
    void InitElement() override;
    void DrawElement() override;
    void UpdateElement() override;

private:
    bool isRecording = false;
    ImVec4 statusColor = ImVec4(0.34f, 0.34f, 0.34f, 1.0f);
    std::string statusTitle = "Not Recording";
    u32 startingFrame = 0;
    u32 frameNum = 0;
    std::vector<OSContPad> history;

    void DisplayStatus();

    void RecordButton(OSContPad input);
    
    void StartRecording();
    void StopRecording();

    bool SaveMacro();
    
};
