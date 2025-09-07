
#pragma once
#include <wx/frame.h>
#include <wx/menu.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include "Scene.h"

class OSGCanvas;


class MainFrame : public wxFrame {
public:
    MainFrame(const wxString& title);
private:
    void OnModeNormal(wxCommandEvent& event);
    void OnModeRectangle(wxCommandEvent& event);
    void OnModePolygon(wxCommandEvent& event);
    void OnOpenColmapFiles(wxCommandEvent& event);
    void OnExportColmapFiles(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);
    void OnDeleteSelected(wxCommandEvent& event);
    void OnInvertSelected(wxCommandEvent& event);

    OSGCanvas* m_canvas;
    wxPanel* m_panel;
    wxMenuBar* m_menuBar;
    wxBoxSizer* m_sizer;
    class Scene* m_scene = nullptr;

    wxDECLARE_EVENT_TABLE();
};
