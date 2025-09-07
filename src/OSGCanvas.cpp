    
#include <osg/Matrix>
#include <osg/Camera>
#include "OSGCanvas.h"
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <vector>
#include <wx/dcclient.h>
#include <osg/ComputeBoundsVisitor>
#include <osg/Point>
#include <osg/LineWidth>

wxBEGIN_EVENT_TABLE(OSGCanvas, wxGLCanvas)
    EVT_PAINT(OSGCanvas::OnPaint)
    EVT_MOUSE_EVENTS(OSGCanvas::OnMouse)
    EVT_SIZE(OSGCanvas::OnSize)
    EVT_KEY_DOWN(OSGCanvas::OnKeyDown)
    EVT_KEY_UP(OSGCanvas::OnKeyUp)
wxEND_EVENT_TABLE()

OSGCanvas::OSGCanvas(wxWindow* parent)
    : wxGLCanvas(parent, wxID_ANY, nullptr)
{
    m_gc = new GraphicsWindowWX(this);
    m_glContext = new wxGLContext(this);
    int w, h;
    GetClientSize(&w, &h);
    m_viewer = new osgViewer::Viewer;
    m_root = new osg::Group;
    m_viewer->setSceneData(m_root.get());
    m_viewer->setCameraManipulator(new osgGA::TrackballManipulator);
    m_viewer->getCamera()->setGraphicsContext(m_gc.get());
    m_viewer->getCamera()->setViewport(new osg::Viewport(0, 0, w, h));
    m_viewer->getCamera()->setProjectionMatrixAsPerspective(45.0f, static_cast<double>(w)/h, 0.1, 1000.0);
    m_viewer->getCamera()->setClearColor(osg::Vec4(0.8f, 0.8f, 0.8f, 1.0f)); // dark background
}

void OSGCanvas::SetContextCurrent()
{
    m_glContext->SetCurrent(*this);
}


void OSGCanvas::SetScene(Scene* scene) {
    m_scene = scene;
    UpdateSceneGraph();
    Refresh();
}

void OSGCanvas::SetCursorMode(CursorMode mode)
{
    m_cursorMode = mode;
    if (mode == MODE_NORMAL) SetCursor(wxCURSOR_ARROW);
    else if (mode == MODE_RECTANGLE) SetCursor(wxCURSOR_CROSS);
    else SetCursor(wxCURSOR_HAND);
}

void OSGCanvas::OnKeyDown(wxKeyEvent& event)
{
    int key = event.GetKeyCode();
    switch (key)
    {
    case 'r':
    case 'R':
    {
        SetCursorMode(MODE_RECTANGLE);
        Refresh(false);
        break;
    }
    case 'p':
    case 'P':
    {
        SetCursorMode(MODE_POLYGON);
        Refresh(false);
        break;
    }
    case 'v':
    case 'V':
    {
        InvertSelectedPoitns();
        break;
    }
    case 'd':
    case 'D':
    case WXK_DELETE:
    {
        DeleteSelectedPoints();
        break;
    }
    default:
        break;
    }
}

void OSGCanvas::OnKeyUp(wxKeyEvent& event)
{
    event.Skip();
}


void OSGCanvas::DeleteSelectedPoints() {
    // TODO: Remove selected points from scene and data
    if (m_scene == nullptr) return;
    m_scene->DeletePoints(selected);
    UpdateSceneGraph(false);
    Refresh();
}

void OSGCanvas::InvertSelectedPoitns()
{
    if (m_scene == nullptr) return;
    int npt = m_scene->GetPoints().size();
    std::vector<char> flags(npt, 1);
    for (int i : selected)
    {
        flags[i] = 0;
    }
    selected.clear();
    for (int i = 0; i < npt; i++)
    {
        if (flags[i]) selected.push_back(i);
    }
    UpdateSelect();
}


void OSGCanvas::OnPaint(wxPaintEvent& event) {
    wxPaintDC dc(this);
    Render();
}


void OSGCanvas::OnMouse(wxMouseEvent& event) {
    int x = event.GetX();
    int y = event.GetY();

    if (m_cursorMode == MODE_NORMAL) {
        /*if (event.LeftDClick()) {
            // Double click: select nearest point
            // TODO: Implement 2D to 3D picking
        } else if (event.LeftDown()) {
            dragging = true;
        } else if (event.LeftUp()) {
            dragging = false;
        } else if (event.RightDown()) {
            dragging = true;
        } else if (event.RightUp()) {
            dragging = false;
        } else if (event.Dragging() && dragging) {
            if (event.LeftIsDown()) {
                // Left drag: rotate
                // TODO: Integrate with OSG trackball manipulator
            } else if (event.RightIsDown()) {
                // Right drag: move
                // TODO: Integrate with OSG manipulator for panning
            }
        }*/
        if (event.LeftDown())
            m_gc->getEventQueue()->mouseButtonPress(x, y, 1);
        else if (event.LeftUp())
            m_gc->getEventQueue()->mouseButtonRelease(x, y, 1);
        else if (event.RightDown())
            m_gc->getEventQueue()->mouseButtonPress(x, y, 3);
        else if (event.RightUp())
            m_gc->getEventQueue()->mouseButtonRelease(x, y, 3);
        else if (event.MiddleDown())
            m_gc->getEventQueue()->mouseButtonPress(x, y, 2);
        else if (event.MiddleUp()) {
            m_gc->getEventQueue()->mouseButtonRelease(x, y, 2);
        }
        else if (event.Dragging())
        {
            m_gc->getEventQueue()->mouseMotion(x, y);
            Refresh();
        }
        else if (event.Moving())
        {
            m_gc->getEventQueue()->mouseMotion(x, y);
        }
        else if (event.GetWheelRotation() != 0) {
            if (event.GetWheelRotation() > 0)
                m_gc->getEventQueue()->mouseScroll(osgGA::GUIEventAdapter::SCROLL_UP);
            else
                m_gc->getEventQueue()->mouseScroll(osgGA::GUIEventAdapter::SCROLL_DOWN);
            Refresh();
        }
    } else if (m_cursorMode == MODE_RECTANGLE) {
        if (event.LeftDown()) {
            rectStart = {x, y};
            rectEnd = rectStart;
            dragging = true;
        } else if (event.Dragging() && dragging && event.LeftIsDown()) {
            rectEnd = {x, y};
            DrawPolygon();
        } else if (event.LeftUp() && dragging) {
            dragging = false;
            // Calculate selection in rectangle
            // TODO: Implement 2D rectangle selection logic
            polygonPoints.clear();
            polygonPoints.push_back({ rectStart.x, rectStart.y }); // OSG Y=bottom
            polygonPoints.push_back({ rectEnd.x, rectStart.y }); // OSG Y=bottom
            polygonPoints.push_back({ rectEnd.x, rectEnd.y }); // OSG Y=bottom
            polygonPoints.push_back({ rectStart.x, rectEnd.y }); // OSG Y=bottom
            SelectPointsInPolygon(polygonPoints);
            polygonPoints.clear();
            DrawPolygon();
            SetCursorMode(MODE_NORMAL);
            UpdateSelect();
        }
    } else if (m_cursorMode == MODE_POLYGON) {
        if (event.LeftDown()) {
            polygonPoints.push_back({x, y});
            polygonDrawing = true;
            DrawPolygon();
        } else if (event.RightDown() && polygonDrawing) {
            polygonDrawing = false;
            // Calculate selection in polygon
            SelectPointsInPolygon(polygonPoints);
            // TODO: Implement 2D polygon selection logic
            polygonPoints.clear();
            DrawPolygon();
            SetCursorMode(MODE_NORMAL);
            UpdateSelect();
        }
    }
}


void OSGCanvas::OnSize(wxSizeEvent& event) {
    int w, h;
    GetClientSize(&w, &h);
    if (m_viewer && m_viewer->getCamera()) {
        m_viewer->getCamera()->setViewport(0, 0, w, h);
        m_viewer->getCamera()->setProjectionMatrixAsPerspective(45.0f, static_cast<double>(w)/h, 0.1, 1000.0);
    }
    if (m_gc.valid())
    {
        // update the window dimensions, in case the window has been resized.
        m_gc->getEventQueue()->windowResize(0, 0, w, h);
        m_gc->resized(0, 0, w, h);
    }
    Refresh();
}


void OSGCanvas::Render() {
    if (m_viewer.valid()) m_viewer->frame();
}

void OSGCanvas::DrawPolygon()
{
    // Remove previous HUD camera if any
    if (hudCamera.valid()) {
        m_root->removeChild(hudCamera.get());
        hudCamera.release();
        Refresh(false);
    }

    // Draw 2D polygon overlay using HUD camera if in polygon mode and points exist
    if (m_cursorMode == MODE_RECTANGLE && dragging)
    {
        int w = GetSize().GetWidth();
        int h = GetSize().GetHeight();
        hudCamera = new osg::Camera;
        hudCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
        hudCamera->setRenderOrder(osg::Camera::POST_RENDER);
        hudCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
        hudCamera->setProjectionMatrix(osg::Matrix::ortho2D(0, w, 0, h));
        hudCamera->setViewport(0, 0, w, h);

        osg::ref_ptr<osg::Geode> hudGeode = new osg::Geode;
        osg::ref_ptr<osg::Geometry> polyGeom = new osg::Geometry;
        osg::ref_ptr<osg::Vec3Array> polyVerts = new osg::Vec3Array;

        polyVerts->push_back(osg::Vec3(rectStart.x, h - rectStart.y, 0)); // OSG Y=bottom
        polyVerts->push_back(osg::Vec3(rectEnd.x, h - rectStart.y, 0)); // OSG Y=bottom
        polyVerts->push_back(osg::Vec3(rectEnd.x, h - rectEnd.y, 0)); // OSG Y=bottom
        polyVerts->push_back(osg::Vec3(rectStart.x, h - rectEnd.y, 0)); // OSG Y=bottom

        polyGeom->setVertexArray(polyVerts.get());
        osg::ref_ptr<osg::Vec4Array> polyColors = new osg::Vec4Array;
        polyColors->push_back(osg::Vec4(0, 0, 1, 1)); // blue
        polyGeom->setColorArray(polyColors.get(), osg::Array::BIND_OVERALL);
        polyGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_LOOP, 0, polyVerts->size()));
        osg::ref_ptr<osg::LineWidth> lw = new osg::LineWidth(2.0f);
        hudGeode->getOrCreateStateSet()->setAttributeAndModes(lw, osg::StateAttribute::ON);
        hudGeode->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
        hudGeode->addDrawable(polyGeom.get());
        hudCamera->addChild(hudGeode.get());
        m_root->addChild(hudCamera.get());
        Refresh();
    }
    if (m_cursorMode == MODE_POLYGON && polygonDrawing)
    {
        int w = GetSize().GetWidth();
        int h = GetSize().GetHeight();
        hudCamera = new osg::Camera;
        hudCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
        hudCamera->setRenderOrder(osg::Camera::POST_RENDER);
        hudCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
        hudCamera->setProjectionMatrix(osg::Matrix::ortho2D(0, w, 0, h));
        hudCamera->setViewport(0, 0, w, h);

        osg::ref_ptr<osg::Geode> hudGeode = new osg::Geode;
        osg::ref_ptr<osg::Geometry> polyGeom = new osg::Geometry;
        osg::ref_ptr<osg::Vec3Array> polyVerts = new osg::Vec3Array;
        for (const auto& pt : polygonPoints) {
            polyVerts->push_back(osg::Vec3(pt.x, h - pt.y, 0)); // OSG Y=bottom
        }
        polyGeom->setVertexArray(polyVerts.get());
        osg::ref_ptr<osg::Vec4Array> polyColors = new osg::Vec4Array;
        polyColors->push_back(osg::Vec4(0, 0, 1, 1)); // blue
        polyGeom->setColorArray(polyColors.get(), osg::Array::BIND_OVERALL);
        polyGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_LOOP, 0, polyVerts->size()));
        hudGeode->addDrawable(polyGeom.get());

        osg::ref_ptr<osg::LineWidth> lw = new osg::LineWidth(2.0f);
        hudGeode->getOrCreateStateSet()->setAttributeAndModes(lw, osg::StateAttribute::ON);
        hudGeode->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
        hudCamera->addChild(hudGeode.get());
        m_root->addChild(hudCamera.get());
        Refresh();
    }
}

void OSGCanvas::UpdateSelect()
{
    if (!pointsGeode.valid()) return;
    osg::Geode* geode = pointsGeode.get();  // your geode
    if (geode && geode->getNumDrawables() > 0) {
        osg::Geometry* geom = dynamic_cast<osg::Geometry*>(geode->getDrawable(0));
        if (geom) {
            osg::Vec4Array* colors = dynamic_cast<osg::Vec4Array*>(geom->getColorArray());
            if (colors) {
                // Now you can access or modify colors
                std::vector<char> flags(colors->size(),0);
                for (int i : selected) flags[i] = 1;
                int index = 0;
                for (auto it = m_scene->GetPoints().begin(); it != m_scene->GetPoints().end(); ++it, index++) {
                    const Point3D& pt = it->second;
                    osg::Vec4& c = (*colors)[index];
                    if (flags[index])
                    {
                        c[0] = 255;
                        c[1] = c[2] = 0;
                    }
                    else
                    {
                        c[0] = pt.color[0] / 255.0f;
                        c[1] = pt.color[1] / 255.0f;
                        c[2] = pt.color[2] / 255.0f;
                    }
                }
                colors->dirty();
                geom->dirtyDisplayList();
                Refresh(false);
            }
        }
    }
}

void OSGCanvas::UpdateSceneGraph(bool reset) {
    if (!m_scene) return;
    m_root->removeChildren(0, m_root->getNumChildren());
    // Add points as OSG geometry
    pointsGeode = new osg::Geode;
    osg::ref_ptr<osg::Geometry> pointsGeom = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    for (auto it = m_scene->GetPoints().begin(); it != m_scene->GetPoints().end(); ++it) {
        const Point3D& pt = it->second;
        vertices->push_back(osg::Vec3(pt.x, pt.y, pt.z));
        colors->push_back(osg::Vec4(pt.color[0]/255.0, pt.color[1]/255.0, pt.color[2]/255.0, 1.0));
    }
    pointsGeom->setVertexArray(vertices.get());
    pointsGeom->setColorArray(colors.get(), osg::Array::BIND_PER_VERTEX);
    pointsGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size()));
    osg::ref_ptr<osg::Point> pointSize = new osg::Point(3.0f); // 3 pixels
    pointsGeom->getOrCreateStateSet()->setAttribute(pointSize.get());
    pointsGeom->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    pointsGeode->addDrawable(pointsGeom.get());
    m_root->addChild(pointsGeode.get());

    // Add cameras as square pyramid wireframes
    camerasGeode = new osg::Geode;
    for (auto it = m_scene->GetImages().begin(); it != m_scene->GetImages().end(); ++it) {
        const Image& img = it->second;
        osg::ref_ptr<osg::Geometry> camGeom = new osg::Geometry;
        osg::ref_ptr<osg::Vec3Array> camVerts = new osg::Vec3Array;
        osg::ref_ptr<osg::Vec4Array> camColors = new osg::Vec4Array;

        // Camera center
        osg::Vec3d C(img.tvec[0], img.tvec[1], img.tvec[2]);
        double scale = 0.2; // Size of pyramid

        // Camera orientation: use quaternion if available, else identity
        osg::Quat q;
        if (img.qvec.size() == 4) {
            q.set(img.qvec[1], img.qvec[2], img.qvec[3], img.qvec[0]); // (x, y, z, w)
        }

        // Define pyramid base in camera local coordinates
        std::vector<osg::Vec3d> base = {
            osg::Vec3d(-scale, -scale, scale),
            osg::Vec3d( scale, -scale, scale),
            osg::Vec3d( scale,  scale, scale),
            osg::Vec3d(-scale,  scale, scale)
        };
        // Transform base to world coordinates
        for (auto& v : base) v = q * v + C;
        // Apex
        osg::Vec3d apex = q * osg::Vec3d(0, 0, 0) + C;

        // Add vertices
        camVerts->push_back(apex); // 0
        for (int i = 0; i < 4; ++i) camVerts->push_back(base[i]); // 1-4

        // Color: green
        for (int i = 0; i < 5; ++i) camColors->push_back(osg::Vec4(0,1,0,1));

        camGeom->setVertexArray(camVerts.get());
        camGeom->setColorArray(camColors.get(), osg::Array::BIND_PER_VERTEX);

        // Draw base square
        osg::ref_ptr<osg::DrawElementsUInt> baseLines = new osg::DrawElementsUInt(osg::PrimitiveSet::LINE_LOOP, 0);
        baseLines->push_back(1); baseLines->push_back(2); baseLines->push_back(3); baseLines->push_back(4);
        camGeom->addPrimitiveSet(baseLines.get());
        // Draw sides
        for (int i = 1; i <= 4; ++i) {
            osg::ref_ptr<osg::DrawElementsUInt> side = new osg::DrawElementsUInt(osg::PrimitiveSet::LINES, 0);
            side->push_back(0); side->push_back(i);
            camGeom->addPrimitiveSet(side.get());
        }
        camerasGeode->addDrawable(camGeom.get());
    }
    //m_root->addChild(camerasGeode.get());

    if (reset)
    {
        osg::ComputeBoundsVisitor cbv;
        m_root->accept(cbv);
        osg::BoundingBox bb = cbv.getBoundingBox();

        if (bb.valid())
        {
            osg::Vec3f center = bb.center();
            float radius = bb.radius();

            osg::Vec3f eye(center.x(), center.y() - radius * 3.0f, center.z() + radius * 1.0f);
            osg::Vec3f up(0.0f, 0.0f, 1.0f);

            auto manip = dynamic_cast<osgGA::TrackballManipulator*>(m_viewer->getCameraManipulator());
            if (manip) {
                manip->setHomePosition(eye, center, up);
                manip->home(0.0);
            }
        }
    }
    
    Refresh(false);
}


// Helper: Point-in-polygon test
static bool PointInPolygon(int x, int y, const std::vector<OSGCanvas::Point2D>& poly) {
    int n = poly.size();
    bool inside = false;
    for (int i = 0, j = n - 1; i < n; j = i++) {
        if (((poly[i].y > y) != (poly[j].y > y)) &&
            (x < (poly[j].x - poly[i].x) * (y - poly[i].y) / (poly[j].y - poly[i].y) + poly[i].x)) {
            inside = !inside;
        }
    }
    return inside;
}
void OSGCanvas::SelectPointsInPolygon(const std::vector<Point2D>& polygon) {
    selected.clear();
    if (!m_scene || polygon.size() < 3) return;

    // Get viewport, projection, and modelview matrices
    osg::Matrixd projection = m_viewer->getCamera()->getProjectionMatrix();
    osg::Matrixd modelview = m_viewer->getCamera()->getViewMatrix();
    osg::Matrixd viewport = m_viewer->getCamera()->getViewport()->computeWindowMatrix();
    osg::Matrixd mat = modelview * projection * viewport;

    int index = 0;
    int w, h;
    GetClientSize(&w, &h);
    for (auto it = m_scene->GetPoints().begin(); it != m_scene->GetPoints().end(); ++it,index++) {
        const Point3D& pt = it->second;
        osg::Vec3d obj(pt.x, pt.y, pt.z);
        osg::Vec3d win = mat.preMult(obj);
        if (PointInPolygon(win.x(), h-win.y(), polygon)) {
            selected.push_back(index);
        }
    }
}

GraphicsWindowWX::GraphicsWindowWX(OSGCanvas* canvas)
{
    _canvas = canvas;
    // default cursor to standard
    _oldCursor = *wxSTANDARD_CURSOR;

    _traits = new GraphicsContext::Traits;

    wxPoint pos = _canvas->GetPosition();
    wxSize  size = _canvas->GetSize();

    _traits->x = pos.x;
    _traits->y = pos.y;
    _traits->width = size.x;
    _traits->height = size.y;

    init();
}

GraphicsWindowWX::~GraphicsWindowWX()
{
}

void GraphicsWindowWX::init()
{
    if (valid())
    {
        setState(new osg::State);
        getState()->setGraphicsContext(this);

        if (_traits.valid() && _traits->sharedContext.valid())
        {
            getState()->setContextID(_traits->sharedContext->getState()->getContextID());
            incrementContextIDUsageCount(getState()->getContextID());
        }
        else
        {
            getState()->setContextID(osg::GraphicsContext::createNewContextID());
        }
    }
}

void GraphicsWindowWX::grabFocus()
{
    // focus the canvas
    _canvas->SetFocus();
}

void GraphicsWindowWX::grabFocusIfPointerInWindow()
{
    // focus this window, if the pointer is in the window
    wxPoint pos = wxGetMousePosition();
    if (wxFindWindowAtPoint(pos) == _canvas)
        _canvas->SetFocus();
}

void GraphicsWindowWX::useCursor(bool cursorOn)
{
    //_canvas->UseCursor(cursorOn);
    if (cursorOn)
    {
        // show the old cursor
        _canvas->SetCursor(_oldCursor);
    }
    else
    {
        // remember the old cursor
        _oldCursor = _canvas->GetCursor();

        // hide the cursor
        //    - can't find a way to do this neatly, so create a 1x1, transparent image
        wxImage image(1, 1);
        image.SetMask(true);
        image.SetMaskColour(0, 0, 0);
        wxCursor cursor(image);
        _canvas->SetCursor(cursor);

        // On wxGTK, only works as of version 2.7.0
        // (http://trac.wxwidgets.org/ticket/2946)
        // SetCursor( wxStockCursor( wxCURSOR_BLANK ) );
    }
}

bool GraphicsWindowWX::makeCurrentImplementation()
{
    _canvas->SetContextCurrent();
    return true;
}

void GraphicsWindowWX::swapBuffersImplementation()
{
    _canvas->SwapBuffers();
}
