#include "ProgramStroke.h"

#include <QtGlobal>
#include <QDebug>

#include <osg/Shader>

#include "Settings.h"
#include "ModelViewProjectionMatrixCallback.h"
#include "ViewportVectorCallback.h"
#include "CameraEyeCallback.h"
#include "CanvasTransformCallback.h"

ProgramStroke::ProgramStroke()
    : osg::Program()
    , m_state(0)
    , m_camera(0)
    , m_transform(0)
    , m_isFogged(false)
{
    this->setName("ProgramStroke");
}

void ProgramStroke::initialize(osg::StateSet *state, osg::Camera *camera, osg::MatrixTransform *t, bool isFogged)
{
    m_state = state;
    m_camera = camera;
    m_transform = t;
    m_isFogged = isFogged;

    if (!this->addPresetShaders())
        qCritical("Could not add necessary shaders");
    if (!this->addPresetUniforms())
        qCritical("Could not add necessary uniforms");

    qInfo("Stroke shader was successfully initialized");
}


void ProgramStroke::updateIsFogged(bool f)
{
    m_isFogged = f;
    this->addUniform<bool>("IsFogged", osg::Uniform::BOOL, m_isFogged);
}

void ProgramStroke::updateTransform(osg::MatrixTransform *t)
{
    m_transform = t;
    /* canvas matrix transform */
    if (!this->addUniformCanvasMatrix())
    {
        qWarning("Could not update CanvasMatrix uniform");
    }
}

osg::MatrixTransform *ProgramStroke::getTransform() const
{
    return m_transform.get();
}

osg::Camera *ProgramStroke::getCamera() const
{
    return m_camera.get();
}

bool ProgramStroke::getIsFogged() const
{
    return m_isFogged;
}

bool ProgramStroke::addPresetShaders()
{
    if (this->getNumShaders() == 3){
        qWarning("Shaders already seems to be added");
        return true;
    }

    /* load and add shaders to the program */
    osg::ref_ptr<osg::Shader> vertShader = new osg::Shader(osg::Shader::VERTEX);
    if (!vertShader->loadShaderSourceFromFile("Shaders/Stroke.vert")){
        qWarning("Could not load vertex shader from file");
        return false;
    }
    if (!this->addShader(vertShader.get())){
        qWarning("Could not add vertext shader");
        return false;
    }

    osg::ref_ptr<osg::Shader> geomShader = new osg::Shader(osg::Shader::GEOMETRY);
    if (!geomShader->loadShaderSourceFromFile("Shaders/Stroke.geom")){
        qWarning("Could not load geometry shader from file");
        return false;
    }
    if (!this->addShader(geomShader.get())){
        qWarning("Could not add geometry shader");
        return false;
    }

    osg::ref_ptr<osg::Shader> fragShader = new osg::Shader(osg::Shader::FRAGMENT);
    if (!fragShader->loadShaderSourceFromFile("Shaders/Stroke.frag")){
        qWarning("Could not load fragment shader from file");
        return false;
    }
    if (!this->addShader(fragShader.get())){
           qWarning("Could not add fragment shader");
           return false;
    }

    return true;
}

bool ProgramStroke::addPresetUniforms()
{
    {
        /* model view proj matrix */
        if (!m_camera.get() || !m_transform.get()){
            qWarning("Camera or canvas transform is null");
            return false;
        }
        if (!this->addUniform<ModelViewProjectionMatrixCallback>("ModelViewProjectionMatrix",
                                                            osg::Uniform::FLOAT_MAT4,
                                                            new ModelViewProjectionMatrixCallback(m_camera.get()) ))
        {
            qWarning("Could not add MVP uniform");
            return false;
        }

        /* viewport vector */
        if (!this->addUniform<ViewportVectorCallback>("Viewport",
                                                      osg::Uniform::FLOAT_VEC2,
                                                      new ViewportVectorCallback(m_camera.get()) ))
        {
            qWarning("Could not add Viewport uniform");
            return false;
        }

        /* camera eye */
        if (!this->addUniform<CameraEyeCallback>("CameraEye",
                                                 osg::Uniform::FLOAT_VEC4,
                                                 new CameraEyeCallback(m_camera.get()) ))
        {
            qWarning("Could not add CameraEye uniform");
            return false;
        }

        /* canvas matrix transform */
        if (!this->addUniformCanvasMatrix())
        {
            qWarning("Could not add CameraEye uniform");
            return false;
        }

        /* stroke thickness */
        if (!this->addUniform<float>("Thickness", osg::Uniform::FLOAT, cher::STROKE_LINE_WIDTH)){
            qWarning("Could not initialize thickness uniform");
            return false;
        }

        /* stroke miter limit */
        if (!this->addUniform<float>("MiterLimit", osg::Uniform::FLOAT, 0.75f)){
            qCritical("Could not initialize miter limit uniform");
            return false;
        }

        /* numher of segments in drawn curve */
        if (!this->addUniform<int>("Segments", osg::Uniform::INT, cher::STROKE_SEGMENTS_NUMBER)){
            qCritical("Could not initialize segments uniform");\
            return false;
        }

        /* fog factor related */
        if (!this->addUniform<float>("FogMin", osg::Uniform::FLOAT, cher::STROKE_FOG_MIN)){
            qCritical("Could not initialize fog factor uniform");
            return false;
        }

        if (!this->addUniform<float>("FogMax", osg::Uniform::FLOAT, cher::STROKE_FOG_MAX)){
            qCritical("Could not initialize fog max factor");
            return false;
        }

        if (!this->addUniform<bool>("IsFogged", osg::Uniform::BOOL, m_isFogged)){
            qCritical("Could not initialize fog min factor");
            return false;
        }

        return true;
    }
}

bool ProgramStroke::addUniformCanvasMatrix()
{
    /* canvas matrix transform */
    if (!this->addUniform<CanvasTransformCallback>("CanvasMatrix",
                                                   osg::Uniform::FLOAT_MAT4,
                                                   new CanvasTransformCallback(m_transform.get()) ))
    {
        qWarning("Could not add CameraEye uniform");
        return false;
    }
    return true;
}

template <typename T>
bool ProgramStroke::addUniform(const std::string &name, osg::Uniform::Type type, T *updateCallback)
{
    if (!m_state.get() || !updateCallback){
        qCritical("State or callback is NULL");
        return false;
    }
    osg::Uniform* uniform = m_state->getOrCreateUniform(name, type);
    uniform->setUpdateCallback(updateCallback);

    return true;
}

template <typename T>
bool ProgramStroke::addUniform(const std::string &name, osg::Uniform::Type type, T value)
{
    if (!m_state.get()){
        qCritical("State is nULL");
        return false;
    }
    osg::Uniform* uniform = m_state->getOrCreateUniform(name, type);
    uniform->set(value);

    return true;
}
