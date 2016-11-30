#ifndef GLSHADERWINDOW_H
#define GLSHADERWINDOW_H

#include "openglwindow.h"
#include "TriMesh.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QMatrix4x4>
#include <QtGui/QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QtGui/QOpenGLVertexArrayObject>
#include <QtGui/QScreen>
#include <QMouseEvent>


class glShaderWindow : public OpenGLWindow
{
    Q_OBJECT
public:
    glShaderWindow(QWindow *parent = 0);
    ~glShaderWindow();

    void initialize();
    void render();
    void resize(int x, int y);
    void setWorkingDirectory(QString& myPath, QString& myName, QString& texture, QString& normalMapName, QString& envMap);
    inline const QStringList& fragShaderSuffix() { return m_fragShaderSuffix;};
    inline const QStringList& vertShaderSuffix() { return m_vertShaderSuffix;};

public slots:
    void openSceneFromFile();
    void openNewTexture();
    void openNewNormalMap();
    void openNewEnvMap();
    void saveScene();
    void toggleFullScreen();
    void saveScreenshot();
    void showAuxWindow();
    void setWindowSize(const QString& size);
    void setShader(const QString& size);
    void phongClicked();
    void blinnPhongClicked();
    void cookTorranceClicked();
    void goochClicked();
    void toonClicked();
    void transparentClicked();
    void noiseMarbleClicked();
    void noiseJadeClicked();
    void noiseWoodClicked();
    void noiseNormalClicked();
    void withNoiseClicked();
    void fromTextureClicked();
    void cartesianCooClicked();
    void sphericalCooClicked();
    void opaqueClicked();
    void PCSSClicked();
    void VSMClicked();
    void PCFClicked();
    void ESMClicked();
    void updateLightIntensity(int lightSliderValue);
    void updateShininess(int shininessSliderValue);
    void updateEta(int etaSliderValue);
    void updateRoughness(int roughnessSliderValue);
    void updateNoiseRate(int noiseRateSliderValue);
    void updateNoisePersistence(int noisePersistenceSliderValue);
    void updateLightSize(int lightSizeSliderValue);
    void updateMaxFilterSize(int maxFilterSizeSliderValue);
    void updateBiasCoeff(int biasCoeffSliderValue);


protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void timerEvent(QTimerEvent *e);
    void resizeEvent(QResizeEvent * ev);
    void wheelEvent(QWheelEvent * ev);


private:
    QOpenGLShaderProgram* prepareShaderProgram(const QString& vertexShaderPath, const QString& fragmentShaderPath);
    void bindSceneToProgram();
    void initializeTransformForScene();
    void initPermTexture();
    void loadTexturesForShaders();
    void openScene();
    void mouseToTrackball(QVector2D &in, QVector3D &out);

    // Model we are displaying:
    QString  workingDirectory;
    QString  modelName;
    QString  textureName;
    QString  normalMapName;
    QString  envMapName;
    trimesh::TriMesh* modelMesh;
    uchar* pixels;
    // Ground
    trimesh::point *g_vertices;
    trimesh::vec *g_normals;
    trimesh::vec2 *g_texcoords;
    trimesh::point *g_colors;
    int *g_indices;
    int g_numPoints;
    int g_numIndices;
    // Parameters controlled by UI
    bool blinnPhong;
    bool gooch;
    bool toon;
    bool cookTorrance;
    bool transparent;
    bool noiseMarble;
    bool noiseJade;
    bool noiseWood;
    bool sphericalCoo;
    bool cartesianCoo;
    bool noiseNormal;
    bool withNoise;
    bool PCSS;
    bool VSM;
    bool ESM;

    float eta;
    float roughness;
    float noiseRate;
    float noisePersistence;
    float lightIntensity;
    float shininess;
    float lightDistance;
    float groundDistance;
    int lightSize;
    int maxFilterSize;
    int biasCoeff;

    // OpenGL variables encapsulated by Qt
    QOpenGLShaderProgram *m_program;
    QOpenGLShaderProgram *ground_program;
    QOpenGLShaderProgram *shadowMapGenerationProgram;
    QOpenGLTexture* environmentMap;
    QOpenGLTexture* texture;
    QOpenGLTexture* normalMap;
    QOpenGLTexture* permTexture;
    // Model
    QOpenGLBuffer m_vertexBuffer;
    QOpenGLBuffer m_indexBuffer;
    QOpenGLBuffer m_normalBuffer;
    QOpenGLBuffer m_colorBuffer;
    QOpenGLBuffer m_texcoordBuffer;
    QOpenGLVertexArrayObject m_vao;
    // Ground
    QOpenGLVertexArrayObject ground_vao;
    QOpenGLBuffer ground_vertexBuffer;
    QOpenGLBuffer ground_indexBuffer;
    QOpenGLBuffer ground_normalBuffer;
    QOpenGLBuffer ground_colorBuffer;
    QOpenGLBuffer ground_texcoordBuffer;
    // Matrix for all objects
    QMatrix4x4 m_matrix[3]; // 0 = object, 1 = light, 2 = ground
    QMatrix4x4 m_perspective;
    // Shadow mapping
    QOpenGLFramebufferObject* shadowMap;
    int shadowMapDimension;

    // User interface variables
    bool fullScreenSnapshots;
    QStringList m_fragShaderSuffix;
    QStringList m_vertShaderSuffix;
    QVector2D lastMousePosition;
    QVector3D lastTBPosition;
    Qt::MouseButton mouseButton;
    float m_screenSize; // max window dimension
    QWidget* auxWidget; // window for parameters

};

#endif // GLSHADERWINDOW_H
