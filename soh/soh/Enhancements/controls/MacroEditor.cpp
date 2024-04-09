#include "MacroEditor.h"
#include "libultraship/libultra/controller.h"
#include <Utils/StringHelper.h>
#include "soh/OTRGlobals.h"
#include "../../UIWidgets.hpp"
#include "z64.h"

#include <nlohmann/json.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <thread>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <variables.h>

#ifndef __WIIU__
#include "controller/controldevice/controller/mapping/sdl/SDLAxisDirectionToButtonMapping.h"
#endif

#define SCALE_IMGUI_SIZE(value) ((value / 13.0f) * ImGui::GetFontSize())

using json = nlohmann::ordered_json;


extern "C" PlayState* gPlayState;

u8 gSaved;
json gLoadedMacroJSON;
std::thread gMacroThread;

static const char* recordingStatusDisplayOptions[4] = { "Not Recording", "Recording...", "Saving...", "Saved!" };

// Test function to debug sending inputs to the game
void TestPlayback() {




}



void SaveMacroImgui(const std::vector<OSContPad>& history) {
    CVarSetInteger("gMacroSaving", 1);
    CVarSave();

    // todo: convert MacroEditorWindow.history into a JSON serializable format
    auto spoilerLog = tinyxml2::XMLDocument(false);
    spoilerLog.InsertEndChild(spoilerLog.NewDeclaration());

    auto rootNode = spoilerLog.NewElement("spoiler-log");
    spoilerLog.InsertEndChild(rootNode);

    gLoadedMacroJSON.clear();

    gLoadedMacroJSON["version"] = (char*) gBuildVersion;
    int i = 0;
    for (const auto& inputs : history) {
        gLoadedMacroJSON["inputHistory"].push_back({ 
            inputs.button, inputs.err_no,
            inputs.gyro_x, inputs.gyro_y,
            inputs.stick_x, inputs.stick_y,
            inputs.right_stick_x, inputs.right_stick_y,
        });
    }

    // todo: check if 'Macros/' directory exists
    if (!std::filesystem::exists(LUS::Context::GetPathRelativeToAppDirectory("Macros"))) {
        std::filesystem::create_directory(LUS::Context::GetPathRelativeToAppDirectory("Macros"));
    }

    // todo: save macro .json
    std::string jsonString = gLoadedMacroJSON.dump(4);
    std::ostringstream fileNameStream;
    for (int i = 0; i < 4; i++) {
        if (i) {
            fileNameStream << '-';
        }
        if (i < 10) {
            fileNameStream << '0';
        }
        fileNameStream << std::to_string(i);
    }
    std::string fileName = fileNameStream.str();
    std::ofstream jsonFile(LUS::Context::GetPathRelativeToAppDirectory(
        (std::string("Macros/") + fileName + std::string(".json")).c_str())
    );
    jsonFile << std::setw(4) << jsonString << std::endl;
    jsonFile.close();

    CVarSetInteger("gMacroSaving", 0);
    CVarSave();
    CVarLoad();

    gSaved = 1;
}

void LoadMacroImgui() {

}

MacroEditorWindow::~MacroEditorWindow() { }


void MacroEditorWindow::InitElement() { }


void MacroEditorWindow::UpdateElement() { }


void MacroEditorWindow::DrawElement() {
    if (gSaved) {
        gSaved = 0;
        gMacroThread.join();
    }

    ImGui::Begin("Input Macro Editor###sohMacroEditorWindowV1", &mIsVisible);

    // Break early if the PlayState is NULL
    if (gPlayState == NULL) {
        ImGui::Text("Waiting For Play State to start...");
        StopRecording();
        StopPlayback();

        ImGui::End();
        return;
    }

    OSContPad* pads = LUS::Context::GetInstance()->GetControlDeck()->GetPads();
    OSContPad mainController = pads[0];

    // Frame Advance Controller Shortcuts
    if (mainController.button & BTN_DDOWN && mainController.button & BTN_L) { gPlayState->frameAdvCtx.enabled = true; }
    if (mainController.button & BTN_DUP && pads[0].button & BTN_L) { gPlayState->frameAdvCtx.enabled = false; }

    // Enable Frame Advance
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
    UIWidgets::PaddedSeparator();

    // Setup Recording
    DisplayStatus();
    UIWidgets::PaddedSeparator();

    ImGui::Text("Macro Recording:");
    ImGui::BeginDisabled(isPlaying);
    if (!isRecording && ImGui::Button("Start Recording")) {
        StartRecording();
    }
    if (isRecording && ImGui::Button("Stop Recording")) {
        StopRecording();
    }

    if (isRecording) {
        std::string buttonsPressed = (
            std::to_string(mainController.button) + " " + 
            std::to_string(mainController.stick_x) + " " +
            std::to_string(mainController.stick_y)
        );
        ImGui::SameLine();
        ImGui::Text(buttonsPressed.c_str());

        RecordInput(mainController);
    }

    if (!isRecording && !history.empty()) {
        ImGui::SameLine();
        if (ImGui::Button("Save")) {
            SaveMacro();
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
        }

    }
    ImGui::EndDisabled();
    UIWidgets::PaddedSeparator();

    ImGui::Text("Macro Playback:");
    ImGui::BeginDisabled(isRecording);
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 6);
    std::string macroName = "Current";
    if (ImGui::BeginCombo("Loaded Macro", macroName.c_str())) {
        if (ImGui::Selectable("Current")) {

        }
        if (ImGui::Selectable("None")) {

        }

        ImGui::EndCombo();
    }

    if (!isPlaying && ImGui::Button("Start Playback")) {
        StartPlayback();
    }
    if (isPlaying && ImGui::Button("Stop Playback")) {
        StopPlayback();
    }
    ImGui::EndDisabled();

    if (isPlaying) {
        SendInput();
    }

    UIWidgets::PaddedSeparator();
    if (ImGui::Button("Run Test")) {
        std::shared_ptr<LUS::Controller> t = LUS::Context::GetInstance()->GetControlDeck()->GetControllerByPort(0);
    }

    ImGui::End();
}


void MacroEditorWindow::DisplayStatus() {
    std::string statusTitle = "";

    if (isRecording) {
        statusTitle = std::string("Recording... ") + std::to_string(frameNum);
        statusColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

    } else if (!isRecording && !isPlaying && history.empty()) {
        statusTitle = "Idle";
        statusColor = ImVec4(0.34f, 0.34f, 0.34f, 1.0f);

    } else if (!isRecording && !isPlaying) {
        statusTitle = "Idle (" + std::to_string(history.size()) + " Frames Loaded)";
        statusColor = ImVec4(0.34f, 0.34f, 0.34f, 1.0f);

    } else if (CVarGetInteger("gMacroSaving", 1) == 1) {
        statusTitle = "Saving";
        statusColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

    } else if (isPlaying) {
        statusTitle =
            std::string("Playing... (") + std::to_string(frameNum) + "/" + std::to_string(history.size()) + ")";
        statusColor = ImVec4(0.60f, 0.40f, 0.0f, 1.0f);
    }

    ImGui::TextColored(statusColor, statusTitle.c_str());
}


void MacroEditorWindow::StartRecording() {
    isRecording = true;

    startingFrame = gPlayState->gameplayFrames;
    frameNum = 0;
    history.clear();
}


void MacroEditorWindow::RecordInput(OSContPad input) {
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
        gMacroThread = std::thread(&SaveMacroImgui, history);
        return true;
    }

    return false;
}


void MacroEditorWindow::StartPlayback() {
    if (history.empty()) {
        return;
    }

    isPlaying = true;
    mainController = LUS::Context::GetInstance()->GetControlDeck()->GetControllerByPort(0);

    startingFrame = gPlayState->gameplayFrames;
    frameNum = 0;

    // todo: disable external input
    // LUS::Context::GetInstance()->GetControlDeck()->BlockGameInput(0);
}


void MacroEditorWindow::SendInput() {
    u32 currentFrame = gPlayState->gameplayFrames;
    u32 framesPassed =  currentFrame - startingFrame;
    if (framesPassed >= (history.size() - 1) || (framesPassed < 0)) {
        // If more frames have passed than exist in the history, stop macro playback
        StopPlayback();
        return;
    }
    frameNum = framesPassed;

    OSContPad *pads = LUS::Context::GetInstance()->GetControlDeck()->GetPads();
    pads[0] = history[framesPassed];

    // Send Input to Game
    LUS::Context::GetInstance()->GetControlDeck()->WriteToPad(pads);
}


void MacroEditorWindow::StopPlayback() {
    isPlaying = false;

    // todo: Enable external input again
    // LUS::Context::GetInstance()->GetControlDeck()->UnblockGameInput(0);
}


bool MacroEditorWindow::LoadMacro() {
    return false;
}