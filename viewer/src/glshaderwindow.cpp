#include "glshaderwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QScreen>
#include <QQuaternion>
#include <QOpenGLFramebufferObjectFormat>
// Buttons/sliders for User interface:
#include <QGroupBox>
#include <QRadioButton>
#include <QSlider>
#include <QLabel>
// Layouts for User interface
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <assert.h>


#include "perlinNoise.h" // defines tables for Perlin Noise
// shadowMapDimension: 512 if copy to CPU, 2048 if not

glShaderWindow::glShaderWindow(QWindow *parent)
    : OpenGLWindow(parent), modelMesh(0),
      m_program(0), ground_program(0), shadowMapGenerationProgram(0),
      g_vertices(0), g_normals(0), g_texcoords(0), g_colors(0), g_indices(0),
      environmentMap(0), texture(0), normalMap(0), permTexture(0), pixels(0), mouseButton(Qt::NoButton), auxWidget(0),
      blinnPhong(true), transparent(true), gooch(false), toon(false), cookTorrance(false), eta(0), noiseRate(0.5), noisePersistence(0.4),
      roughness(0.3), lightIntensity(1.5f), noiseMarble(false), noiseJade(true), noiseWood(false), cartesianCoo(false), withNoise(true),
      sphericalCoo(false), noiseNormal(false), PCSS(true), VSM(false), ESM(false), lightSize(1), maxFilterSize(5), biasCoeff(10),
      shininess(50.0f), lightDistance(5.0f), groundDistance(0.78), shadowMap(0), shadowMapDimension(512), fullScreenSnapshots(false),
      m_indexBuffer(QOpenGLBuffer::IndexBuffer), ground_indexBuffer(QOpenGLBuffer::IndexBuffer)
{
    m_fragShaderSuffix << "*.frag" << "*.fs";
    m_vertShaderSuffix << "*.vert" << "*.vs";
}

glShaderWindow::~glShaderWindow()
{
    if (modelMesh) delete modelMesh;
    if (m_program) {
        m_program->release();
        delete m_program;
    }
    if (pixels) delete [] pixels;
    m_vertexBuffer.release();
    m_vertexBuffer.destroy();
    m_indexBuffer.release();
    m_indexBuffer.destroy();
    m_colorBuffer.release();
    m_colorBuffer.destroy();
    m_normalBuffer.release();
    m_normalBuffer.destroy();
    m_texcoordBuffer.release();
    m_texcoordBuffer.destroy();
    m_vao.release();
    m_vao.destroy();
    ground_vertexBuffer.release();
    ground_vertexBuffer.destroy();
    ground_indexBuffer.release();
    ground_indexBuffer.destroy();
    ground_colorBuffer.release();
    ground_colorBuffer.destroy();
    ground_normalBuffer.release();
    ground_normalBuffer.destroy();
    ground_texcoordBuffer.release();
    ground_texcoordBuffer.destroy();
    ground_vao.release();
    ground_vao.destroy();
    if (g_vertices) delete [] g_vertices;
    if (g_colors) delete [] g_colors;
    if (g_normals) delete [] g_normals;
    if (g_indices) delete [] g_indices;
    if (shadowMap) {
        shadowMap->release();
        delete shadowMap;
    }
}


void glShaderWindow::openSceneFromFile() {
    QFileDialog dialog(0, "Open Scene", workingDirectory, "*.off *.obj *.ply *.3ds");
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    QString filename;
    int ret = dialog.exec();
    if (ret == QDialog::Accepted) {
        workingDirectory = dialog.directory().path() + "/";
        modelName = dialog.selectedFiles()[0];
    }

    if (!modelName.isNull())
    {
        openScene();
        renderNow();
    }
}

void glShaderWindow::openNewTexture() {
    QFileDialog dialog(0, "Open texture image", workingDirectory + "../textures/", "*.png *.PNG *.jpg *.JPG");
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    QString filename;
    int ret = dialog.exec();
    if (ret == QDialog::Accepted) {
        textureName = dialog.selectedFiles()[0];
        if (!textureName.isNull()) {
            if (m_program) m_program->bind();
            if ((m_program->uniformLocation("colorTexture") != -1) || (ground_program->uniformLocation("colorTexture") != -1)) {
                glActiveTexture(GL_TEXTURE0);
                if (texture) {
                    texture->release();
                    texture->destroy();
                    delete texture;
                    texture = 0;
                }
                texture = new QOpenGLTexture(QImage(textureName));
                if (texture) {
                    texture->setWrapMode(QOpenGLTexture::Repeat);
                    texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
                    texture->setMagnificationFilter(QOpenGLTexture::Linear);
                    texture->bind(0);
                    if (m_program->uniformLocation("colorTexture") != -1) m_program->setUniformValue("colorTexture", 0);
                    if (ground_program->uniformLocation("colorTexture") != -1) ground_program->setUniformValue("colorTexture", 0);
                }
            }
        }
        renderNow();
    }
}

void glShaderWindow::openNewNormalMap() {
    QFileDialog dialog(0, "Open normal map image", workingDirectory + "../textures/", "*.png *.PNG *.jpg *.JPG");
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    QString filename;
    int ret = dialog.exec();
    if (ret == QDialog::Accepted) {
        normalMapName= dialog.selectedFiles()[0];
        m_program->bind();
        if ((!normalMapName.isNull()) && (m_program->uniformLocation("normalMap") != -1)|| (ground_program->uniformLocation("normalMap") != -1)) {
            glActiveTexture(GL_TEXTURE1);
            if (normalMap) {
                normalMap->release();
                normalMap->destroy();
                delete normalMap;
                normalMap = 0;
            }
            normalMap = new QOpenGLTexture(QImage(normalMapName).mirrored());
            if (normalMap) {
                normalMap->setWrapMode(QOpenGLTexture::MirroredRepeat);
                normalMap->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
                normalMap->setMagnificationFilter(QOpenGLTexture::Nearest);
                normalMap->bind(1);
                if (m_program->uniformLocation("normalMap") != -1) m_program->setUniformValue("normalMap", 1);
                if (ground_program->uniformLocation("normalMap") != -1) ground_program->setUniformValue("normalMap", 1);            }
        }
        renderNow();
    }
}
void glShaderWindow::openNewEnvMap() {
    QFileDialog dialog(0, "Open environment map image", workingDirectory + "../textures/", "*.png *.PNG *.jpg *.JPG");
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    QString filename;
    int ret = dialog.exec();
    if (ret == QDialog::Accepted) {
        envMapName= dialog.selectedFiles()[0];
        m_program->bind();
        if ((!envMapName.isNull()) && (m_program->uniformLocation("envMap") != -1)) {
            glActiveTexture(GL_TEXTURE1);
            if (environmentMap) {
                environmentMap->release();
                environmentMap->destroy();
                delete environmentMap;
                environmentMap = 0;
            }
            environmentMap = new QOpenGLTexture(QImage(envMapName).mirrored());
            if (environmentMap) {
                environmentMap->setWrapMode(QOpenGLTexture::MirroredRepeat);
                environmentMap->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
                environmentMap->setMagnificationFilter(QOpenGLTexture::Nearest);
                environmentMap->bind(1);
                m_program->setUniformValue("envMap", 1);
            }
        }
        renderNow();
    }
}

void glShaderWindow::PCFClicked()
{
    PCSS = false;
    VSM = false;
    ESM = false;

    //ground_program = prepareShaderProgram(":/3_textured.vert", ":/3_textured.frag");
    //m_program = prepareShaderProgram(":/PCF.vert", ":/PCF.frag");
    ground_program = prepareShaderProgram(":/textured_PCF.vert", ":/textured_PCF.frag");
    setShader("PCF");
    renderNow();
}

void glShaderWindow::PCSSClicked()
{
    PCSS = true;
    VSM = false;
    ESM = false;
    //ground_program = prepareShaderProgram(":/3_textured.vert", ":/3_textured.frag");
    //m_program = prepareShaderProgram(":/PCSS.vert", ":/PCSS.frag");
    ground_program = prepareShaderProgram(":/textured_PCSS.vert", ":/textured_PCSS.frag");
    setShader("PCSS");
    renderNow();
}

void glShaderWindow::VSMClicked()
{
    PCSS = false;
    VSM = true;
    ESM = false;

    //ground_program = prepareShaderProgram(":/3_textured.vert", ":/3_textured.frag");
    //m_program = prepareShaderProgram(":/VSM.vert", ":/VSM.frag");
    ground_program = prepareShaderProgram(":/textured_VSM.vert", ":/textured_VSM.frag");
    setShader("VSM");
    renderNow();
}

void glShaderWindow::ESMClicked()
{
    PCSS = false;
    VSM = false;
    ESM = true;

    //ground_program = prepareShaderProgram(":/3_textured.vert", ":/3_textured.frag");
    //m_program = prepareShaderProgram(":/ESM.vert", ":/ESM.frag");
    ground_program = prepareShaderProgram(":/textured_ESM.vert", ":/textured_ESM.frag");
    setShader("ESM");
    renderNow();
}

void glShaderWindow::phongClicked()
{
    blinnPhong = false;
    cookTorrance = false;
    gooch = false;
    renderNow();
}

void glShaderWindow::blinnPhongClicked()
{
    blinnPhong = true;
    cookTorrance = false;
    gooch = false;
    toon = false;
    renderNow();
}

void glShaderWindow::cookTorranceClicked()
{
    blinnPhong = false;
    cookTorrance = true;
    gooch = false;
    toon = false;
    renderNow();
}

void glShaderWindow::goochClicked()
{
    blinnPhong = false;
    cookTorrance = false;
    gooch = true;
    toon = false;
    renderNow();
}

void glShaderWindow::toonClicked()
{
    blinnPhong = false;
    cookTorrance = false;
    gooch = false;
    toon = true;
    renderNow();
}

void glShaderWindow::transparentClicked()
{
    transparent = true;
    renderNow();
}

void glShaderWindow::noiseMarbleClicked()
{
    noiseMarble = true;
    noiseJade = false;
    noiseWood = false;
    renderNow();
}

void glShaderWindow::noiseJadeClicked()
{
    noiseMarble = false;
    noiseJade = true;
    noiseWood = false;
    renderNow();
}

void glShaderWindow::noiseWoodClicked()
{
    noiseMarble = false;
    noiseJade = false;
    noiseWood = true;
    renderNow();
}

void glShaderWindow::noiseNormalClicked()
{
    cartesianCoo = false;
    sphericalCoo = false;
    noiseNormal = true;
    renderNow();
}

void glShaderWindow::cartesianCooClicked()
{
    cartesianCoo = true;
    sphericalCoo = false;
    noiseNormal = false;
    renderNow();
}

void glShaderWindow::sphericalCooClicked()
{
    cartesianCoo = false;
    sphericalCoo = true;
    noiseNormal = false;
    renderNow();
}

void glShaderWindow::withNoiseClicked()
{
    withNoise = true;
    renderNow();
}

void glShaderWindow::fromTextureClicked()
{
    withNoise = false;
    renderNow();
}

void glShaderWindow::opaqueClicked()
{
    transparent = false;
    renderNow();
}

void glShaderWindow::updateLightIntensity(int lightSliderValue)
{
    lightIntensity = lightSliderValue / 100.0;
    renderNow();
}

void glShaderWindow::updateShininess(int shininessSliderValue)
{
    shininess = shininessSliderValue;
    renderNow();
}

void glShaderWindow::updateEta(int etaSliderValue)
{
    eta = etaSliderValue/100.0;
    renderNow();
}

void glShaderWindow::updateRoughness(int roughnessSliderValue)
{
    roughness = roughnessSliderValue/100.0;
    renderNow();
}

void glShaderWindow::updateNoiseRate(int noiseRateSliderValue)
{
    noiseRate = noiseRateSliderValue/100.0;
    renderNow();
}

void glShaderWindow::updateNoisePersistence(int noisePersistenceSliderValue)
{
    noisePersistence = noisePersistenceSliderValue/100.0;
    renderNow();
}

void glShaderWindow::updateLightSize(int lightSizeSliderValue)
{
    lightSize = lightSizeSliderValue;
    renderNow();
}

void glShaderWindow::updateMaxFilterSize(int maxFilterSizeSliderValue)
{
    maxFilterSize = maxFilterSizeSliderValue;
    renderNow();
}

void glShaderWindow::updateBiasCoeff(int biasCoeffSliderValue)
{
    biasCoeff = biasCoeffSliderValue;
    renderNow();
}

void glShaderWindow::showAuxWindow()
{
    if (auxWidget)
        if (auxWidget->isVisible()) return;
    auxWidget = new QWidget;
    auxWidget->move(0,0);

    QVBoxLayout *outer = new QVBoxLayout;
    QHBoxLayout *buttons = new QHBoxLayout;

    // Specular Model selection box
    QGroupBox *groupBox = new QGroupBox("Specular Model selection");
    QRadioButton *radio1 = new QRadioButton("Phong");
    QRadioButton *radio2 = new QRadioButton("Blinn-Phong");
    QRadioButton *radio3 = new QRadioButton("Cook-Torrance");
    QRadioButton *radio4 = new QRadioButton("Gooch");
    QRadioButton *radio5 = new QRadioButton("X-Toon");
    if (blinnPhong)
    {
        radio1->setChecked(false);
        radio2->setChecked(true);
        radio3->setChecked(false);
        radio4->setChecked(false);
        radio5->setChecked(false);
    }
    else if (cookTorrance)
    {
        radio1->setChecked(false);
        radio2->setChecked(false);
        radio3->setChecked(true);
        radio4->setChecked(false);
        radio5->setChecked(false);
    }
    else if (gooch)
    {
        radio1->setChecked(false);
        radio2->setChecked(false);
        radio3->setChecked(false);
        radio4->setChecked(true);
        radio5->setChecked(false);
    }
    else if (toon)
    {
        radio1->setChecked(false);
        radio2->setChecked(false);
        radio3->setChecked(false);
        radio4->setChecked(false);
        radio5->setChecked(true);
    }
    else
    {
        radio1->setChecked(true);
        radio2->setChecked(false);
        radio3->setChecked(false);
        radio4->setChecked(false);
        radio5->setChecked(false);
    }
    connect(radio1, SIGNAL(clicked()), this, SLOT(phongClicked()));
    connect(radio2, SIGNAL(clicked()), this, SLOT(blinnPhongClicked()));
    connect(radio3, SIGNAL(clicked()), this, SLOT(cookTorranceClicked()));
    connect(radio4, SIGNAL(clicked()), this, SLOT(goochClicked()));
    connect(radio5, SIGNAL(clicked()), this, SLOT(toonClicked()));

    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addWidget(radio1);
    vbox->addWidget(radio2);
    vbox->addWidget(radio3);
    vbox->addWidget(radio4);
    vbox->addWidget(radio5);
    groupBox->setLayout(vbox);
    buttons->addWidget(groupBox);

    // Shadow type selection box
    QGroupBox *groupBox0 = new QGroupBox("Shadow type selection");
    QRadioButton *shadow1 = new QRadioButton("PCF");
    QRadioButton *shadow2 = new QRadioButton("PCSS");
    QRadioButton *shadow3 = new QRadioButton("VSM");
    QRadioButton *shadow4 = new QRadioButton("ESM");
    if (PCSS)
    {
        shadow1->setChecked(false);
        shadow2->setChecked(true);
        shadow3->setChecked(false);
        shadow4->setChecked(false);
    }
    else if (VSM)
    {
        shadow1->setChecked(false);
        shadow2->setChecked(false);
        shadow3->setChecked(true);
        shadow4->setChecked(false);
    }
    else if (ESM)
    {
        shadow1->setChecked(false);
        shadow2->setChecked(false);
        shadow3->setChecked(false);
        shadow4->setChecked(true);
    }
    else
    {
        shadow1->setChecked(true);
        shadow2->setChecked(false);
        shadow3->setChecked(false);
        shadow4->setChecked(false);
    }
    connect(shadow1, SIGNAL(clicked()), this, SLOT(PCFClicked()));
    connect(shadow2, SIGNAL(clicked()), this, SLOT(PCSSClicked()));
    connect(shadow3, SIGNAL(clicked()), this, SLOT(VSMClicked()));
    connect(shadow4, SIGNAL(clicked()), this, SLOT(ESMClicked()));

    QVBoxLayout *vbox0 = new QVBoxLayout;
    vbox0->addWidget(shadow1);
    vbox0->addWidget(shadow2);
    vbox0->addWidget(shadow3);
    vbox0->addWidget(shadow4);
    groupBox0->setLayout(vbox0);
    buttons->addWidget(groupBox0);

    // Environment Mapping box
    QGroupBox *groupBox1 = new QGroupBox("Environment Mapping");
    QRadioButton *transparent1 = new QRadioButton("&Transparent");
    QRadioButton *transparent2 = new QRadioButton("&Opaque");
    if (transparent) transparent1->setChecked(true);
    else transparent2->setChecked(true);
    connect(transparent1, SIGNAL(clicked()), this, SLOT(transparentClicked()));
    connect(transparent2, SIGNAL(clicked()), this, SLOT(opaqueClicked()));
    QVBoxLayout *vbox1 = new QVBoxLayout;
    vbox1->addWidget(transparent1);
    vbox1->addWidget(transparent2);
    groupBox1->setLayout(vbox1);
    buttons->addWidget(groupBox1);
    outer->addLayout(buttons);

    // Normal Mapping box
    QGroupBox *groupBox2 = new QGroupBox("Normal Mapping");
    QRadioButton *normal1 = new QRadioButton("&Texture with cartesian coordinates");
    QRadioButton *normal2 = new QRadioButton("&Texture with spherical coordinates");
    QRadioButton *normal3 = new QRadioButton("&With noise");
    if (sphericalCoo)
    {
        normal1->setChecked(false);
        normal2->setChecked(true);
        normal3->setChecked(false);
    }
    else if (noiseNormal)
    {
        normal1->setChecked(false);
        normal2->setChecked(false);
        normal3->setChecked(true);
    }
    else
    {
        normal1->setChecked(true);
        normal2->setChecked(false);
        normal3->setChecked(false);
    }
    connect(normal1, SIGNAL(clicked()), this, SLOT(cartesianCooClicked()));
    connect(normal2, SIGNAL(clicked()), this, SLOT(sphericalCooClicked()));
    connect(normal3, SIGNAL(clicked()), this, SLOT(noiseNormalClicked()));
    QVBoxLayout *vbox2 = new QVBoxLayout;
    vbox2->addWidget(normal1);
    vbox2->addWidget(normal2);
    vbox2->addWidget(normal3);
    groupBox2->setLayout(vbox2);
    buttons->addWidget(groupBox2);
    outer->addLayout(buttons);

    // Displacement Mapping box
    QGroupBox *groupBox4 = new QGroupBox("Displacement Mapping");
    QRadioButton *displacement1 = new QRadioButton("&With noise");
    QRadioButton *displacement2 = new QRadioButton("&From texture");
    if (withNoise) displacement1->setChecked(true);
    else displacement2->setChecked(true);
    connect(displacement1, SIGNAL(clicked()), this, SLOT(withNoiseClicked()));
    connect(displacement2, SIGNAL(clicked()), this, SLOT(fromTextureClicked()));
    QVBoxLayout *vbox4 = new QVBoxLayout;
    vbox4->addWidget(displacement1);
    vbox4->addWidget(displacement2);
    groupBox4->setLayout(vbox4);
    buttons->addWidget(groupBox4);
    outer->addLayout(buttons);

    // Perlin noise box
    QGroupBox *groupBox3 = new QGroupBox("Perlin Noise");
    QRadioButton *model1 = new QRadioButton("&Marble");
    QRadioButton *model2 = new QRadioButton("&Jade");
    QRadioButton *model3 = new QRadioButton("&Wood");
    if (noiseMarble)
    {
        model1->setChecked(true);
        model2->setChecked(false);
        model3->setChecked(false);
    }
    else if (noiseJade)
    {
        model1->setChecked(false);
        model2->setChecked(true);
        model3->setChecked(false);
    }
    else
    {
        model1->setChecked(false);
        model2->setChecked(false);
        model3->setChecked(true);
    }
    connect(model1, SIGNAL(clicked()), this, SLOT(noiseMarbleClicked()));
    connect(model2, SIGNAL(clicked()), this, SLOT(noiseJadeClicked()));
    connect(model3, SIGNAL(clicked()), this, SLOT(noiseWoodClicked()));
    QVBoxLayout *vbox3 = new QVBoxLayout;
    vbox3->addWidget(model1);
    vbox3->addWidget(model2);
    vbox3->addWidget(model3);
    groupBox3->setLayout(vbox3);
    buttons->addWidget(groupBox3);
    outer->addLayout(buttons);

    // light source intensity
    QSlider* lightSlider = new QSlider(Qt::Horizontal);
    lightSlider->setTickPosition(QSlider::TicksBelow);
    lightSlider->setMinimum(0);
    lightSlider->setMaximum(200);
    lightSlider->setSliderPosition(100*lightIntensity);
    connect(lightSlider,SIGNAL(valueChanged(int)),this,SLOT(updateLightIntensity(int)));
    QLabel* lightLabel = new QLabel("Light intensity = ");
    QLabel* lightLabelValue = new QLabel();
    lightLabelValue->setNum(100 * lightIntensity);
    connect(lightSlider,SIGNAL(valueChanged(int)),lightLabelValue,SLOT(setNum(int)));
    QHBoxLayout *hboxLight = new QHBoxLayout;
    hboxLight->addWidget(lightLabel);
    hboxLight->addWidget(lightLabelValue);
    outer->addLayout(hboxLight);
    outer->addWidget(lightSlider);

    // Phong shininess slider
    QSlider* shininessSlider = new QSlider(Qt::Horizontal);
    shininessSlider->setTickPosition(QSlider::TicksBelow);
    shininessSlider->setMinimum(0);
    shininessSlider->setMaximum(200);
    shininessSlider->setSliderPosition(shininess);
    connect(shininessSlider,SIGNAL(valueChanged(int)),this,SLOT(updateShininess(int)));
    QLabel* shininessLabel = new QLabel("Phong exponent = ");
    QLabel* shininessLabelValue = new QLabel();
    shininessLabelValue->setNum(shininess);
    connect(shininessSlider,SIGNAL(valueChanged(int)),shininessLabelValue,SLOT(setNum(int)));
    QHBoxLayout *hboxShininess = new QHBoxLayout;
    hboxShininess->addWidget(shininessLabel);
    hboxShininess->addWidget(shininessLabelValue);
    outer->addLayout(hboxShininess);
    outer->addWidget(shininessSlider);

    // Eta slider
    QSlider* etaSlider = new QSlider(Qt::Horizontal);
    etaSlider->setTickPosition(QSlider::TicksBelow);
    etaSlider->setTickInterval(100);
    etaSlider->setMinimum(0);
    etaSlider->setMaximum(500);
    etaSlider->setSliderPosition(eta*100);
    connect(etaSlider,SIGNAL(valueChanged(int)),this,SLOT(updateEta(int)));
    QLabel* etaLabel = new QLabel("Eta (index of refraction) * 100 =");
    QLabel* etaLabelValue = new QLabel();
    etaLabelValue->setNum(eta * 100);
    connect(etaSlider,SIGNAL(valueChanged(int)),etaLabelValue,SLOT(setNum(int)));
    QHBoxLayout *hboxEta= new QHBoxLayout;
    hboxEta->addWidget(etaLabel);
    hboxEta->addWidget(etaLabelValue);
    outer->addLayout(hboxEta);
    outer->addWidget(etaSlider);

    // Roughness slider
    QSlider* roughnessSlider = new QSlider(Qt::Horizontal);
    roughnessSlider->setTickPosition(QSlider::TicksBelow);
    //roughnessSlider->setTickInterval(100);
    roughnessSlider->setMinimum(0);
    roughnessSlider->setMaximum(100);
    roughnessSlider->setSliderPosition(roughness*100);
    connect(roughnessSlider,SIGNAL(valueChanged(int)),this,SLOT(updateRoughness(int)));
    QLabel* roughnessLabel = new QLabel("Roughness  (microfacet distribution) * 100 =");
    QLabel* roughnessLabelValue = new QLabel();
    roughnessLabelValue->setNum(roughness * 100);
    connect(roughnessSlider,SIGNAL(valueChanged(int)),roughnessLabelValue,SLOT(setNum(int)));
    QHBoxLayout *hboxRoughness= new QHBoxLayout;
    hboxRoughness->addWidget(roughnessLabel);
    hboxRoughness->addWidget(roughnessLabelValue);
    outer->addLayout(hboxRoughness);
    outer->addWidget(roughnessSlider);

    // noiseRate slider
    QSlider* noiseRateSlider = new QSlider(Qt::Horizontal);
    noiseRateSlider->setTickPosition(QSlider::TicksBelow);
    //noiseRateSlider->setTickInterval(100);
    noiseRateSlider->setMinimum(0);
    noiseRateSlider->setMaximum(100);
    noiseRateSlider->setSliderPosition(noiseRate*100);
    connect(noiseRateSlider,SIGNAL(valueChanged(int)),this,SLOT(updateNoiseRate(int)));
    QLabel* noiseRateLabel = new QLabel("Noise contribution (3 for Wood) =");
    QLabel* noiseRateLabelValue = new QLabel();
    noiseRateLabelValue->setNum(noiseRate * 100);
    connect(noiseRateSlider,SIGNAL(valueChanged(int)),noiseRateLabelValue,SLOT(setNum(int)));
    QHBoxLayout *hboxNoiseRate= new QHBoxLayout;
    hboxNoiseRate->addWidget(noiseRateLabel);
    hboxNoiseRate->addWidget(noiseRateLabelValue);
    outer->addLayout(hboxNoiseRate);
    outer->addWidget(noiseRateSlider);

    // noisePersistence slider
    QSlider* noisePersistenceSlider = new QSlider(Qt::Horizontal);
    noisePersistenceSlider->setTickPosition(QSlider::TicksBelow);
    //noisePersistenceSlider->setTickInterval(100);
    noisePersistenceSlider->setMinimum(0);
    noisePersistenceSlider->setMaximum(100);
    noisePersistenceSlider->setSliderPosition(noisePersistence*100);
    connect(noisePersistenceSlider,SIGNAL(valueChanged(int)),this,SLOT(updateNoisePersistence(int)));
    QLabel* noisePersistenceLabel = new QLabel("Noise persistence (default value : 0.4) =");
    QLabel* noisePersistenceLabelValue = new QLabel();
    noisePersistenceLabelValue->setNum(noisePersistence * 100);
    connect(noisePersistenceSlider,SIGNAL(valueChanged(int)),noisePersistenceLabelValue,SLOT(setNum(int)));
    QHBoxLayout *hboxNoisePersistence= new QHBoxLayout;
    hboxNoisePersistence->addWidget(noisePersistenceLabel);
    hboxNoisePersistence->addWidget(noisePersistenceLabelValue);
    outer->addLayout(hboxNoisePersistence);
    outer->addWidget(noisePersistenceSlider);

    // Light Size slider
    QSlider* lightSizeSlider = new QSlider(Qt::Horizontal);
    lightSizeSlider->setTickPosition(QSlider::TicksBelow);
    lightSizeSlider->setTickInterval(1);
    lightSizeSlider->setMinimum(1);
    lightSizeSlider->setMaximum(50);
    lightSizeSlider->setSliderPosition(lightSize);
    connect(lightSizeSlider,SIGNAL(valueChanged(int)),this,SLOT(updateLightSize(int)));
    QLabel* lightSizeLabel = new QLabel("Size of the light soure : ");
    QLabel* lightSizeLabelValue = new QLabel();
    lightSizeLabelValue->setNum(lightSize);
    connect(lightSizeSlider,SIGNAL(valueChanged(int)),lightSizeLabelValue,SLOT(setNum(int)));
    QHBoxLayout *hboxLightSize= new QHBoxLayout;
    hboxLightSize->addWidget(lightSizeLabel);
    hboxLightSize->addWidget(lightSizeLabelValue);
    outer->addLayout(hboxLightSize);
    outer->addWidget(lightSizeSlider);

    // Maximum filter size for the shadow slider
    QSlider* maxFilterSizeSlider = new QSlider(Qt::Horizontal);
    maxFilterSizeSlider->setTickPosition(QSlider::TicksBelow);
    maxFilterSizeSlider->setTickInterval(1);
    maxFilterSizeSlider->setMinimum(1);
    maxFilterSizeSlider->setMaximum(30);
    maxFilterSizeSlider->setSliderPosition(maxFilterSize);
    connect(maxFilterSizeSlider,SIGNAL(valueChanged(int)),this,SLOT(updateMaxFilterSize(int)));
    QLabel* maxFilterSizeLabel = new QLabel("Maximum size of the filter used to compute the shadow : ");
    QLabel* maxFilterSizeLabelValue = new QLabel();
    maxFilterSizeLabelValue->setNum(maxFilterSize);
    connect(maxFilterSizeSlider,SIGNAL(valueChanged(int)),maxFilterSizeLabelValue,SLOT(setNum(int)));
    QHBoxLayout *hboxMaxFilterSize= new QHBoxLayout;
    hboxMaxFilterSize->addWidget(maxFilterSizeLabel);
    hboxMaxFilterSize->addWidget(maxFilterSizeLabelValue);
    outer->addLayout(hboxMaxFilterSize);
    outer->addWidget(maxFilterSizeSlider);

    // bias coefficient slider (for shadow acne)
    QSlider* biasCoeffSlider = new QSlider(Qt::Horizontal);
    biasCoeffSlider->setTickPosition(QSlider::TicksBelow);
    biasCoeffSlider->setTickInterval(1);
    biasCoeffSlider->setMinimum(1);
    biasCoeffSlider->setMaximum(30);
    biasCoeffSlider->setSliderPosition(biasCoeff);
    connect(biasCoeffSlider,SIGNAL(valueChanged(int)),this,SLOT(updateBiasCoeff(int)));
    QLabel* biasCoeffLabel = new QLabel("bias coefficient to avoid shadow acne : ");
    QLabel* biasCoeffLabelValue = new QLabel();
    biasCoeffLabelValue->setNum(biasCoeff);
    connect(biasCoeffSlider,SIGNAL(valueChanged(int)),biasCoeffLabelValue,SLOT(setNum(int)));
    QHBoxLayout *hboxBiasCoeff= new QHBoxLayout;
    hboxBiasCoeff->addWidget(biasCoeffLabel);
    hboxBiasCoeff->addWidget(biasCoeffLabelValue);
    outer->addLayout(hboxBiasCoeff);
    outer->addWidget(biasCoeffSlider);

    auxWidget->setLayout(outer);
    auxWidget->show();
}


void glShaderWindow::bindSceneToProgram()
{
    // Now, the model
    m_vao.bind();

    m_vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    m_vertexBuffer.bind();
    m_vertexBuffer.allocate(&(modelMesh->vertices.front()), modelMesh->vertices.size() * sizeof(trimesh::point));

    m_indexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    m_indexBuffer.bind();
    m_indexBuffer.allocate(&(modelMesh->faces.front()), modelMesh->faces.size() * 3 * sizeof(int));

    if (modelMesh->colors.size() > 0) {
        m_colorBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
        m_colorBuffer.bind();
        m_colorBuffer.allocate(&(modelMesh->colors.front()), modelMesh->colors.size() * sizeof(trimesh::Color));
    }

    m_normalBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    m_normalBuffer.bind();
    m_normalBuffer.allocate(&(modelMesh->normals.front()), modelMesh->normals.size() * sizeof(trimesh::vec));

    if (modelMesh->texcoords.size() > 0) {
        m_texcoordBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
        m_texcoordBuffer.bind();
        m_texcoordBuffer.allocate(&(modelMesh->texcoords.front()), modelMesh->texcoords.size() * sizeof(trimesh::vec2));
    }
    m_program->bind();
    // Enable the "vertex" attribute to bind it to our vertex buffer
    m_vertexBuffer.bind();
    m_program->setAttributeBuffer( "vertex", GL_FLOAT, 0, 3 );
    m_program->enableAttributeArray( "vertex" );

    // Enable the "color" attribute to bind it to our colors buffer
    if (modelMesh->colors.size() > 0) {
        m_colorBuffer.bind();
        m_program->setAttributeBuffer( "color", GL_FLOAT, 0, 3 );
        m_program->enableAttributeArray( "color" );
        m_program->setUniformValue("noColor", false);
    } else {
        m_program->setUniformValue("noColor", true);
    }
    m_normalBuffer.bind();
    m_program->setAttributeBuffer( "normal", GL_FLOAT, 0, 3 );
    m_program->enableAttributeArray( "normal" );

    if (modelMesh->texcoords.size() > 0) {
        m_texcoordBuffer.bind();
        m_program->setAttributeBuffer( "texcoords", GL_FLOAT, 0, 2 );
        m_program->enableAttributeArray( "texcoords" );
    }
    m_program->release();
    shadowMapGenerationProgram->bind();
    // Enable the "vertex" attribute to bind it to our vertex buffer
    m_vertexBuffer.bind();
    shadowMapGenerationProgram->setAttributeBuffer( "vertex", GL_FLOAT, 0, 3 );
    shadowMapGenerationProgram->enableAttributeArray( "vertex" );
    if (modelMesh->colors.size() > 0) {
        m_colorBuffer.bind();
        shadowMapGenerationProgram->setAttributeBuffer( "color", GL_FLOAT, 0, 3 );
        shadowMapGenerationProgram->enableAttributeArray( "color" );
    }
    m_normalBuffer.bind();
    shadowMapGenerationProgram->setAttributeBuffer( "normal", GL_FLOAT, 0, 3 );
    shadowMapGenerationProgram->enableAttributeArray( "normal" );
    if (modelMesh->texcoords.size() > 0) {
        m_texcoordBuffer.bind();
        shadowMapGenerationProgram->setAttributeBuffer( "texcoords", GL_FLOAT, 0, 2 );
        shadowMapGenerationProgram->enableAttributeArray( "texcoords" );
    }
    shadowMapGenerationProgram->release();
    m_vao.release();

    // Bind ground VAO to program as well
    ground_vao.bind();
    ground_vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ground_vertexBuffer.bind();
    trimesh::point center = modelMesh->bsphere.center;
    float radius = modelMesh->bsphere.r;

    int numR = 10;
    int numTh = 20;
    g_numPoints = numR * numTh;
    // Allocate once, fill in for every new model.
    if (g_vertices == 0) g_vertices = new trimesh::point[g_numPoints];
    if (g_normals == 0) g_normals = new trimesh::vec[g_numPoints];
    if (g_colors == 0) g_colors = new trimesh::point[g_numPoints];
    if (g_texcoords == 0) g_texcoords = new trimesh::vec2[g_numPoints];
    if (g_indices == 0) g_indices = new int[6 * g_numPoints];
    for (int i = 0; i < numR; i++) {
        for (int j = 0; j < numTh; j++) {
            int p = i + j * numR;
            g_normals[p] = trimesh::point(0, 1, 0);
            g_colors[p] = trimesh::point(0.6, 0.85, 0.9);
            float theta = (float)j * 2 * M_PI / numTh;
            float rad =  5.0 * radius * (float) i / numR;
            g_vertices[p] = center + trimesh::point(rad * cos(theta), - groundDistance * radius, rad * sin(theta));
            rad =  5.0 * (float) i / numR;
            g_texcoords[p] = trimesh::vec2(rad * cos(theta), rad * sin(theta));
        }
    }
    ground_vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ground_vertexBuffer.bind();
    ground_vertexBuffer.allocate(g_vertices, g_numPoints * sizeof(trimesh::point));
    ground_normalBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ground_normalBuffer.bind();
    ground_normalBuffer.allocate(g_normals, g_numPoints * sizeof(trimesh::point));
    ground_colorBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ground_colorBuffer.bind();
    ground_colorBuffer.allocate(g_colors, g_numPoints * sizeof(trimesh::point));
    ground_texcoordBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ground_texcoordBuffer.bind();
    ground_texcoordBuffer.allocate(g_texcoords, g_numPoints * sizeof(trimesh::vec2));

    g_numIndices = 0;
    for (int i = 0; i < numR - 1; i++) {
        for (int j = 0; j < numTh; j++) {
            int j_1 = (j + 1) % numTh;
            g_indices[g_numIndices++] = i + j * numR;
            g_indices[g_numIndices++] = i + 1 + j_1 * numR;
            g_indices[g_numIndices++] = i + 1 + j * numR;
            g_indices[g_numIndices++] = i + j * numR;
            g_indices[g_numIndices++] = i + j_1 * numR;
            g_indices[g_numIndices++] = i + 1 + j_1 * numR;
        }

    }
    ground_indexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ground_indexBuffer.bind();
    ground_indexBuffer.allocate(g_indices, g_numIndices * sizeof(int));

    ground_program->bind();
    ground_vertexBuffer.bind();
    ground_program->setAttributeBuffer( "vertex", GL_FLOAT, 0, 3 );
    ground_program->enableAttributeArray( "vertex" );
    ground_colorBuffer.bind();
    ground_program->setAttributeBuffer( "color", GL_FLOAT, 0, 3 );
    ground_program->enableAttributeArray( "color" );
    ground_normalBuffer.bind();
    ground_program->setAttributeBuffer( "normal", GL_FLOAT, 0, 3 );
    ground_program->enableAttributeArray( "normal" );
    ground_program->setUniformValue("noColor", false);
    ground_texcoordBuffer.bind();
    ground_program->setAttributeBuffer( "texcoords", GL_FLOAT, 0, 2 );
    ground_program->enableAttributeArray( "texcoords" );
    ground_program->release();
    // Also bind it to shadow mapping program:
    shadowMapGenerationProgram->bind();
    ground_vertexBuffer.bind();
    shadowMapGenerationProgram->setAttributeBuffer( "vertex", GL_FLOAT, 0, 3 );
    shadowMapGenerationProgram->enableAttributeArray( "vertex" );
    shadowMapGenerationProgram->release();
    ground_colorBuffer.bind();
    shadowMapGenerationProgram->setAttributeBuffer( "color", GL_FLOAT, 0, 3 );
    shadowMapGenerationProgram->enableAttributeArray( "color" );
    ground_normalBuffer.bind();
    shadowMapGenerationProgram->setAttributeBuffer( "normal", GL_FLOAT, 0, 3 );
    shadowMapGenerationProgram->enableAttributeArray( "normal" );
    ground_texcoordBuffer.bind();
    shadowMapGenerationProgram->setAttributeBuffer( "texcoords", GL_FLOAT, 0, 2 );
    shadowMapGenerationProgram->enableAttributeArray( "texcoords" );
    ground_program->release();
    ground_vao.release();
}
void glShaderWindow::initializeTransformForScene()
{
    // Set standard transformation and light source
    float radius = modelMesh->bsphere.r;
    m_perspective.setToIdentity();
    m_perspective.perspective(45, (float)width()/height(), 0.1 * radius, 20 * radius);
    QVector3D center = QVector3D(modelMesh->bsphere.center[0],
            modelMesh->bsphere.center[1],
            modelMesh->bsphere.center[2]);
    QVector3D eye = center + 2 * radius * QVector3D(0,0,1);
    m_matrix[0].setToIdentity();
    m_matrix[1].setToIdentity();
    m_matrix[2].setToIdentity();
    m_matrix[0].lookAt(eye, center, QVector3D(0,1,0));
    m_matrix[1].translate(-center);
}

void glShaderWindow::openScene()
{
    if (modelMesh) {
        delete(modelMesh);
        m_vertexBuffer.release();
        m_indexBuffer.release();
        m_colorBuffer.release();
        m_normalBuffer.release();
        m_texcoordBuffer.release();
        m_vao.release();
    }

    modelMesh = trimesh::TriMesh::read(qPrintable(modelName));
    if (!modelMesh) {
        QMessageBox::warning(0, tr("qViewer"),
                             tr("Could not load file ") + modelName, QMessageBox::Ok);
        openSceneFromFile();
    }
    modelMesh->need_bsphere();
    modelMesh->need_bbox();
    modelMesh->need_normals();
    modelMesh->need_faces();

    bindSceneToProgram();
    initializeTransformForScene();
}

void glShaderWindow::saveScene()
{
    QFileDialog dialog(0, "Save current scene", workingDirectory,
    "*.ply *.ray *.obj *.off *.sm *.stl *.cc *.dae *.c++ *.C *.c++");
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    QString filename;
    int ret = dialog.exec();
    if (ret == QDialog::Accepted) {
        workingDirectory = dialog.directory().path();
        filename = dialog.selectedFiles()[0];
    }
    if (!filename.isNull()) {
        if (!modelMesh->write(qPrintable(filename))) {
            QMessageBox::warning(0, tr("qViewer"),
                tr("Could not save file: ") + filename, QMessageBox::Ok);
        }
    }
}

void glShaderWindow::toggleFullScreen()
{
    fullScreenSnapshots = !fullScreenSnapshots;
}

void glShaderWindow::saveScreenshot()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    QPixmap pixmap;
    if (screen) {
        if (fullScreenSnapshots) pixmap = screen->grabWindow(winId());
        else pixmap = screen->grabWindow(winId(), x(), y(), width(), height());
    }
    QFileDialog dialog(0, "Save current picture", workingDirectory, "*.png *.jpg");
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    QString filename;
    int ret = dialog.exec();
    if (ret == QDialog::Accepted) {
        filename = dialog.selectedFiles()[0];
        if (!pixmap.save(filename)) {
            QMessageBox::warning(0, tr("qViewer"),
                tr("Could not save picture file: ") + filename, QMessageBox::Ok);
        }
    }
}

void glShaderWindow::setWindowSize(const QString& size)
{
    QStringList dims = size.split("x");
    resize(dims[0].toInt(), dims[1].toInt());
}

void glShaderWindow::setShader(const QString& shader)
{
    // Prepare a complete shader program...
    QDir shadersDir = QDir(":/");
    QString shader2 = shader + "*";
    QStringList shaders = shadersDir.entryList(QStringList(shader2));
    QString vertexShader;
    QString fragmentShader;
    foreach (const QString &str, shaders) {
        QString suffix = str.right(str.size() - str.lastIndexOf("."));
        if (m_vertShaderSuffix.filter(suffix).size() > 0) {
            vertexShader = ":/" + str;
        }
        if (m_fragShaderSuffix.filter(suffix).size() > 0) {
            fragmentShader = ":/" + str;
        }
    }
    m_program = prepareShaderProgram(vertexShader, fragmentShader);
    bindSceneToProgram();
    loadTexturesForShaders();
    renderNow();
}

void glShaderWindow::loadTexturesForShaders() {
    m_program->bind();
    if (texture) {
        glActiveTexture(GL_TEXTURE0);
        texture->release();
        texture->destroy();
        delete texture;
        texture = 0;
    }
    if (permTexture) {
        glActiveTexture(GL_TEXTURE1);
        permTexture->release();
        permTexture->destroy();
        delete permTexture;
        permTexture = 0;
    }
    if (environmentMap) {
        glActiveTexture(GL_TEXTURE1);
        environmentMap->release();
        environmentMap->destroy();
        delete environmentMap;
        environmentMap = 0;
    }
    if (normalMap) {
        glActiveTexture(GL_TEXTURE1);
        normalMap->release();
        normalMap->destroy();
        delete normalMap;
        normalMap = 0;
    }
    // load textures as required by the shader.
    if (m_program->uniformLocation("earthDay") != -1) {
        // the shader is about the earth. We load the related textures (day + relief)
        glActiveTexture(GL_TEXTURE0);
        texture = new QOpenGLTexture(QImage(workingDirectory + "../textures/earth1.png"));
        if (texture) {
            texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
            texture->setMagnificationFilter(QOpenGLTexture::Linear);
            texture->setWrapMode(QOpenGLTexture::MirroredRepeat);
            texture->bind(0);
            m_program->setUniformValue("earthDay", 0);
        }
        glActiveTexture(GL_TEXTURE1);
        normalMap = new QOpenGLTexture(QImage(workingDirectory + "../textures/earth3.png"));
        if (normalMap) {
            normalMap->setWrapMode(QOpenGLTexture::MirroredRepeat);
            normalMap->setMagnificationFilter(QOpenGLTexture::LinearMipMapLinear);
            normalMap->setMinificationFilter(QOpenGLTexture::Linear);
            normalMap->bind(1);
            m_program->setUniformValue("earthNormals", 1);
        }
    } else {
        if ((m_program->uniformLocation("colorTexture") != -1) || (ground_program->uniformLocation("colorTexture") != -1)) {
            // the shader wants a texture. We load one.
            glActiveTexture(GL_TEXTURE0);
            texture = new QOpenGLTexture(QImage(textureName));
            if (texture) {
                texture->setWrapMode(QOpenGLTexture::Repeat);
                texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
                texture->setMagnificationFilter(QOpenGLTexture::Linear);
                texture->bind(0);
                if (m_program->uniformLocation("colorTexture") != -1) m_program->setUniformValue("colorTexture", 0);
                if (ground_program->uniformLocation("colorTexture") != -1) ground_program->setUniformValue("colorTexture", 0);
            }
        }
        if (m_program->uniformLocation("normalMap") != -1) {
            // the shader wants an environment map, we load one.
            glActiveTexture(GL_TEXTURE1);
            normalMap = new QOpenGLTexture(QImage(normalMapName));
            if (normalMap) {
                normalMap->setWrapMode(QOpenGLTexture::MirroredRepeat);
                normalMap->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
                normalMap->setMagnificationFilter(QOpenGLTexture::Nearest);
                normalMap->bind(1);
                m_program->setUniformValue("normalMap", 1);
            }
        }
        if (m_program->uniformLocation("envMap") != -1) {
            // the shader wants an environment map, we load one.
            glActiveTexture(GL_TEXTURE1);
            environmentMap = new QOpenGLTexture(QImage(envMapName).mirrored());
            if (environmentMap) {
                environmentMap->setWrapMode(QOpenGLTexture::MirroredRepeat);
                environmentMap->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
                environmentMap->setMagnificationFilter(QOpenGLTexture::Nearest);
                environmentMap->bind(1);
                m_program->setUniformValue("envMap", 1);
            }
        } else {
        // for Perlin noise
            if (m_program->uniformLocation("permTexture") != -1) {
                glActiveTexture(GL_TEXTURE1);
                permTexture = new QOpenGLTexture(QImage(pixels, 256, 256, QImage::Format_RGBA8888));
                if (permTexture) {
                    permTexture->setWrapMode(QOpenGLTexture::MirroredRepeat);
                    permTexture->setMinificationFilter(QOpenGLTexture::Nearest);
                    permTexture->setMagnificationFilter(QOpenGLTexture::Nearest);
                    permTexture->bind(1);
                    m_program->setUniformValue("permTexture", 1);
                }
            }
        }
    }
}

void glShaderWindow::initialize()
{
    // Debug: which OpenGL version are we running? Must be >= 3.2 for this code to work.
    // qDebug("OpenGL initialized: version: %s GLSL: %s", glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));
    // Set the clear color to black
    glClearColor( 0.2f, 0.2f, 0.2f, 1.0f );
    glEnable (GL_CULL_FACE); // cull face
    glCullFace (GL_BACK); // cull back face
    glFrontFace (GL_CCW); // GL_CCW for counter clock-wise
    glEnable (GL_DEPTH_TEST); // z_buffer
    glEnable (GL_MULTISAMPLE);

    // Prepare a complete shader program...
    // We can't call setShader because of initialization issues
    if (m_program) {
        m_program->release();
        delete(m_program);
    }
    m_program = prepareShaderProgram(":/PCF.vert", ":/PCF.frag");
    if (ground_program) {
        ground_program->release();
        delete(ground_program);
    }
    ground_program = prepareShaderProgram(":/textured_PCF.vert", ":/textured_PCF.frag");
    if (shadowMapGenerationProgram) {
        shadowMapGenerationProgram->release();
        delete(shadowMapGenerationProgram);
    }
    shadowMapGenerationProgram = prepareShaderProgram(":/h_shadowMapGeneration.vert", ":/h_shadowMapGeneration.frag");

    // loading texture:
    loadTexturesForShaders();

    m_vao.create();
    m_vao.bind();
    m_vertexBuffer.create();
    m_indexBuffer.create();
    m_colorBuffer.create();
    m_normalBuffer.create();
    m_texcoordBuffer.create();
    if (width() > height()) m_screenSize = width(); else m_screenSize = height();
    initPermTexture(); // create Perlin noise texture
    m_vao.release();

    ground_vao.create();
    ground_vao.bind();
    ground_vertexBuffer.create();
    ground_indexBuffer.create();
    ground_colorBuffer.create();
    ground_normalBuffer.create();
    ground_texcoordBuffer.create();
    ground_vao.release();

    openScene();
}

void glShaderWindow::resizeEvent(QResizeEvent* event)
{
   OpenGLWindow::resizeEvent(event);
   resize(event->size().width(), event->size().height());
}

void glShaderWindow::resize(int x, int y)
{
    OpenGLWindow::resize(x,y);
    if (x > y) m_screenSize = x; else m_screenSize = y;
    if (m_program && modelMesh) {
        QMatrix4x4 persp;
        float radius = modelMesh->bsphere.r;
        m_program->bind();
        m_perspective.setToIdentity();
        if (x > y)
            m_perspective.perspective(60, (float)x/y, 0.1 * radius, 20 * radius);
        else {
            m_perspective.perspective((240.0/M_PI) * atan((float)y/x), (float)x/y, 0.1 * radius, 20 * radius);
        }
        m_program->setUniformValue("perspective", m_perspective);
        renderNow();
    }
}

QOpenGLShaderProgram* glShaderWindow::prepareShaderProgram(const QString& vertexShaderPath, const QString& fragmentShaderPath)
{
    QOpenGLShaderProgram* program = new QOpenGLShaderProgram(this);
    if (!program) qWarning() << "Failed to allocate the shader";
    bool result = program->addShaderFromSourceFile(QOpenGLShader::Vertex, vertexShaderPath);
    if ( !result )
        qWarning() << program->log();
    result = program->addShaderFromSourceFile(QOpenGLShader::Fragment, fragmentShaderPath);
    if ( !result )
        qWarning() << program->log();
    result = program->link();
    if ( !result )
        qWarning() << program->log();
    program->bind();

    return program;
}

void glShaderWindow::setWorkingDirectory(QString& myPath, QString& myName, QString& texture, QString& envMap)
{
    workingDirectory = myPath;
    modelName = myPath + myName;
    textureName = myPath + "../textures/" + texture;
    envMapName = myPath + "../textures/" + envMap;
}

void glShaderWindow::mouseToTrackball(QVector2D &mousePosition, QVector3D &spacePosition)
{
    const float tbRadius = 0.8f;
    float r2 = mousePosition.x() * mousePosition.x() + mousePosition.y() * mousePosition.y();
    const float t2 = tbRadius * tbRadius / 2.0;
    spacePosition = QVector3D(mousePosition.x(), mousePosition.y(), 0.0);
    if (r2 < t2) {
        spacePosition.setZ(sqrt(2.0 * t2 - r2));
    } else {
        spacePosition.setZ(t2 / sqrt(r2));
    }
}

// virtual trackball implementation
void glShaderWindow::mousePressEvent(QMouseEvent *e)
{
    lastMousePosition = (2.0/m_screenSize) * (QVector2D(e->localPos()) - QVector2D(0.5 * width(), 0.5*height()));
    mouseToTrackball(lastMousePosition, lastTBPosition);
    mouseButton = e->button();
}

void glShaderWindow::wheelEvent(QWheelEvent * ev)
{
    int matrixMoving = 0;
    if (ev->modifiers() & Qt::ShiftModifier) matrixMoving = 1;
    else if (ev->modifiers() & Qt::AltModifier) matrixMoving = 2;

    QPoint numDegrees = ev->angleDelta() /(float) (8 * 3.0);
    if (matrixMoving == 0) {
        QMatrix4x4 t;
        t.translate(0.0, 0.0, numDegrees.y() * modelMesh->bsphere.r / 100.0);
        m_matrix[matrixMoving] = t * m_matrix[matrixMoving];
    } else  if (matrixMoving == 1) {
        lightDistance -= 0.1 * numDegrees.y();
    } else  if (matrixMoving == 2) {
        groundDistance += 0.1 * numDegrees.y();
    }
    renderNow();
}

void glShaderWindow::mouseMoveEvent(QMouseEvent *e)
{
    if (mouseButton == Qt::NoButton) return;
    QVector2D mousePosition = (2.0/m_screenSize) * (QVector2D(e->localPos()) - QVector2D(0.5 * width(), 0.5*height()));
    QVector3D currTBPosition;
    mouseToTrackball(mousePosition, currTBPosition);
    int matrixMoving = 0;
    if (e->modifiers() & Qt::ShiftModifier) matrixMoving = 1;
    else if (e->modifiers() & Qt::AltModifier) matrixMoving = 2;

    switch (mouseButton) {
    case Qt::LeftButton: {
        QVector3D rotAxis = QVector3D::crossProduct(lastTBPosition, currTBPosition);
        float rotAngle = (180.0/M_PI) * rotAxis.length() /(lastTBPosition.length() * currTBPosition.length()) ;
        rotAxis.normalize();
        QQuaternion rotation = QQuaternion::fromAxisAndAngle(rotAxis, rotAngle);
        m_matrix[matrixMoving].translate(modelMesh->bsphere.center[0],
                modelMesh->bsphere.center[1],
                modelMesh->bsphere.center[2]);
        m_matrix[matrixMoving].rotate(rotation);
        m_matrix[matrixMoving].translate(- modelMesh->bsphere.center[0],
                - modelMesh->bsphere.center[1],
                - modelMesh->bsphere.center[2]);
        break;
    }
    case Qt::RightButton: {
        QVector2D diff = 0.2 * m_screenSize * (mousePosition - lastMousePosition);
        if (matrixMoving == 0) {
            QMatrix4x4 t;
            t.translate(diff.x() * modelMesh->bsphere.r / 100.0, -diff.y() * modelMesh->bsphere.r / 100.0, 0.0);
            m_matrix[matrixMoving] = t * m_matrix[matrixMoving];
        } else if (matrixMoving == 1) {
            lightDistance += 0.1 * diff.y();
        } else  if (matrixMoving == 2) {
            groundDistance += 0.1 * diff.y();
        }
        break;
    }
    }
    lastTBPosition = currTBPosition;
    lastMousePosition = mousePosition;
    renderNow();
}

void glShaderWindow::mouseReleaseEvent(QMouseEvent *e)
{
    mouseButton = Qt::NoButton;
}

void glShaderWindow::timerEvent(QTimerEvent *e)
{

}


void glShaderWindow::render()
{
    QOpenGLTexture* sm = 0;
    m_program->bind();
    QVector3D center = QVector3D(modelMesh->bsphere.center[0],
            modelMesh->bsphere.center[1],
            modelMesh->bsphere.center[2]);
    QVector3D lightPosition = m_matrix[1] * (center + lightDistance * modelMesh->bsphere.r * QVector3D(0, 0, 1));

    QMatrix4x4 lightCoordMatrix;
    QMatrix4x4 lightPerspective;
    float nearPlane;
    float farPlane;
    if ((ground_program->uniformLocation("shadowMap") != -1) || (m_program->uniformLocation("shadowMap") != -1) ){
        glActiveTexture(GL_TEXTURE2);
        glViewport(0, 0, shadowMapDimension, shadowMapDimension);
        // The shader wants a shadow map.
        if (!shadowMap) {
            // create FBO for shadow map:
            QOpenGLFramebufferObjectFormat shadowMapFormat;
            shadowMapFormat.setAttachment(QOpenGLFramebufferObject::Depth);
            shadowMapFormat.setTextureTarget(GL_TEXTURE_2D);
            shadowMapFormat.setInternalTextureFormat(GL_RGBA32F_ARB);
            // shadowMapFormat.setInternalTextureFormat(GL_DEPTH_COMPONENT);
            shadowMap = new QOpenGLFramebufferObject(shadowMapDimension, shadowMapDimension, shadowMapFormat);
        }
        // Render into shadow Map
        m_program->release();
        ground_program->release();
        shadowMapGenerationProgram->bind();
        if (!shadowMap->bind()) {
            std::cerr << "Can't render in the shadow map" << std::endl;
        }
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f );
        glDisable(GL_CULL_FACE);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // set up camera position in light source:
        // TODO_TP3: you must initialize these two matrices.
        lightCoordMatrix.lookAt(lightPosition, center, QVector3D(0,0,1));

        float radius = modelMesh->bsphere.r;
        float fovy = 2.0 * (180.0/M_PI) * atan2(radius, lightDistance);

        //lemming
        /*
        float nearPlane = (lightDistance * radius - radius) * 0.7;
        float farPlane = (lightDistance * radius + radius) * 1.5;
        lightPerspective.perspective(fovy, 1.0, nearPlane, farPlane);
        */

        //buddha

        float nearPlane = 300;
        float farPlane = 800;
        lightPerspective.perspective(40, 1.0, nearPlane, farPlane);


        shadowMapGenerationProgram->setUniformValue("matrix", lightCoordMatrix);
        shadowMapGenerationProgram->setUniformValue("perspective", lightPerspective);
        // Draw the entire scene:
        m_vao.bind();
        glDrawElements(GL_TRIANGLES, 3 * modelMesh->faces.size(), GL_UNSIGNED_INT, 0);
        m_vao.release();
        ground_vao.bind();
        glDrawElements(GL_TRIANGLES, g_numIndices, GL_UNSIGNED_INT, 0);
        ground_vao.release();
        glFinish();
        // done. Back to normal drawing.
        shadowMapGenerationProgram->release();
        // add shadow map as texture:
        shadowMap->bindDefault();
#define CRUDE_BUT_WORKS
#ifdef CRUDE_BUT_WORKS
        // That one works, but slow. In theory not required.
        QImage debugPix = shadowMap->toImage();
        QOpenGLTexture* sm = new QOpenGLTexture(QImage(debugPix));
        sm->bind(shadowMap->texture());
        sm->setWrapMode(QOpenGLTexture::ClampToEdge);
        //debugPix.save("debug.png");
#endif
        glClearColor( 0.2f, 0.2f, 0.2f, 1.0f );
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK); // cull back face
    }
    m_program->bind();
    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_program->setUniformValue("lightPosition", lightPosition);
    m_program->setUniformValue("matrix", m_matrix[0]);
    m_program->setUniformValue("perspective", m_perspective);
    m_program->setUniformValue("lightMatrix", m_matrix[1]);
    m_program->setUniformValue("normalMatrix", m_matrix[0].normalMatrix());
    m_program->setUniformValue("lightIntensity", 1.0f);
    m_program->setUniformValue("noiseRate", noiseRate);
    m_program->setUniformValue("noisePersistence", noisePersistence);
    m_program->setUniformValue("blinnPhong", blinnPhong);
    m_program->setUniformValue("cookTorrance", cookTorrance);
    m_program->setUniformValue("gooch", gooch);
    m_program->setUniformValue("toon", toon);
    m_program->setUniformValue("cartesianCoo", cartesianCoo);
    m_program->setUniformValue("sphericalCoo", sphericalCoo);
    m_program->setUniformValue("noiseNormal", noiseNormal);
    m_program->setUniformValue("noiseMarble", noiseMarble);
    m_program->setUniformValue("noiseJade", noiseJade);
    m_program->setUniformValue("noiseWood", noiseWood);
    m_program->setUniformValue("transparent", transparent);
    m_program->setUniformValue("withNoise", withNoise);
    m_program->setUniformValue("lightIntensity", lightIntensity);
    m_program->setUniformValue("shininess", shininess);
    m_program->setUniformValue("eta", eta);
    m_program->setUniformValue("roughness", roughness);
    m_program->setUniformValue("radius", modelMesh->bsphere.r);
    m_program->setUniformValue("lightSize", lightSize);
    m_program->setUniformValue("maxFilterSize", maxFilterSize);
    m_program->setUniformValue("biasCoeff", biasCoeff);
    // Shadow Mapping
    if (m_program->uniformLocation("shadowMap") != -1) {
        m_program->setUniformValue("shadowMap", shadowMap->texture());
        // TODO_TP3: send the right transform here
        m_program->setUniformValue("worldToLightSpace", lightPerspective * lightCoordMatrix);
        m_program->setUniformValue("nearPlane", nearPlane);
        m_program->setUniformValue("farPlane", farPlane);
    }

    m_vao.bind();
    glDrawElements(GL_TRIANGLES, 3 * modelMesh->faces.size(), GL_UNSIGNED_INT, 0);
    m_vao.release();
    m_program->release();

    if (m_program->uniformLocation("earthDay") == -1 && m_program->uniformLocation("normalMap") == -1) {
        glActiveTexture(GL_TEXTURE0);
        ground_program->bind();
        ground_program->setUniformValue("lightPosition", lightPosition);
        ground_program->setUniformValue("matrix", m_matrix[0]);
        ground_program->setUniformValue("lightMatrix", m_matrix[1]);
        ground_program->setUniformValue("perspective", m_perspective);
        ground_program->setUniformValue("normalMatrix", m_matrix[0].normalMatrix());
        ground_program->setUniformValue("lightIntensity", 1.0f);
        ground_program->setUniformValue("blinnPhong", blinnPhong);
        ground_program->setUniformValue("cookTorrance", cookTorrance);
        ground_program->setUniformValue("gooch", gooch);
        ground_program->setUniformValue("toon", toon);
        ground_program->setUniformValue("transparent", transparent);
        ground_program->setUniformValue("lightIntensity", lightIntensity);
        ground_program->setUniformValue("shininess", shininess);
        ground_program->setUniformValue("roughness", roughness);
        ground_program->setUniformValue("noiseRate", noiseRate);
        ground_program->setUniformValue("eta", eta);
        ground_program->setUniformValue("radius", modelMesh->bsphere.r);
        ground_program->setUniformValue("lightSize", lightSize);
        ground_program->setUniformValue("maxFilterSize", maxFilterSize);
        ground_program->setUniformValue("biasCoeff", biasCoeff);
        if (ground_program->uniformLocation("colorTexture") != -1) ground_program->setUniformValue("colorTexture", 0);
        if (ground_program->uniformLocation("shadowMap") != -1) {
            ground_program->setUniformValue("shadowMap", shadowMap->texture());
            // TODO_TP3: send the right transform here
            ground_program->setUniformValue("worldToLightSpace", lightPerspective * lightCoordMatrix);
            //m_program->setUniformValue("nearPlane", nearPlane);
            //m_program->setUniformValue("farPlane", farPlane);
            ground_program->setUniformValue("nearPlane", nearPlane);
            ground_program->setUniformValue("farPlane", farPlane);
        }
        ground_vao.bind();
        glDrawElements(GL_TRIANGLES, g_numIndices, GL_UNSIGNED_INT, 0);
        ground_vao.release();
        ground_program->release();
    }

#ifdef CRUDE_BUT_WORKS
    if (sm) {
        sm->release();
        delete sm;
    }
#endif
}
