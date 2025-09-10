    
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
#include <osg/BlendFunc>

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
    m_viewer->getCamera()->setClearColor(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f)); // dark background

    long style = GetWindowStyle();
    style |= wxWANTS_CHARS;
    SetWindowStyle(style);
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
    else if (mode == MODE_RECTANGLE || mode==MODE_RECTANGLE_CAMERA) SetCursor(wxCURSOR_CROSS);
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
        if (event.ControlDown())
        {
            SetCursorMode(MODE_RECTANGLE_CAMERA);
        }
        else
        {
            SetCursorMode(MODE_RECTANGLE);
        }
        Refresh(false);
        break;
    }
    case 'p':
    case 'P':
    {
        if (event.ControlDown())
        {
            SetCursorMode(MODE_POLYGON_CAMERA);
        }
        else
        {
            SetCursorMode(MODE_POLYGON);
        }
        Refresh(false);
        break;
    }
    case 'v':
    case 'V':
    {
        InvertSelected();
        break;
    }
    case 'n':
    case 'N':
    {
        SetCursorMode(MODE_NORMAL);
        break;
    }
    case 'd':
    case 'D':
    case WXK_DELETE:
    {
        DeleteSelected();
        break;
    }
    case '+':
    case WXK_NUMPAD_ADD:
    {
        ScalePoint(1);
        break;
    }
    case '-':
    case WXK_NUMPAD_SUBTRACT:
    {
        ScalePoint(-1);
        break;
    }
    case WXK_ESCAPE:
    {
        ResetView();
        break;
    }
    case WXK_UP:
    {
        ScaleCamera(1);
        break;
    }
    case WXK_DOWN:
    {
        ScaleCamera(-1);
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


void OSGCanvas::DeleteSelected() {
    // TODO: Remove selected points from scene and data
    if (m_scene == nullptr) return;
    if (lastSelectMode == MODE_RECTANGLE || lastSelectMode == MODE_POLYGON)
    {
        m_scene->DeletePoints(selectedPoints);
        selectedPoints.clear();
    }
    else if (lastSelectMode == MODE_RECTANGLE_CAMERA || lastSelectMode == MODE_POLYGON_CAMERA)
    {
        m_scene->DeleteImages(selectedCameras);
        selectedCameras.clear();
    }
    UpdateSceneGraph(false);
    Refresh();
}

void OSGCanvas::InvertSelected()
{
    if (m_scene == nullptr) return;
    if (lastSelectMode == MODE_RECTANGLE || lastSelectMode == MODE_POLYGON)
    {
        int npt = m_scene->GetPoints().size();
        std::vector<char> flags(npt, 1);
        for(int i: selectedPoints)
        {
            flags[i] = 0;
        }
        selectedPoints.clear();
        for (int i = 0; i < npt; i++)
        {
            if (flags[i]) selectedPoints.push_back(i);
        }
    }
    else if (lastSelectMode == MODE_RECTANGLE_CAMERA || lastSelectMode == MODE_POLYGON_CAMERA)
    {
        int npt = m_scene->GetImages().size();
        std::vector<char> flags(npt, 1);
        for(int i: selectedCameras)
        {
            flags[i] = 0;
        }
        selectedCameras.clear();
        for (int i = 0; i < npt; i++)
        {
            if (flags[i]) selectedCameras.push_back(i);
        }
    }
    UpdateSelect();
}

void OSGCanvas::ResetView()
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
        Refresh(false);
    }
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
    } else if (m_cursorMode == MODE_RECTANGLE || m_cursorMode == MODE_RECTANGLE_CAMERA) {
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
            SelectObjectsInPolygon(polygonPoints);
            
        }
    } else if (m_cursorMode == MODE_POLYGON || m_cursorMode == MODE_POLYGON_CAMERA) {
        if (event.LeftDown()) {
            polygonPoints.push_back({x, y});
            polygonDrawing = true;
            DrawPolygon();
        } else if (event.RightDown() && polygonDrawing) {
            polygonDrawing = false;
            // Calculate selection in polygon
            SelectObjectsInPolygon(polygonPoints);
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

void OSGCanvas::ScalePoint(int delta)
{
    if (delta > 0) pointSize *= 2.0f;
    else pointSize *= 0.5f;
    if (pointsGeode.valid())
    {
        osg::Geode* geode = pointsGeode.get();
        if (geode && geode->getNumDrawables() > 0) {
            osg::Geometry* geom = dynamic_cast<osg::Geometry*>(geode->getDrawable(0));
            if (geom) {
                osg::ref_ptr<osg::Point> pointSizer = new osg::Point(pointSize); // 3 pixels
                geom->getOrCreateStateSet()->setAttribute(pointSizer.get());
                geom->dirtyBound();
                Refresh(false);
            }
        }
    }
}

void OSGCanvas::DrawCameras()
{
    if (camerasGeode.valid())
    {
        m_root->removeChild(camerasGeode);
        camerasGeode.release();
    }
    if (m_scene->GetImages().size() == 0) return;
    std::vector<char> flags(m_scene->GetImages().size(),0);
    for (int index : selectedCameras)
    {
        flags[index] = 1;
    }
    double scale = 0.2;
    double minx=FLT_MAX, maxx=-FLT_MAX, miny=FLT_MAX, maxy=-FLT_MAX;
    std::vector<osg::Vec3d> Cs;
    std::vector<osg::Matrix> Rs;
    for (auto it = m_scene->GetImages().begin(); it != m_scene->GetImages().end(); ++it) {
        const Image& img = it->second;
        osg::Quat q(img.qvec[1], img.qvec[2], img.qvec[3], img.qvec[0]);
        osg::Matrix Rt;
        Rt.makeRotate(q);   // R is camera-to-world rotation? NO, it's world-to-camera!
        // Camera center in world coords
        osg::Vec3d t(img.tvec[0], img.tvec[1], img.tvec[2]);
        osg::Vec3d C = -(Rt * t);   // C = -R^T * t
        Cs.push_back(C);
        osg::Matrix R;// = osg::Matrix::transpose(R);  // world-from-camera rotation
        R.transpose(Rt);
        Rs.push_back(R);
        if (C.x() < minx) minx = C.x();
        if (C.x() > maxx) maxx = C.x();
        if (C.y() < miny) miny = C.y();
        if (C.y() > maxy) maxy = C.y();
    }
    double dx = maxx - minx;
    double dy = maxy - miny;
    scale = 0.5*std::sqrt(dx*dx+dy*dy) * cameraSize; // Size of pyramid
    camerasGeode = new osg::Geode;
    for (int i = 0; i < Rs.size();i++) {
        osg::ref_ptr<osg::Geometry> camGeom = new osg::Geometry;
        osg::ref_ptr<osg::Vec3Array> camVerts = new osg::Vec3Array;
        osg::ref_ptr<osg::Vec4Array> camColors = new osg::Vec4Array;

        // Define pyramid base in camera local coordinates
        std::vector<osg::Vec3d> base = {
            osg::Vec3d(-scale, -scale, scale),
            osg::Vec3d(scale, -scale, scale),
            osg::Vec3d(scale,  scale, scale),
            osg::Vec3d(-scale,  scale, scale)
        };
        // Transform base to world coordinates
        osg::Matrix R=Rs[i];// = osg::Matrix::transpose(R);  // world-from-camera rotation
        osg::Vec3d C = Cs[i];   // C = -R^T * t

        for (auto& v : base) v = R * v + C;
        // Apex
        osg::Vec3d apex = C;
        // Add vertices
        camVerts->push_back(apex); // 0
        for (int j = 0; j < 4; ++j) camVerts->push_back(base[j]); // 1-4
        // Color: green
        osg::Vec4 camColor(1, 0, 0, 1);
        osg::Vec4 planeColor(1, 0.2, 0.2, 0.3f);
        if (flags[i])
        {
            camColor = { 0, 0, 1, 1 };
            planeColor = { 0.2, 0.2, 1.0, 0.3f };
        }
        for (int j = 0; j < 5; ++j) camColors->push_back(camColor);
        camGeom->setVertexArray(camVerts.get());
        camGeom->setColorArray(camColors.get(), osg::Array::BIND_PER_VERTEX);

        // Draw base square
        osg::ref_ptr<osg::DrawElementsUInt> baseLines = new osg::DrawElementsUInt(osg::PrimitiveSet::LINE_LOOP, 0);
        baseLines->push_back(1); baseLines->push_back(2); baseLines->push_back(3); baseLines->push_back(4);
        camGeom->addPrimitiveSet(baseLines.get());
        // Draw sides
        for (int j = 1; j <= 4; ++j) {
            osg::ref_ptr<osg::DrawElementsUInt> side = new osg::DrawElementsUInt(osg::PrimitiveSet::LINES, 0);
            side->push_back(0); side->push_back(j);
            camGeom->addPrimitiveSet(side.get());
        }

        osg::ref_ptr<osg::StateSet> ssCam = camGeom->getOrCreateStateSet();
        // Enable blending
        ssCam->setMode(GL_BLEND, osg::StateAttribute::ON);
        // Set blending function (optional, commonly used for standard alpha blending)
        ssCam->setAttributeAndModes(new osg::BlendFunc(
            osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA));

        osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth(2.0); // 3 pixels
        ssCam->setAttribute(lineWidth.get());
        ssCam->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
        ssCam->setMode(GL_DEPTH_WRITEMASK, osg::StateAttribute::OFF);
        ssCam->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

        // --- Image plane rectangle (filled) ---
        osg::ref_ptr<osg::Geometry> planeGeom = new osg::Geometry;
        osg::ref_ptr<osg::Vec3Array> planeVerts = new osg::Vec3Array;
        for (int j = 0; j < 4; ++j)
            planeVerts->push_back(base[j]);

        osg::ref_ptr<osg::Vec4Array> planeColors = new osg::Vec4Array;
        for (int j = 0; j < 4; ++j)
            planeColors->push_back(planeColor); // semi-transparent green

        planeGeom->setVertexArray(planeVerts.get());
        planeGeom->setColorArray(planeColors.get(), osg::Array::BIND_PER_VERTEX);
        planeGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4));

        // State for transparency
        osg::StateSet* ssPlane = planeGeom->getOrCreateStateSet();
        ssPlane->setMode(GL_BLEND, osg::StateAttribute::ON);
        ssPlane->setAttributeAndModes(new osg::BlendFunc(
            osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA));
        ssPlane->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
        ssPlane->setMode(GL_DEPTH_WRITEMASK, osg::StateAttribute::OFF);
        ssPlane->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

        camerasGeode->addDrawable(camGeom.get());
        camerasGeode->addDrawable(planeGeom.get());
    }
    m_root->addChild(camerasGeode.get());
    Refresh(false);
}

void OSGCanvas::ScaleCamera(int delta)
{
    if (delta > 0) cameraSize *= 2.0f;
    else cameraSize *= 0.5f;
    DrawCameras();
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
    if ((m_cursorMode == MODE_RECTANGLE || m_cursorMode == MODE_RECTANGLE_CAMERA) && dragging)
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
        
        osg::StateSet* ss = polyGeom->getOrCreateStateSet();
        ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        ss->setMode(GL_BLEND, osg::StateAttribute::ON);
        osg::ref_ptr<osg::LineWidth> lw = new osg::LineWidth(2.0f);
        ss->setAttributeAndModes(lw, osg::StateAttribute::ON);

        hudGeode->addDrawable(polyGeom.get());
        hudCamera->addChild(hudGeode.get());
        m_root->addChild(hudCamera.get());
        Refresh();
    }
    if ((m_cursorMode == MODE_POLYGON || m_cursorMode==MODE_POLYGON_CAMERA )&& polygonDrawing)
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
        osg::StateSet* ss = polyGeom->getOrCreateStateSet();
        ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        ss->setMode(GL_BLEND, osg::StateAttribute::ON);
        osg::ref_ptr<osg::LineWidth> lw = new osg::LineWidth(2.0f);
        ss->setAttributeAndModes(lw, osg::StateAttribute::ON);
        hudGeode->addDrawable(polyGeom.get());
        hudCamera->addChild(hudGeode.get());
        m_root->addChild(hudCamera.get());
        Refresh();
    }
}

void OSGCanvas::DrawPoints()
{
    if (pointsGeode.valid())
    {
        m_root->removeChild(pointsGeode);
        pointsGeode.release();
    }
    pointsGeode = new osg::Geode;
    osg::ref_ptr<osg::Geometry> pointsGeom = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    for (auto it = m_scene->GetPoints().begin(); it != m_scene->GetPoints().end(); ++it) {
        const Point3D& pt = it->second;
        vertices->push_back(osg::Vec3(pt.x, pt.y, pt.z));
        colors->push_back(osg::Vec4(pt.color[0] / 255.0, pt.color[1] / 255.0, pt.color[2] / 255.0, 1.0));
    }
    pointsGeom->setVertexArray(vertices.get());
    pointsGeom->setColorArray(colors.get(), osg::Array::BIND_PER_VERTEX);
    pointsGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size()));
    osg::ref_ptr<osg::Point> pointSizer = new osg::Point(pointSize); // 3 pixels
    pointsGeom->getOrCreateStateSet()->setAttribute(pointSizer.get());
    pointsGeom->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    pointsGeode->addDrawable(pointsGeom.get());
    m_root->addChild(pointsGeode.get());
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
                for (int i : selectedPoints) flags[i] = 1;
                int index = 0;
                for (auto it = m_scene->GetPoints().begin(); it != m_scene->GetPoints().end(); ++it, index++) {
                    const Point3D& pt = it->second;
                    osg::Vec4& c = (*colors)[index];
                    if (flags[index])
                    {
                        c[0] = 0;
                        c[1] = 0;
                        c[2] = 255;
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
    //cameras
    DrawCameras();
}

void OSGCanvas::UpdateSceneGraph(bool reset) {
    if (!m_scene) return;
    // Add points as OSG geometry
    DrawPoints();
    // Add cameras as square pyramid wireframes
    DrawCameras();
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
void OSGCanvas::SelectObjectsInPolygon(const std::vector<Point2D>& polygon) {
    lastSelectMode = m_cursorMode;
    selectedPoints.clear();
    selectedCameras.clear();
    if (!m_scene || polygon.size() < 3) return;
    // Get viewport, projection, and modelview matrices
    osg::Matrixd projection = m_viewer->getCamera()->getProjectionMatrix();
    osg::Matrixd modelview = m_viewer->getCamera()->getViewMatrix();
    osg::Matrixd viewport = m_viewer->getCamera()->getViewport()->computeWindowMatrix();
    osg::Matrixd mat = modelview * projection * viewport;
    if (lastSelectMode == MODE_RECTANGLE || lastSelectMode == MODE_POLYGON)
    {
        int index = 0;
        int w, h;
        GetClientSize(&w, &h);
        for (auto it = m_scene->GetPoints().begin(); it != m_scene->GetPoints().end(); ++it, index++) {
            const Point3D& pt = it->second;
            osg::Vec3d obj(pt.x, pt.y, pt.z);
            osg::Vec3d win = mat.preMult(obj);
            if (PointInPolygon(win.x(), h - win.y(), polygon)) {
                selectedPoints.push_back(index);
            }
        }
    }
    else
    {
        std::vector<osg::Vec3d> Cs;
        for (auto it = m_scene->GetImages().begin(); it != m_scene->GetImages().end(); ++it) {
            const Image& img = it->second;
            osg::Quat q(img.qvec[1], img.qvec[2], img.qvec[3], img.qvec[0]);
            osg::Matrix Rt;
            Rt.makeRotate(q);   // R is camera-to-world rotation? NO, it's world-to-camera!
            // Camera center in world coords
            osg::Vec3d t(img.tvec[0], img.tvec[1], img.tvec[2]);
            osg::Vec3d C = -(Rt * t);   // C = -R^T * t
            Cs.push_back(C);
        }
        int index = 0;
        int w, h;
        GetClientSize(&w, &h);
        for (auto it = m_scene->GetImages().begin(); it != m_scene->GetImages().end(); ++it, index++) {
            const Image& img = it->second;
            osg::Quat q(img.qvec[1], img.qvec[2], img.qvec[3], img.qvec[0]);
            osg::Matrix Rt;
            Rt.makeRotate(q);   // R is camera-to-world rotation? NO, it's world-to-camera!
            // Camera center in world coords
            osg::Vec3d t(img.tvec[0], img.tvec[1], img.tvec[2]);
            osg::Vec3d obj = -(Rt * t);   // C = -R^T * t
            osg::Vec3d win = mat.preMult(obj);
            if (PointInPolygon(win.x(), h - win.y(), polygon)) {
                selectedCameras.push_back(index);
            }
        }
    }
    polygonPoints.clear();
    SetCursorMode(MODE_NORMAL);
    DrawPolygon();
    UpdateSelect();
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
