
#include "MainFrame.h"
#include "OSGCanvas.h"
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/aboutdlg.h>

enum {
    ID_OpenColmap = wxID_HIGHEST + 1,
    ID_ExportColmap,
    ID_DeleteSelected,
    ID_ModeNormal,
    ID_ModeRectangle,
    ID_ModePolygon,
    ID_ModeRectangleCam,
    ID_ModePolygonCam,
    ID_InvertSelected,
    ID_ResetView,
    ID_IncreasePointSize,
    ID_DecreasePointSize,
    ID_IncreaseCamSize,
    ID_DecreaseCamSize,
	ID_About
};

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_MENU(ID_OpenColmap, MainFrame::OnOpenColmapFiles)
    EVT_MENU(ID_ExportColmap, MainFrame::OnExportColmapFiles)
    EVT_MENU(ID_DeleteSelected, MainFrame::OnDeleteSelected)
    EVT_MENU(ID_InvertSelected, MainFrame::OnInvertSelected)
    EVT_MENU(ID_ModeNormal, MainFrame::OnModeNormal)
    EVT_MENU(ID_ModeRectangle, MainFrame::OnModeRectangle)
    EVT_MENU(ID_ModePolygon, MainFrame::OnModePolygon)
    EVT_MENU(ID_ModeRectangleCam, MainFrame::OnModeRectangleCam)
    EVT_MENU(ID_ModePolygonCam, MainFrame::OnModePolygonCam)
    EVT_MENU(wxID_EXIT, MainFrame::OnExit)
    EVT_MENU(ID_ResetView, MainFrame::OnResetView)
    EVT_MENU(ID_IncreasePointSize, MainFrame::OnIncreasePointSize)
    EVT_MENU(ID_DecreasePointSize, MainFrame::OnDecreasePointSize)
    EVT_MENU(ID_IncreaseCamSize, MainFrame::OnIncreaseCamSize)
    EVT_MENU(ID_DecreaseCamSize, MainFrame::OnDecreaseCamSize)
	EVT_MENU(ID_About, MainFrame::OnAbout)
wxEND_EVENT_TABLE()

MainFrame::MainFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(1024, 768))
{
    m_menuBar = new wxMenuBar();
    wxMenu* fileMenu = new wxMenu;
    fileMenu->Append(ID_OpenColmap, "Import COLMAP Files");
    fileMenu->Append(ID_ExportColmap, "Export COLMAP Files");
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT, "Exit");
    m_menuBar->Append(fileMenu, "File");

    wxMenu* cursorMenu = new wxMenu;
    cursorMenu->Append(ID_ModeNormal, "Normal Mode(N)");
    cursorMenu->Append(ID_ModeRectangle, "Rectangle Select Point Mode(R)");
    cursorMenu->Append(ID_ModePolygon, "Polygon Select Point Mode(P)");
    cursorMenu->Append(ID_ModeRectangleCam, "Rectangle Select Camera Mode(Ctrl+R)");
    cursorMenu->Append(ID_ModePolygonCam, "Polygon Select Camera Mode(Ctrl+P)");
    m_menuBar->Append(cursorMenu, "Select");

    wxMenu* viewMenu = new wxMenu;
    viewMenu->Append(ID_ResetView, "Reset view(Esc)");
    viewMenu->Append(ID_IncreasePointSize, "Increase point size(+)");
    viewMenu->Append(ID_DecreasePointSize, "Decrease point size(-)");
    viewMenu->Append(ID_IncreaseCamSize, "Increase camera size(\u2191)");
    viewMenu->Append(ID_DecreaseCamSize, "Decrease camera size(\u2193)");
    m_menuBar->Append(viewMenu, "View");

    wxMenu* editMenue = new wxMenu;
    editMenue->Append(ID_InvertSelected, "Invert Selected(V)");
    editMenue->Append(ID_DeleteSelected, "Delete Selected(Del)");
    m_menuBar->Append(editMenue, "Edit");
	
	wxMenu* helpMenu = new wxMenu;
    helpMenu->Append(ID_About, "About");
    m_menuBar->Append(helpMenu, "Help");

    SetMenuBar(m_menuBar);

    m_panel = new wxPanel(this);
    m_sizer = new wxBoxSizer(wxVERTICAL);
    m_canvas = new OSGCanvas(m_panel);
    m_sizer->Add(m_canvas, 1, wxEXPAND | wxALL, 5);
    m_panel->SetSizer(m_sizer);
}

void MainFrame::OnAbout(wxCommandEvent& event)
{
    wxAboutDialogInfo info;
    info.SetName("ColmapEditor");
    info.SetVersion("0.1.0");
    info.SetDescription("A lightweight point cloud and camera editor for COLMAP.");
    info.SetCopyright("(C) 2025 Deyan Deng");

    // Author and contact
    //info.AddDeveloper("Deyan Deng");
    info.SetWebSite("mailto:dengdeyan@gmail.com", "Email: dengdeyan@gmail.com");

    wxAboutBox(info,this);
}

void MainFrame::OnOpenColmapFiles(wxCommandEvent& event) {
    wxDirDialog dirDialog(this, "Select COLMAP sparse directory", "", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
    if (dirDialog.ShowModal() == wxID_CANCEL) return;
    wxString dirPath = dirDialog.GetPath();

    wxString pointsPath = dirPath + "\\points3D.txt";
    wxString camerasPath = dirPath + "\\cameras.txt";
    wxString imagesPath = dirPath + "\\images.txt";

    if (m_scene) delete m_scene;
    m_scene = new Scene();
    bool ok = m_scene->Import(pointsPath.ToStdString(), camerasPath.ToStdString(), imagesPath.ToStdString());
    if (!ok) {
        wxMessageBox("Failed to import COLMAP files.", "Error", wxICON_ERROR);
        delete m_scene;
        m_scene = nullptr;
        return;
    }
    if (m_canvas && m_scene) {
        m_canvas->SetScene(m_scene);
    }
}

void MainFrame::OnExportColmapFiles(wxCommandEvent& event) {
    if (!m_scene) {
        wxMessageBox("No scene loaded.", "Error", wxICON_ERROR);
        return;
    }

    wxDirDialog dirDialog(this, "Select COLMAP sparse directory", "", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
    if (dirDialog.ShowModal() == wxID_CANCEL) return;
    wxString dirPath = dirDialog.GetPath();
    wxString pointsPath = dirPath + "\\points3D.txt";
    wxString camerasPath = dirPath + "\\cameras.txt";
    wxString imagesPath = dirPath + "\\images.txt";

    bool ok = m_scene->Export(pointsPath.ToStdString(), camerasPath.ToStdString(), imagesPath.ToStdString());
    if (!ok) {
        wxMessageBox("Failed to export COLMAP files.", "Error", wxICON_ERROR);
        return;
    }
}

void MainFrame::OnIncreasePointSize(wxCommandEvent& event)
{
    m_canvas->ScalePoint(1);
}

void MainFrame::OnDecreasePointSize(wxCommandEvent& event)
{
    m_canvas->ScalePoint(-1);
}

void MainFrame::OnIncreaseCamSize(wxCommandEvent& event)
{
    m_canvas->ScaleCamera(1);
}

void MainFrame::OnDecreaseCamSize(wxCommandEvent& event)
{
    m_canvas->ScaleCamera(-1);
}


void MainFrame::OnDeleteSelected(wxCommandEvent& event) {
    // TODO: Delete selected points in OSGCanvas
    m_canvas->DeleteSelected();
}

void MainFrame::OnInvertSelected(wxCommandEvent& event)
{
    m_canvas->InvertSelected();
}

void MainFrame::OnResetView(wxCommandEvent& event)
{
    m_canvas->ResetView();
}

void MainFrame::OnExit(wxCommandEvent& event) {
    if (m_scene) {
        delete m_scene;
        m_scene = nullptr;
    }
    Close(true);
}


void MainFrame::OnModeNormal(wxCommandEvent& event) {
    if (m_canvas) m_canvas->SetCursorMode(OSGCanvas::MODE_NORMAL);
}

void MainFrame::OnModeRectangle(wxCommandEvent& event) {
    if (m_canvas) m_canvas->SetCursorMode(OSGCanvas::MODE_RECTANGLE);
}

void MainFrame::OnModePolygon(wxCommandEvent& event) {
    if (m_canvas) m_canvas->SetCursorMode(OSGCanvas::MODE_POLYGON);
}


void MainFrame::OnModeRectangleCam(wxCommandEvent& event) {
    if (m_canvas) m_canvas->SetCursorMode(OSGCanvas::MODE_RECTANGLE_CAMERA);
}

void MainFrame::OnModePolygonCam(wxCommandEvent& event) {
    if (m_canvas) m_canvas->SetCursorMode(OSGCanvas::MODE_POLYGON_CAMERA);
}
