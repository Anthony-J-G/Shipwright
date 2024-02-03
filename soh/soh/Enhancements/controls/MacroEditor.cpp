#include "MacroEditor.h"
#include "libultraship/libultra/controller.h"
#include <Utils/StringHelper.h>
#include "soh/OTRGlobals.h"
#include "../../UIWidgets.hpp"
#include "z64.h"
#include <thread>

#ifndef __WIIU__
#include "controller/controldevice/controller/mapping/sdl/SDLAxisDirectionToButtonMapping.h"
#endif

extern "C" PlayState* gPlayState;

#define SCALE_IMGUI_SIZE(value) ((value / 13.0f) * ImGui::GetFontSize())

std::thread gMacroThread;


void SaveMacroImgui() {
    CVarSetInteger("gMacroSaving", 1);
    CVarSave();

    // todo: convert MacroEditorWindow.history into a JSON serializable format


    // check if 'Macros/' directory exists

    // save macro .json


    CVarSetInteger("gMacroSaving", 0);
    CVarSave();
    CVarLoad();
}


MacroEditorWindow::~MacroEditorWindow() { }


void MacroEditorWindow::InitElement() { }


void MacroEditorWindow::UpdateElement() { }


void MacroEditorWindow::DrawElement() {
    ImGui::Begin("Input Macro Editor###sohMacroEditorWindowV1", &mIsVisible);

    // Break early if the PlayState is NULL
    if (gPlayState == NULL) {
        ImGui::Text("Waiting For Play State to start...");
        StopRecording();

        ImGui::End();
        return;
    }

    OSContPad* pads = LUS::Context::GetInstance()->GetControlDeck()->GetPads();
    OSContPad mainController = pads[0];

    // Frame Advance Controller Shortcuts
    if (mainController.button & BTN_DDOWN && mainController.button & BTN_L) { gPlayState->frameAdvCtx.enabled = true; }
    if (mainController.button & BTN_DUP && pads[0].button & BTN_L) { gPlayState->frameAdvCtx.enabled = false; }

    // Enable Frame Advance
    UIWidgets::PaddedSeparator();
    ImGui::Checkbox("Frame Advance##frameAdvance", (bool*)&gPlayState->frameAdvCtx.enabled);
    UIWidgets::Tooltip(
        "Toggles frame advance mode. Inputting L + D-pad Down turns it on, inputting L + D-pad Up turns it off."
    );
    if (gPlayState->frameAdvCtx.enabled) {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 6.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.22f, 0.38f, 0.56f, 1.0f));
        if (ImGui::Button("Advance 1", ImVec2(ImGui::GetContentRegionAvail().x / 2.0f, 0.0f))) {
            CVarSetInteger("gFrameAdvance", 1);
        }
        ImGui::SameLine();
        ImGui::Button("Advance (Hold)");
        if (ImGui::IsItemActive()) {
            CVarSetInteger("gFrameAdvance", 1);
        }
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(1);
    }

    // Setup Recording
    DisplayStatus();

    if (!isRecording) {
        if (ImGui::Button("Start Recording", ImVec2(-1.0f, 0.0f))) {
            StartRecording();
        }

    } else {
        std::string buttonsPressed = (
            std::to_string(pads[0].button) + " " + 
            std::to_string(pads[0].stick_x) + " " + 
            std::to_string(pads[0].stick_y)
        );

        RecordButton(mainController);

        ImGui::Text(buttonsPressed.c_str());

        if (ImGui::Button("Stop Recording", ImVec2(-1.0f, 0.0f))) {
            StopRecording();
        }

    }

    if (!isRecording && !history.empty()) {
        std::string msg = "Macro Loaded with total frame count (" + std::to_string(history.size()) + ")";
        ImGui::Text(msg.c_str());

        if (ImGui::Button("Save", ImVec2(-1.0f, 0.0f))) {
            SaveMacro();
        }

    }

    ImGui::End();
}


void MacroEditorWindow::DisplayStatus() {


    if (isRecording) {
        statusTitle = "Recording...";
        statusColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    }
    if (!isRecording) {
        statusTitle = "Not Recording";
        statusColor = ImVec4(0.34f, 0.34f, 0.34f, 1.0f);
    }

    ImGui::TextColored(statusColor, (statusTitle + " " + std::to_string(frameNum)).c_str());

}


void MacroEditorWindow::StartRecording() {
    isRecording = true;

    startingFrame = gPlayState->gameplayFrames;
    frameNum = 0;
    history.clear();
}


void MacroEditorWindow::RecordButton(OSContPad input) {
    if (frameNum == history.size()) {
        history.push_back(input);
    }
    frameNum = gPlayState->gameplayFrames - startingFrame;
}


void MacroEditorWindow::StopRecording() {
    isRecording = false;
}


bool MacroEditorWindow::SaveMacro() {
    if (CVarGetInteger("gMacroSaving", 0) == 0) {
        statusTitle = "Saving...";
        statusColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

        gMacroThread = std::thread(&SaveMacroImgui);
        return true;
    }    

    return false;
}