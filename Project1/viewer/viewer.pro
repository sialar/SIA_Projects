DESTDIR = ../../Project1/viewer
QT       += core gui opengl

TARGET = myViewer
TEMPLATE = app

macx {
  QMAKE_CXXFLAGS += -Wno-unknown-pragmas
} else {
  QMAKE_LFLAGS += -Wno-unknown-pragmas -fopenmp
}

SOURCES +=  \
            src/main.cpp \
            src/openglwindow.cpp \
            src/glshaderwindow.cpp

HEADERS  += \
            src/openglwindow.h \
            src/glshaderwindow.h \
    src/perlinNoise.h

RESOURCES += shaders/core-profile.qrc
OTHER_FILES +=  \
                shaders/brick.vert \
                shaders/brick.frag \
                shaders/noiseMarble.vert \
                shaders/noiseMarble.frag \
                shaders/noiseJade.vert \
                shaders/noiseJade.frag \
                shaders/noiseWood.frag \
    shaders/1_simple.frag \
    shaders/1_simple.vert \
    shaders/2_phong.frag \
    shaders/2_phong.vert \
    shaders/3_textured.frag \
    shaders/3_textured.vert \
    shaders/4_earth.frag \
    shaders/4_earth.vert \
    shaders/5_envMap.frag \
    shaders/5_envMap.vert \
    shaders/noiseAlone.frag \
    shaders/noiseAlone.vert \
    shaders/illumination.frag \
    shaders/illumination.vert \
    shaders/h_shadowMapGeneration.frag \
    shaders/h_shadowMapGeneration.vert

# trimesh library for loading objects.
# Reference/source: http://gfx.cs.princeton.edu/proj/trimesh2/
INCLUDEPATH += ../trimesh2/include/

LIBS += -L../../Project1/trimesh2/lib -ltrimesh

DISTFILES += \
    shaders/textured_ESM.frag \
    shaders/textured_PCF.frag \
    shaders/textured_PCSS.frag \
    shaders/textured_VSM.frag \
    shaders/textured_ESM.vert \
    shaders/textured_PCF.vert \
    shaders/textured_PCSS.vert \
    shaders/textured_VSM.vert
