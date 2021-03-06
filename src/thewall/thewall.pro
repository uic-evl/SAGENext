# -------------------------------------------------
# Project created by QtCreator 2010-04-08T11:36:20
# -------------------------------------------------
QT += network opengl declarative
TARGET = sagenext
TEMPLATE = app


QTWEBKIT = $$(QTWEBKIT_DIR)
isEmpty(QTWEBKIT) {
    QT += webkit
}
else {
#
# QtWebKit is built separately from WebKit source code using Tools/Scripts/build-webkit --qt --3d-canvas --3d-rendering --accelerated-2d-canvas
#
    message("Using a custom QtWebKit library: $$(QTWEBKIT)")
    QT -= webkit

    message("$$(QTWEBKIT_DIR)/include/QtWebKit")
    INCLUDEPATH += $$(QTWEBKIT_DIR)/include/QtWebKit
    message("-L$$(QTWEBKIT_DIR)/lib -lQtWebKit")
    LIBS += -L$$(QTWEBKIT_DIR)/lib -lQtWebKit
}


#
# NUMA lib
#
linux-g++|linux-g++-64 {
    message("Using Linux libnuma library")
    LIBS += -lnuma
}


# If unix (linux, mac)
# unix includes linux-g++  linux-g++-64    macx  macx-g++  symbian ...
unix {
    INCLUDEPATH += /usr/include

#
# Use pkg-config
#
    CONFIG += link_pkgconfig

#
# LibVNCServer
# install the package in trusted library directory such as /usr/local
# or
# Add LIBVNCSERVER_INSTALL_PATH/lib/pkgconfig in your PKG_CONFIG_PATH environment variable
#
# If you're to compile libvncserver, then don't forget to include GCrypt support
# ./configure --with-gcrypt
    packagesExist(libvncclient) {
        message("Linking LibVNCServer lib")
    	PKGCONFIG += libvncclient
    }
    else {
        error("Package LibVNCServer doesn't exist !")
        LIBVNCSERVER_LIBS = $$system(libvncserver-config --libs)
        isEmpty(LIBVNCSERVER_LIBS) {
            error("Missing Package : LibVCNServer is required")
        }
        else {
            message("libvncserver-config --libs =>" $$LIBVNCSERVER_LIBS)
            LIBS += $${LIBVNCSERVER_LIBS}
        }
    }



#
# Poppler-qt4 for PDFviewer
#
# Add QtSDK/Desktop/Qt/474/gcc/lib/pkgconfig in PKG_CONFIG_PATH environment variable
# configure poppler with --enable-poppler-qt4  (you might need to compile/install openjpeg)
# Add POPPLER_INSTALL_PATH/lib/pkgconfig in PKG_CONFIG_PATH
#
    packagesExist(poppler-qt4) {
        message("Linking poppler-qt4 lib")
    	PKGCONFIG += poppler-qt4
    }
    else {
        error("Missing Package : poppler-qt4 is required")
    }
} # end of unix{}




#
# Qwt
# Use $$(..) to obtain contents of an environment variable
#
QWT_HOME = $$(HOME)/Dev/qwt-6.0.1
exists( $$QWT_HOME/lib/libqwt.so ) {
    message("Package Qwt is available")
    INCLUDEPATH += $${QWT_HOME}/include
	LIBS += -L$${QWT_HOME}/lib -lqwt
    DEFINES += USE_QWT
}
else {
    warning("Missing Package : Qwt is not available, but continue")
}





BUILD_DIR = _BUILD
!exists($$BUILD_DIR) {
    #message("Creating build directory")
    system(mkdir $${BUILD_DIR})
}

MOC_DIR = $$BUILD_DIR/_MOC
OBJECTS_DIR = $$BUILD_DIR/_OBJ
UI_DIR = $$BUILD_DIR/_UI



# Use parenthesis to read from environment variable
SAGENEXT_USER_DIR = $$(HOME)/.sagenext
MEDIA_ROOT_DIR = $$SAGENEXT_USER_DIR/media
message("Your media root directory : $$MEDIA_ROOT_DIR")

IMAGE_DIR = $$MEDIA_ROOT_DIR/image
VIDEO_DIR = $$MEDIA_ROOT_DIR/video
PDF_DIR = $$MEDIA_ROOT_DIR/pdf
PLUGIN_DIR = $$MEDIA_ROOT_DIR/plugins

!exists($$MEDIA_ROOT_DIR) {
    message("Creating media directories")
    system(mkdir -p $$MEDIA_ROOT_DIR)
    system(mkdir $$IMAGE_DIR)
    system(mkdir $$VIDEO_DIR)
    system(mkdir $$PLUGIN_DIR)
}


!exists($$IMAGE_DIR) {
    system(mkdir $$IMAGE_DIR)
}
!exists($$VIDEO_DIR) {
    system(mkdir $$VIDEO_DIR)
}
!exists($$PDF_DIR) {
	system(mkdir $$PDF_DIR)
}
!exists($$PLUGIN_DIR) {
    system(mkdir $$PLUGIN_DIR)
}

SESSIONS_DIR = $$SAGENEXT_USER_DIR/sessions
!exists($$SESSIONS_DIR) {
    system(mkdir $$SESSIONS_DIR)
}

THUMBNAIL_DIR = $$SAGENEXT_USER_DIR/.thumbnails
message("Thumbnail dir is $${THUMBNAIL_DIR}")
!exists($$THUMBNAIL_DIR) {
    system(mkdir $$THUMBNAIL_DIR)
}




# where to put TARGET file
#DESTDIR += ../../
#CONFIG(debug, debug|release):DESTDIR += debug
#CONFIG(release, debug|release):DESTDIR += release

FORMS += \
    applications/base/sn_affinitycontroldialog.ui \
    system/sn_resourcemonitorwidget.ui \
    settings/sn_settingstackeddialog.ui \
    settings/sn_generalsettingdialog.ui \
    settings/sn_systemsettingdialog.ui \
    settings/sn_graphicssettingdialog.ui \
    settings/sn_guisettingdialog.ui \
    settings/sn_screenlayoutdialog.ui


RESOURCES += ../resources.qrc


SOURCES += \
main.cpp \
sn_mediastorage.cpp \
sn_scene.cpp \
sn_view.cpp \
sn_sagenextlauncher.cpp \
settings/sn_settingstackeddialog.cpp \
#
common/sn_commonitem.cpp \
common/sn_doublebuffer.cpp \
common/sn_layoutwidget.cpp \
common/sn_sharedpointer.cpp \
#
uiserver/sn_uiserver.cpp \
uiserver/sn_uimsgthread.cpp \
uiserver/sn_fileserver.cpp \
#
system/sn_resourcemonitor.cpp \
system/sn_scheduler.cpp \
system/sn_resourcemonitorwidget.cpp \
system/sn_prioritygrid.cpp \
#
sage/fsManager.cpp \
sage/fsmanagermsgthread.cpp \
sage/sageLegacy.cpp \
#
applications/base/sn_perfmonitor.cpp \
applications/base/sn_appinfo.cpp \
applications/base/sn_basewidget.cpp \
applications/base/sn_railawarewidget.cpp \
applications/base/sn_sagestreamwidget.cpp \
applications/base/sn_sagepixelreceiver.cpp \
applications/base/sn_affinityinfo.cpp \
applications/base/sn_affinitycontroldialog.cpp \
applications/base/sn_priority.cpp \
#
applications/sn_checker.cpp \
applications/sn_sagestreammplayer.cpp \
applications/sn_fittslawtest.cpp \
applications/sn_webwidget.cpp \
applications/sn_pixmapwidget.cpp \
applications/sn_mediabrowser.cpp \
applications/sn_vncwidget.cpp \
applications/sn_pdfvieweropenglwidget.cpp


HEADERS += \
sn_scene.h \
sn_view.h \
sn_sagenextlauncher.h \
sn_mediastorage.h \
settings/sn_settingstackeddialog.h \
#
common/sn_commonitem.h \
common/sn_commondefinitions.h \
common/sn_doublebuffer.h \
common/sn_layoutwidget.h \
common/sn_sharedpointer.h \
#
uiserver/sn_uiserver.h \
uiserver/sn_uimsgthread.h \
uiserver/sn_fileserver.h \
#
system/sn_resourcemonitor.h \
system/sn_resourcemonitorwidget.h \
system/sn_scheduler.h \
system/sn_prioritygrid.h \
#
sage/fsManager.h \
sage/fsmanagermsgthread.h \
sage/sagecommondefinitions.h \
#
applications/base/sn_appinfo.h \
applications/base/sn_perfmonitor.h \
applications/base/sn_affinityinfo.h \
applications/base/sn_affinitycontroldialog.h \
applications/base/sn_basewidget.h \
applications/base/sn_railawarewidget.h \
applications/base/sn_sagestreamwidget.h \
applications/base/sn_sagepixelreceiver.h \
applications/base/sn_priority.h \
#
applications/sn_webwidget.h \
applications/sn_pixmapwidget.h \
applications/sn_vncwidget.h \
applications/sn_mediabrowser.h \
applications/sn_checker.h \
applications/sn_sagestreammplayer.h \
applications/sn_fittslawtest.h \
applications/sn_pdfvieweropenglwidget.h

