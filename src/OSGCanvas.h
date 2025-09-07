#pragma once
#include <wx/wx.h>
#include <wx/glcanvas.h>
#include <osgViewer/Viewer>
#include <osgViewer/GraphicsWindow>
#include <osg/Group>
#include "Scene.h"

class OSGCanvas : public wxGLCanvas {
public:
    enum CursorMode {
        MODE_NORMAL,
        MODE_RECTANGLE,
        MODE_POLYGON
    };
    struct Point2D { int x, y; };
    void SetCursorMode(CursorMode mode);
    CursorMode GetCursorMode() const { return m_cursorMode; }
public:
    OSGCanvas(wxWindow* parent);
    void SetScene(class Scene* scene);
    void DeleteSelectedPoints();
    void InvertSelectedPoitns();
    void ResetView();
    void SelectPointsInPolygon(const std::vector<Point2D>& polygon);
    void SetContextCurrent();
    void DrawPolygon();
    void ScalePoint(int delta);
protected:
    void OnPaint(wxPaintEvent& event);
    void OnMouse(wxMouseEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnIdle(wxIdleEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);
    void Render();
    void UpdateSceneGraph(bool reset=true);
    void UpdateSelect();

    osg::ref_ptr<osgViewer::Viewer> m_viewer;
    osg::ref_ptr<osg::Group> m_root;
    class Scene* m_scene = nullptr;
    CursorMode m_cursorMode = MODE_NORMAL;

    wxGLContext* m_glContext;
    osg::ref_ptr<osgViewer::GraphicsWindow> m_gc;
    bool dragging = false;
    Point2D rectStart, rectEnd;
    std::vector<Point2D> polygonPoints;
    bool polygonDrawing = false;

    std::vector<int> selected;
    osg::ref_ptr<osg::Geode> camerasGeode;
    osg::ref_ptr<osg::Geode> pointsGeode;
    osg::ref_ptr<osg::Camera> hudCamera;

    float pointSize = 3.0f;

    wxDECLARE_EVENT_TABLE();
};

class GraphicsWindowWX : public osgViewer::GraphicsWindow
{
public:
    GraphicsWindowWX(OSGCanvas* canvas);
    ~GraphicsWindowWX();

    void init();

    //
    // GraphicsWindow interface
    //
    void grabFocus();
    void grabFocusIfPointerInWindow();
    void useCursor(bool cursorOn);

    bool makeCurrentImplementation();
    void swapBuffersImplementation();

    // not implemented yet...just use dummy implementation to get working.
    virtual bool valid() const { return true; }
    virtual bool realizeImplementation() { return true; }
    virtual bool isRealizedImplementation() const { return _canvas->IsShownOnScreen(); }
    virtual void closeImplementation() {}
    virtual bool releaseContextImplementation() { return true; }

private:
    // XXX need to set _canvas to NULL when the canvas is deleted by
    // its parent. for this, need to add event handler in OSGCanvas
    OSGCanvas* _canvas;
    wxCursor _oldCursor;
};
