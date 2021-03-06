
#include <thread>
#include <utility>
#include <cstring>

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/ip/Fill.h"
#include "cinder/Log.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"

#include "imgui.h"
#include "imgui_impl_cinder_gl3.h"

#include "MidiConnection.h"
#include "VirtualLpp.h"
#include "Timer.h"

using namespace ci;
using namespace ci::app;

using namespace std;
using namespace std::chrono;

using namespace glm;

const int defaultSize = 800;
const int portNameLength = 128;
const int maxPorts = 32;

const ImGuiWindowFlags windowFlags =
    ImGuiWindowFlags_NoTitleBar
    | ImGuiWindowFlags_NoResize
    | ImGuiWindowFlags_NoMove
    | ImGuiWindowFlags_NoCollapse;

class VirtualLppApp : public App
{
public:
    VirtualLppApp()
        : io(ImGui::GetIO())
    { }
    
    void setup() override;
    void cleanup() override;
    
    void mouseDown( MouseEvent event ) override;
    void mouseUp(MouseEvent event) override;
    void mouseDrag(MouseEvent event) override;
    
    void keyDown(KeyEvent event) override;
    void keyUp(KeyEvent event) override;
    
    void resize() override;
    void update() override;
    void draw() override;
    
private:
    void drawGui();
    void drawBottomPanel();
    void drawSidePanel();
    void drawMidiConnectionMenu(const char* label, MidiConnection& con);
    void drawSequenceNotes(Sequence& s);
    void drawSequenceInfo(Sequence& s);
    
    VirtualLpp lpp;
    lpp::Timer lppTimer;
    mutex lppMutex;
    
    ImGuiIO& io;
    bool showGui;
    ImVec2 sidePanelPos;
    ImVec2 sidePanelSize;
    ImVec2 bottomPanelPos;
    ImVec2 bottomPanelSize;
    
    array<string, GRID_SIZE> sequenceNames;
};

void VirtualLppApp::setup()
{
    lpp.setWidth(defaultSize);
    lppTimer.start(milliseconds(1), [&] {
        lock_guard<mutex> lock(lppMutex);
        lpp.update();
    });
    
    ImGui_ImplCinder_Init(true);
    io.IniFilename = nullptr;
    io.Fonts->AddFontFromFileTTF(
        getAssetPath("Cousine-Regular.ttf").string().data(),
        14.0);
    io.Fonts->Build();
    
    showGui = true;
    
    for (int i = 0; i < GRID_SIZE; i++)
    {
        sequenceNames[i] = "Sequence " + to_string(i + 1);
    }
    
    gl::enableAlphaBlending();
}

void VirtualLppApp::cleanup()
{
    lppTimer.stop();
    ImGui_ImplCinder_Shutdown();
}

void VirtualLppApp::mouseDown(MouseEvent event)
{
    lock_guard<mutex> lock(lppMutex);
    lpp.mouseDown(event);
}

void VirtualLppApp::mouseUp(MouseEvent event)
{
    lock_guard<mutex> lock(lppMutex);
    lpp.mouseUp(event);
}

void VirtualLppApp::mouseDrag(MouseEvent event)
{
    lock_guard<mutex> lock(lppMutex);
    lpp.mouseDrag(event);
}

void VirtualLppApp::keyDown(KeyEvent event)
{
    if (event.getChar() == '`')
    {
        showGui = !showGui;
        resize();
    }
}

void VirtualLppApp::keyUp(KeyEvent event)
{
    
}

void VirtualLppApp::resize()
{
    int w = getWindowWidth();
    int h = getWindowHeight();
    
    int lppSize = min(getWindowWidth(), getWindowHeight());
    
    if (showGui)
    {
        lppSize = 2 * lppSize / 3;
        sidePanelPos = ImVec2(lppSize, 0);
        sidePanelSize = ImVec2(w - lppSize, lppSize);
        bottomPanelPos = ImVec2(0, lppSize);
        bottomPanelSize = ImVec2(w, h - lppSize);
    }
    
    {
        lock_guard<mutex> lock(lppMutex);
        lpp.setWidth(lppSize);
    }
    gl::setMatricesWindow(w, h);
}

void VirtualLppApp::update()
{
    
}

void VirtualLppApp::draw()
{
    gl::setMatricesWindow(getWindowSize());
    gl::clear(ColorAf(0.2, 0.2, 0.2));
    lpp.draw();
    
    ImGui_ImplCinder_NewFrame();
    if (showGui)
    {
        drawGui();
    }
    ImGui::Render();
}

void VirtualLppApp::drawGui()
{
    drawBottomPanel();
    drawSidePanel();
}

void VirtualLppApp::drawBottomPanel()
{
    ImGui::SetNextWindowPos(bottomPanelPos);
    ImGui::SetNextWindowSize(bottomPanelSize);
    ImGui::Begin(
        "bottom panel", NULL,
        windowFlags | ImGuiWindowFlags_HorizontalScrollbar);
    
    string scaleSteps = "";
    int lastOffset = 0;
    for (int i = 1; i < lp_scale.num_notes; i++)
    {
        scaleSteps += to_string(lp_scale.offsets[i] - lastOffset);
        lastOffset = lp_scale.offsets[i];
        scaleSteps += ", ";
    }
    scaleSteps += to_string(12 - lastOffset);
    
    bool armed = flag_is_set(lp_flags, LP_ARMED);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(armed ? 1 : 0.5, 0, 0, 1));
    ImGui::Value("Armed", armed);
    ImGui::PopStyleColor();
    ImGui::SameLine();
    
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0.3, 0.3, 1));
    ImGui::Value("BPM", millis_to_bpm((float)lp_sequencer.step_millis));
    ImGui::PopStyleColor();
    ImGui::SameLine();
    
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0.7, 0.3, 1));
    ImGui::Text("Swing: %.2f%%",
                100.0f * (lp_sequencer.swing_millis + lp_sequencer.step_millis)
                / (2 * lp_sequencer.step_millis));
    ImGui::PopStyleColor();
    ImGui::SameLine();
    
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3, 0.7, 1, 1));
    ImGui::Text("Scale: %s", scaleSteps.data());
    ImGui::PopStyleColor();
    ImGui::SameLine();
    
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3, 1, 0.3, 1));
    ImGui::Text("Modifiers: 0x%08x", (unsigned int)lp_modifiers);
    ImGui::PopStyleColor();
    
    ImGui::Separator();
    
    for (int seqI = 0; seqI < GRID_SIZE; seqI++)
    {
        drawSequenceNotes(lp_sequencer.sequences[seqI]);
    }
    
    ImGui::End();
}

void VirtualLppApp::drawSidePanel()
{
    ImGui::SetNextWindowPos(sidePanelPos);
    ImGui::SetNextWindowSize(sidePanelSize);
    ImGui::Begin("side panel", NULL, windowFlags);
    
    ImGui::Text("Master Sequence: %d", lp_sequencer.master_sequence + 1);
    
    for (int i = 0; i < GRID_SIZE; i++)
    {
        if (ImGui::TreeNode(sequenceNames[i].data()))
        {
            ImGui::BeginGroup();
            drawSequenceInfo(lp_sequencer.sequences[i]);
            ImGui::EndGroup();
            ImGui::TreePop();
        }
    }

    // drawMidiConnectionMenu("Midi In", lpp.getMidiIn());
    // drawMidiConnectionMenu("Midi Out", lpp.getMidiOut());

    ImGui::End();
}

void VirtualLppApp::drawMidiConnectionMenu(const char* label, MidiConnection& con)
{
    int portChoice = con.getId();
    const vector<string>& portNames = con.getPortNames();

    bool (*portNameFn)(void*, int, const char**) =
        [](void* data, int i, const char** text)->bool
        {
            vector<string>* names = (vector<string>*)data;
            *text = (*names)[i].c_str();
            return true;
        };

    if (ImGui::Combo(label, &portChoice,
                     portNameFn, (void*)&portNames, portNames.size()))
    {
        con.connect(portChoice);
    }
}

void VirtualLppApp::drawSequenceNotes(Sequence& s)
{
    for (int stepI = 0; stepI < SEQUENCE_LENGTH; stepI++)
    {
        Note& n = s.notes[stepI];
        
        ImVec4 color =
            s.playhead == stepI ? ImVec4(0.2, 0.2, 0.2, 1)
            : flag_is_set(n.flags, NTE_SLIDE) ? ImVec4(0, 0, 0.3, 1)
            : n.note_number > -1 ? ImVec4(0, 0.3, 0, 1)
            : ImVec4(0.2, 0, 0, 1);
        ImVec4 hoverColor = ImVec4(color.x * 1.1, color.y * 1.1, color.z * 1.1, 1);

        ImGui::PushStyleColor(ImGuiCol_Button, color);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, hoverColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
        
        ImGui::SmallButton(to_string(n.note_number).data());
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(
                "Velocity: %d\n"
                "Skip: %s",
                n.velocity,
                flag_is_set(n.flags, NTE_SKIP) ? "true" : "false");
        }
        
        ImGui::PopStyleColor(3);
        
        ImGui::SameLine();
    }
    ImGui::Dummy(ImVec2(0, 0));
}

void VirtualLppApp::drawSequenceInfo(Sequence& s)
{
    ImGui::Text("State: %s",
        flag_is_set(s.flags, SEQ_PLAYING)
            ? "Playing" : "Stopped");
    
    ImGui::Value("Muted", flag_is_set(s.flags, SEQ_MUTED));
    ImGui::Value("Soloed", flag_is_set(s.flags, SEQ_SOLOED));
    ImGui::Value("Linked To", flag_is_set(s.flags, SEQ_LINKED_TO));
    ImGui::Value("Linked", flag_is_set(s.flags, SEQ_LINKED));

    ImGui::Value("Octave", s.layout.octave);
    ImGui::Value("Root Note", s.layout.root_note);
    ImGui::Value("Channel", s.channel);
    
    ImGui::Value("Record Control", flag_is_set(s.flags, SEQ_RECORD_CONTROL));
    ImGui::Value("Control Code", s.control_code);
    ImGui::Value("Control Division", s.control_div);
    ImGui::Value("Control Offset", s.control_offset);
}


CINDER_APP(VirtualLppApp,
           RendererGl(RendererGl::Options().msaa(4)),
           [&]( App::Settings *settings ) {
               settings->setWindowSize(defaultSize, defaultSize);
           })




