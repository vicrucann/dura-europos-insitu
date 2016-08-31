#include "Stroke.h"

#include <assert.h>

#include <QDebug>
#include <QtGlobal>
#include <QFile>

#include <osg/Program>
#include <osg/LineWidth>
#include <osg/StateSet>
#include <osg/BlendFunc>
#include <osg/Point>
#include <osg/Texture2D>

#include "ModelViewProjectionMatrixCallback.h"
#include "ViewportVectorCallback.h"
#include "CurveFitting/libPathFitter/OsgPathFitter.h"

entity::Stroke::Stroke()
    : entity::Entity2D()
    , m_lines(new osg::DrawArrays(GL_LINE_STRIP_ADJACENCY))
    , m_program(new osg::Program)
    , m_camera(0)
    , m_color(cher::STROKE_CLR_NORMAL)
    , m_isCurved(false)
    , m_isShadered(false)
{
    osg::Vec4Array* colors = new osg::Vec4Array;
    colors->push_back(m_color);
    osg::Vec3Array* verts = new osg::Vec3Array;

    this->addPrimitiveSet(m_lines.get());
    this->setVertexArray(verts);
    this->setColorArray(colors);
    this->setColorBinding(osg::Geometry::BIND_OVERALL);

    osg::StateSet* stateset = new osg::StateSet;
    osg::LineWidth* linewidth = new osg::LineWidth();
    linewidth->setWidth(cher::STROKE_LINE_WIDTH);
    osg::BlendFunc* blendfunc = new osg::BlendFunc();
    stateset->setAttributeAndModes(linewidth,osg::StateAttribute::ON);
    stateset->setAttributeAndModes(blendfunc, osg::StateAttribute::ON);
    stateset->setAttributeAndModes(new osg::Point(cher::STROKE_LINE_WIDTH));
    stateset->setTextureAttributeAndModes(0, new osg::Texture2D, osg::StateAttribute::OFF);
    stateset->setMode(GL_LINE_SMOOTH, osg::StateAttribute::ON);
    stateset->setMode(GL_BLEND, osg::StateAttribute::ON);
    stateset->setMode(GL_LIGHTING,osg::StateAttribute::OFF);
    this->setStateSet(stateset);

    this->setDataVariance(osg::Object::DYNAMIC);
    this->setUseDisplayList(false);
    this->setUseVertexBufferObjects(true);
    this->setName("Stroke");

    qDebug("New stroke ctor complete");
}

entity::Stroke::Stroke(const entity::Stroke& copy, const osg::CopyOp& copyop)
    : entity::Entity2D(copy, copyop)
    , m_lines(copy.m_lines)
    , m_program(copy.m_program)
    , m_color(copy.m_color)
    , m_isCurved(copy.m_isCurved)
    , m_isShadered(copy.m_isShadered)
{
    qDebug("stroke copy ctor done");
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

void entity::Stroke::setLines(osg::DrawArrays* lines)
{
    m_lines = lines;
}

const osg::DrawArrays*entity::Stroke::getLines() const
{
    return m_lines.get();
}

void entity::Stroke::setColor(const osg::Vec4f& color)
{
    m_color = color;
    osg::Vec4Array* colors = static_cast<osg::Vec4Array*>(this->getColorArray());
    //(*colors)[0] = color;
    colors->front() = m_color;
    colors->dirty();
    this->dirtyDisplayList();
    this->dirtyBound();
}

const osg::Vec4f&entity::Stroke::getColor() const
{
    return m_color;
}

bool entity::Stroke::isCurved() const
{
    return m_isCurved;
}

bool entity::Stroke::isShadered() const
{
    return m_isShadered;
}

const osg::Program *entity::Stroke::getProgram() const
{
    return m_program.get();
}

bool entity::Stroke::copyFrom(const entity::Stroke *copy)
{
    if (!copy){
        qWarning("Copy stroke is NULL");
        return false;
    }
    osg::Vec3Array* verts = static_cast<osg::Vec3Array*>(this->getVertexArray());
    const osg::Vec3Array* vertsCopy = static_cast<const osg::Vec3Array*>(copy->getVertexArray());

    if (this->getColor() != cher::STROKE_CLR_NORMAL ||
            !this->getLines() ||
            static_cast<int>(this->getLines()->getMode()) != GL_LINE_STRIP_ADJACENCY ||
            !verts || !vertsCopy) {
        qWarning("stroke::copyFrom() : stroke parameters check failed");
        return false;
    }

    if (verts->size() != 0 || vertsCopy->size() == 0){
        qWarning("stroke::copyFrom() : stroke size check failed");
        return false;
    }

    if (!copy->isShadered()){
        if (this->getColor() != cher::STROKE_CLR_NORMAL ||
                !this->getLines() ||
                static_cast<int>(this->getLines()->getMode()) != GL_LINE_STRIP_ADJACENCY ||
                static_cast<int>(copy->getLines()->getMode()) != GL_LINE_STRIP_ADJACENCY ||
                !verts || !vertsCopy)
        {
            qWarning("stroke::copyFrom() : stroke parameters check failed");
            return false;
        }


        for (unsigned int i=0; i<vertsCopy->size(); ++i){
            osg::Vec2f p = copy->getPoint(i);
            this->appendPoint(p.x(), p.y());
        }
    }
    else {
        if (static_cast<int>(copy->getLines()->getMode()) != GL_LINES_ADJACENCY_EXT){
            qWarning("stroke::copyFrom : copy stroke geometry's mode failed");
            return false;
        }
        if (vertsCopy->size() % 4 != 0 ){
            qWarning("stroke::copyFrom : copy stroke vertex number must be divadable to 4");
            return false;
        }

        for (unsigned int i=1; i<vertsCopy->size(); i=i+4){
            Q_ASSERT(i<vertsCopy->size());
            osg::Vec2f p = copy->getPoint(i);
            this->appendPoint(p.x(), p.y());
        }
        osg::Vec2f p = copy->getPoint(vertsCopy->size()-2);
        this->appendPoint(p.x(), p.y());
        Q_ASSERT(verts->size() == vertsCopy->size()/4 + 1);
    }

    return true;
}

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

void entity::Stroke::appendPoint(const float u, const float v)
{
    //osg::Vec4Array* colors = static_cast<osg::Vec4Array*>(this->getColorArray());
    osg::Vec3Array* verts = static_cast<osg::Vec3Array*>(this->getVertexArray());

    //colors->push_back(cher::STROKE_CLR_NORMAL);
    //colors->dirty();

    verts->push_back(osg::Vec3f(u,v,0.f));
    unsigned int sz = verts->size();

    m_lines->setFirst(0);
    m_lines->setCount(sz);

    verts->dirty();
    this->dirtyBound();
    // read more: http://forum.openscenegraph.org/viewtopic.php?t=2190&postdays=0&postorder=asc&start=15
}

osg::Vec2f entity::Stroke::getPoint(unsigned int i) const
{
    const osg::Vec3Array* verts = static_cast<const osg::Vec3Array*>(this->getVertexArray());
    if (!verts){
        qWarning("Stroke vertices are not initialized. Cannot obtain a point.");
        return osg::Vec2f(0.f, 0.f);
    }
    unsigned int sz = verts->size();
    if (i>=sz){
        qWarning("Stroke's index is out of range. Cannot obtain  a point");
        return osg::Vec2f(0.f, 0.f);
    }
    osg::Vec3f p = (*verts)[i];
    if (std::fabs(p.z()) > cher::EPSILON){
        qWarning("Stroke::getPoint : unexpected value of z-coordinate");
        qInfo() << p.z();
        return osg::Vec2f(0.f, 0.f);
    }

    return osg::Vec2f(p.x(), p.y());
}

osg::Camera *entity::Stroke::getCamera() const
{
    return m_camera.get();
}

bool entity::Stroke::redefineToCurve(float tolerance)
{
    if (m_isCurved) return true;

    osg::ref_ptr<osg::Vec3Array> path = static_cast<osg::Vec3Array*>(this->getVertexArray());
    if (!path.get()){
        qWarning("Vertex data is NULL");
        return false;
    }

    /* set up auto threshold if necessary */
    if (tolerance < 0.f){
        /* auto threshold helps to avoid under-fitting or over-fitting of the curve
         * depending on the scale of drawn stroke. */
        float length = this->getLength();
        qDebug() << "length=" << length;

        tolerance = length * 0.005;
        qDebug() << "assume threshold=" << tolerance;
    }

    OsgPathFitter<osg::Vec3Array, osg::Vec3f, float> fitter;
    fitter.init(*(path.get()));
    osg::ref_ptr<osg::Vec3Array> curves = fitter.fit(tolerance);
    if (!curves.get()){
        qWarning("Curves is NULL");
        return false;
    }

    this->setVertexArray(curves);
    curves->dirty();
    this->dirtyBound();
    qDebug() << "path.samples=" << path->size();
    qDebug() << "curves.points=" << curves->size();

    m_isCurved = true;
    return true;
}

// read more on why: http://stackoverflow.com/questions/36655888/opengl-thick-and-smooth-non-broken-lines-in-3d
bool entity::Stroke::redefineToShader(osg::Camera *camera)
{
    /* The used shader requires that each line segment is represented as GL_LINES_AJACENCY_EXT
    *  This required adding padding points for each line segment
    *  Same goes for color, if it's not uniform */

    if (m_isShadered) return true;
    if (!this->redefineToCurve()){
        qWarning() << "Could not re-define to curve";
        return false;
    }

    /* initialize m_program */
    if (!this->initializeShaderProgram(camera)){
        qWarning("Could not properly initialize the stroke shader program, default look will be used");
        return false;
    }

    osg::ref_ptr<osg::Vec3Array> bezierPts = static_cast<osg::Vec3Array*>(this->getVertexArray());
    if (!bezierPts) return false;

    /* reset the primitive type */
    m_lines->set(GL_LINES_ADJACENCY_EXT, 0, bezierPts->size());

    /* set shader attributes */
    this->setVertexAttribArray(0, bezierPts, osg::Array::BIND_PER_VERTEX);
    osg::Vec4Array* colors = dynamic_cast<osg::Vec4Array*>(this->getColorArray());
    Q_ASSERT(colors);
    this->setVertexAttribArray(1, colors, osg::Array::BIND_OVERALL);

    /* set up m_program as StateSet */
    osg::StateSet* stateset = this->getOrCreateStateSet();
    Q_ASSERT(stateset);
    stateset->setAttributeAndModes(m_program.get(), osg::StateAttribute::ON);

    m_camera = camera;
    m_isShadered = true;

    return true;
}

bool entity::Stroke::redefineToDefault()
{
    if (!m_isShadered && !m_isCurved) return true;

    if (m_isCurved){
        m_isCurved = false;
    }

    if (m_isShadered){
        osg::ref_ptr<osg::Vec3Array> shaderedPts = static_cast<osg::Vec3Array*>(this->getVertexArray());
        if (!shaderedPts.get()) {
            qWarning("Could not extract shadered vertices");
            return false;
        }
        if (shaderedPts->size() % 4 != 0 || shaderedPts->size() == 0 ||
                static_cast<int>(m_lines->getMode()) !=  GL_LINES_ADJACENCY_EXT) {
            qWarning("Shadered vertices chech failed");
            return false;
        }

        osg::ref_ptr<osg::Vec3Array> defaultPts = new osg::Vec3Array(shaderedPts->size()/4+1);
        int idx = 0;
        for (size_t i=1; i<shaderedPts->size(); i=i+4){
            (*defaultPts)[idx++] = (*shaderedPts)[i];
        }
        (*defaultPts)[idx++] = (*shaderedPts)[shaderedPts->size()-2];
        Q_ASSERT(idx == shaderedPts->size()/4+1);

        this->setVertexArray(defaultPts.get());

        /* reset the primitive type */
        m_lines->set(GL_LINE_STRIP_ADJACENCY, 0, defaultPts->size());

        /* disable shader program */
        this->getOrCreateStateSet()->setAttributeAndModes(m_program.get(), osg::StateAttribute::OFF);

        m_isShadered = false;
    }

    return true;
}

int entity::Stroke::getNumPoints() const
{
    const osg::Vec3Array* verts = static_cast<const osg::Vec3Array*>(this->getVertexArray());
    Q_ASSERT(verts);

    return static_cast<int>(verts->size());
}

float entity::Stroke::getLength() const
{
    osg::BoundingBox bb = this->getBoundingBox();
    if (std::fabs(bb.zMax()-bb.zMin()) > cher::EPSILON ){
        qWarning("Stroke->getLength(): z coordinates of a stroke are unexpected values");
        qDebug() << "zMax " << bb.zMax();
        qDebug() << "zMin " << bb.zMin();
        return 0;
    }
    return std::max(bb.xMax() - bb.xMin(), bb.yMax() - bb.yMin());
}

bool entity::Stroke::isLengthy() const
{
    return this->getLength()>cher::STROKE_MINL;
}

void entity::Stroke::moveDelta(double du, double dv)
{
    osg::Vec3Array* verts = static_cast<osg::Vec3Array*>(this->getVertexArray());
    for (unsigned int i=0; i<verts->size(); ++i){
        osg::Vec3f vi = (*verts)[i];
        (*verts)[i] = osg::Vec3f(du+vi.x(), dv+vi.y(), 0);
    }
    verts->dirty();
    this->dirtyBound();
}

void entity::Stroke::scale(double scaleX, double scaleY, osg::Vec3f center)
{
    osg::Vec3Array* verts = static_cast<osg::Vec3Array*>(this->getVertexArray());
    for (unsigned int i=0; i<verts->size(); ++i){
        osg::Vec3f vi = (*verts)[i] - center;
        (*verts)[i] = center + osg::Vec3f(scaleX*vi.x(), scaleY*vi.y(), 0);
    }
    verts->dirty();
    this->dirtyBound();
}

void entity::Stroke::scale(double scale, osg::Vec3f center)
{
    osg::Vec3Array* verts = static_cast<osg::Vec3Array*>(this->getVertexArray());
    for (unsigned int i=0; i<verts->size(); ++i){
        osg::Vec3f vi = (*verts)[i] - center;
        (*verts)[i] = center + osg::Vec3f(scale*vi.x(), scale*vi.y(), 0);
    }
    verts->dirty();
    this->dirtyBound();
}

void entity::Stroke::rotate(double theta, osg::Vec3f center)
{
    osg::Vec3Array* verts = static_cast<osg::Vec3Array*>(this->getVertexArray());
    for (unsigned int i=0; i<verts->size(); ++i){
        osg::Vec3f vi = (*verts)[i] - center;
        (*verts)[i] = center + osg::Vec3f(vi.x() * std::cos(theta) - vi.y() * std::sin(theta),
                                          vi.x() * std::sin(theta) + vi.y() * std::cos(theta), 0);
    }
    verts->dirty();
    this->dirtyBound();
}

cher::ENTITY_TYPE entity::Stroke::getEntityType() const
{
    return cher::ENTITY_STROKE;
}

bool entity::Stroke::initializeShaderProgram(osg::Camera *camera)
{
    if (!camera){
        qWarning("Camera is NULL");
        return false;
    }

    if (m_program->getNumShaders() == 3) {
        return true;
    }

    m_program->setName("DefaultStrokeShader");

    /* load and add shaders to the program */
    osg::ref_ptr<osg::Shader> vertShader = new osg::Shader(osg::Shader::VERTEX);
    if (!vertShader->loadShaderSourceFromFile("Shaders/Stroke.vert")){
        qWarning("Could not load vertex shader from file");
        return false;
    }
    if (!m_program->addShader(vertShader.get())){
        qWarning("Could not add vertext shader");
        return false;
    }

    osg::ref_ptr<osg::Shader> geomShader = new osg::Shader(osg::Shader::GEOMETRY);
    if (!geomShader->loadShaderSourceFromFile("Shaders/Stroke.geom")){
        qWarning("Could not load geometry shader from file");
        return false;
    }
    if (!m_program->addShader(geomShader.get())){
        qWarning("Could not add geometry shader");
        return false;
    }

    osg::ref_ptr<osg::Shader> fragShader = new osg::Shader(osg::Shader::FRAGMENT);
    if (!fragShader->loadShaderSourceFromFile("Shaders/Stroke.frag")){
        qWarning("Could not load fragment shader from file");
        return false;
    }
    if (!m_program->addShader(fragShader.get())){
           qWarning("Could not add fragment shader");
           return false;
    }

    /* add uniforms */
    osg::StateSet* state = this->getOrCreateStateSet();
    if (!state){
        qWarning("Creating shader: StateSet is NULL");
        return false;
    }

    /* model view proj matrix */
    osg::Uniform* MVPMatrix = new osg::Uniform(osg::Uniform::FLOAT_MAT4, "ModelViewProjectionMatrix");
    MVPMatrix->setUpdateCallback(new ModelViewProjectionMatrixCallback(camera));
    state->addUniform(MVPMatrix);

    /* viewport vector */
    osg::Uniform* viewportVector = new osg::Uniform(osg::Uniform::FLOAT_VEC2, "Viewport");
    viewportVector->setUpdateCallback(new ViewportVectorCallback(camera));
    state->addUniform(viewportVector);

    /* stroke thickness */
    float thickness = cher::STROKE_LINE_WIDTH;
    state->addUniform(new osg::Uniform("Thickness", thickness));

    /*  stroke miter limit */
    float miterLimit = 0.75;
    state->addUniform(new osg::Uniform("MiterLimit", miterLimit));

    /* stroke number of segments (the more the smoothier the look) */
    int segments = cher::STROKE_SEGMENTS_NUMBER;
    state->addUniform(new osg::Uniform("Segments", segments));

    return true;
}

/* for serialization of stroke type
 * for more info, see OSG beginner's guide, or
 * OSG cookbook. In both, there is a section on serialization.
*/
REGISTER_OBJECT_WRAPPER(Stroke_Wrapper
                        , new entity::Stroke
                        , entity::Stroke
                        , "osg::Object osg::Geometry entity::Stroke")
{
    ADD_OBJECT_SERIALIZER(Lines, osg::DrawArrays, NULL);
    ADD_VEC4F_SERIALIZER(Color, osg::Vec4f());
}
