TEMPLATE = app
CONFIG += console c++1z link_pkgconfig
CONFIG -= app_bundle
CONFIG -= qt

PKGCONFIG += sfml-graphics

QMAKE_CXXFLAGS_RELEASE = -march=native -Ofast

SOURCES += main.cpp

DISTFILES += \
    update.vert \
    update.frag \
    render.frag \
    render_45.frag \
    update_45.frag \
    update_45.vert \
    spawn.frag
