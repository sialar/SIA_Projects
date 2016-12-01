#include <QApplication>
#include <QGLFormat>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QSignalMapper>
#include <QDir>

#include "glshaderwindow.h"

static QSignalMapper sizeMapper;
static QSignalMapper shaderMapper;

void printUsage(const char *myname)
{
    fprintf(stderr, "\n");
    fprintf(stderr, "\n Usage    : %s [infile]\n", myname);
    exit(1);
}

void setupFileMenu(QMenuBar* myMenuBar, glShaderWindow* glWindow, QApplication *myApp)
{
    QMenu* fileMenu = myMenuBar->addMenu(myMenuBar->tr("&File"));
    // Open Scene
    QAction* openSceneAction = new QAction(myMenuBar->tr("&Open Scene"), fileMenu);
    openSceneAction->setShortcuts(QKeySequence::Open);
    openSceneAction->setStatusTip(myMenuBar->tr("&Opens an existing scene file"));
    glWindow->connect(openSceneAction, SIGNAL(triggered()), glWindow, SLOT(openSceneFromFile()));
    fileMenu->addAction(openSceneAction);
    // Load new texture
    QAction* openTextureAction = new QAction(myMenuBar->tr("&Load texture"), fileMenu);
    openTextureAction->setStatusTip(myMenuBar->tr("&Opens a new texture image file"));
    glWindow->connect(openTextureAction, SIGNAL(triggered()), glWindow, SLOT(openNewTexture()));
    fileMenu->addAction(openTextureAction);
    // Load new normal map
    QAction* openNormalMapAction = new QAction(myMenuBar->tr("&Load normal map"), fileMenu);
    openNormalMapAction->setStatusTip(myMenuBar->tr("&Opens a new normal map image file"));
    glWindow->connect(openNormalMapAction, SIGNAL(triggered()), glWindow, SLOT(openNewNormalMap()));
    fileMenu->addAction(openNormalMapAction);
    // Load new environment Map
    QAction* openEnvMapAction = new QAction(myMenuBar->tr("&Load environment map"), fileMenu);
    openEnvMapAction->setStatusTip(myMenuBar->tr("&Opens a new environment map file"));
    glWindow->connect(openEnvMapAction, SIGNAL(triggered()), glWindow, SLOT(openNewEnvMap()));
    fileMenu->addAction(openEnvMapAction);
    // Save screenshot
    QAction* saveImageAction = new QAction(myMenuBar->tr("&Save screenshot"), fileMenu);
    saveImageAction->setShortcuts(QKeySequence::Print);
    saveImageAction->setStatusTip(myMenuBar->tr("&Saves a copy of the screen to a file"));
    glWindow->connect(saveImageAction, SIGNAL(triggered()), glWindow, SLOT(saveScreenshot()));
    fileMenu->addAction(saveImageAction);
    // Fullscreen or just picture?
    QAction* toggleFullscreenAction = new QAction(myMenuBar->tr("&Full screen screenshot"), fileMenu);
    toggleFullscreenAction->setCheckable(true);
    toggleFullscreenAction->setStatusTip(myMenuBar->tr("&Save only the active window, or the entire screen"));
    glWindow->connect(toggleFullscreenAction, SIGNAL(changed()), glWindow, SLOT(toggleFullScreen()));
    fileMenu->addAction(toggleFullscreenAction);
    // Save Scene
    QAction* saveSceneAction = new QAction(myMenuBar->tr("&Save Scene"), fileMenu);
    saveSceneAction->setShortcuts(QKeySequence::Save);
    saveSceneAction->setStatusTip(myMenuBar->tr("&Saves current scene file"));
    glWindow->connect(saveSceneAction, SIGNAL(triggered()), glWindow, SLOT(saveScene()));
    fileMenu->addAction(saveSceneAction);
    // Quit program
    QAction* quitAction = new QAction(myMenuBar->tr("&Quit"), fileMenu);
    quitAction->setShortcuts(QKeySequence::Quit);
    quitAction->setStatusTip(myMenuBar->tr("&Quit program"));
    glWindow->connect(quitAction, SIGNAL(triggered()), myApp, SLOT(quit()));
    fileMenu->addAction(quitAction);
}

void setupWindowMenu(QMenuBar* myMenuBar, glShaderWindow* glWindow)
{
    QMenu* sizeMenu = myMenuBar->addMenu(myMenuBar->tr("&Window size"));
    // Window sizes.
    QStringList sizes = QStringList() << "512x512" << "480x480" << "640x480" << "800x600" <<
                                         "1024x768" << "1024x1024" << "1280x1024";
    QActionGroup* setSizeAction = new QActionGroup(sizeMenu);
    setSizeAction->setExclusive(true);
    foreach (const QString& sizeName, sizes) {
        QAction* action = setSizeAction->addAction(sizeName);
        action->setCheckable(true);
        if (sizeName == "640x480") action->setChecked(true);
        glWindow->connect(action, SIGNAL(triggered()), &sizeMapper, SLOT(map()));
        sizeMapper.setMapping(action, sizeName);
        sizeMenu->addAction(action);
    }
    QAction* showAuxWin = setSizeAction->addAction("show parameters");
    showAuxWin->setShortcut(Qt::CTRL + Qt::Key_A);
    glWindow->connect(showAuxWin, SIGNAL(triggered()), glWindow, SLOT(showAuxWindow()));
    sizeMenu->addAction(showAuxWin);

    glWindow->connect(&sizeMapper, SIGNAL(mapped(const QString&)), glWindow, SLOT(setWindowSize(const QString&)));
}

void setupShaderMenu(QMenuBar* myMenuBar, glShaderWindow* glWindow) {
    // list of all shaders
    QMenu* shaderMenu = myMenuBar->addMenu(myMenuBar->tr("&Shaders"));
    // Take all shaders from resource file:
    QDir shadersDir = QDir(":/");
    // Oldest shaders first
    QStringList fragShaders = shadersDir.entryList(glWindow->fragShaderSuffix(), QDir::Files|QDir::Readable, QDir::Name);
    QStringList vertShaders = shadersDir.entryList(glWindow->vertShaderSuffix(), QDir::Files|QDir::Readable, QDir::Name);
    QActionGroup* setShaderAction = new QActionGroup(shaderMenu);
    foreach (const QString &shaderName, fragShaders) {
        QString baseName = shaderName.left(shaderName.lastIndexOf("."));
        // is it one the "hidden" shaders (deferred shading, shadow generation) ?
        if (baseName.indexOf("h_") != 0) {
            // is there a vertex shader with the same name?
            if (vertShaders.filter(baseName).size() > 0) {
                // Note that we only check for existence, not compilation
                QAction* action = setShaderAction->addAction(baseName);
                action->setCheckable(true);
                if (baseName == "1_simple") action->setChecked(true);
                glWindow->connect(action, SIGNAL(triggered()), &shaderMapper, SLOT(map()));
                shaderMapper.setMapping(action, baseName);
                shaderMenu->addAction(action);
            }
        }
    }
    glWindow->connect(&shaderMapper, SIGNAL(mapped(const QString&)), glWindow, SLOT(setShader(const QString&)));
}


int main( int argc, char* argv[] )
{
    setlocale(LC_ALL,"C");
    QApplication app(argc, argv);
    QString sceneName = "buddha50K.ply";
    QString textureName = "pixar4.png";
    QString normalMapName = "metal_crosshatch_pattern_6190173.JPG";    
    QString envMapName = "pisa.png";

    // Read scene name from arguments:
    QStringList arguments = app.arguments();
    for (int i = 1; i < arguments.size(); i++)
    {
        const QString& arg = arguments[i];
        if (i == 1 && !arg.startsWith("-")) {
            sceneName = arg;
        } else printUsage(argv[0]);
    }

    // Specify an OpenGL 4.1 format using the Core profile.
    // That is, no old-school fixed pipeline functionality
    QSurfaceFormat format;
    format.setSamples(16); // for anti-aliasing
    format.setDepthBufferSize(24);
    format.setVersion(3,0);
    format.setProfile(QSurfaceFormat::CoreProfile);

    // Create GL shader window and add it to application
    glShaderWindow window;
    window.setFormat(format);
    window.resize(640, 480);
    QString appPath = app.applicationDirPath();
#ifdef __APPLE__
    appPath = appPath + "/../../../models/";
#else
    appPath = appPath + "/models/";
#endif
    window.setWorkingDirectory(appPath, sceneName, textureName, normalMapName, envMapName);
    window.show();

    // Menu bar to be shared between all windows.
    QMenuBar *menuBar = new QMenuBar(0);
    setupFileMenu(menuBar, &window, &app);
    setupWindowMenu(menuBar, &window);
    setupShaderMenu(menuBar, &window);
    menuBar->show();
    window.showAuxWindow();

    return app.exec();
}
