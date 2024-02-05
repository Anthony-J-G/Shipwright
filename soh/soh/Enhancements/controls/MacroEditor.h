#pragma once

#include "stdint.h"
#include <libultraship/libultraship.h>
#include <ImGui/imgui.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <set>


typedef enum {
    MacroEditorStatus_NotRecording      = 0,
    MacroEditorStatus_Recording,
    MacroEditorStatus_Saving,
    MacroEditorStatus_SavingFinished,
} MacroEditorStatus;



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
    bool isPlaying = false;

    ImVec4 statusColor = ImVec4(0.34f, 0.34f, 0.34f, 1.0f);

    u32 startingFrame = 0;
    u32 frameNum = 0;
    std::vector<OSContPad> history;

    OSContPad* p1;

    void DisplayStatus();

    void RecordInput(OSContPad input);
    void SendInput();
    
    void StartRecording();
    void StopRecording();

    void StartPlayback();
    void StopPlayback();

    bool SaveMacro();
    bool LoadMacro();
};
