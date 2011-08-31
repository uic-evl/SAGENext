# -------------------------------------------------
# Project created by QtCreator 2010-04-08T11:36:20
# -------------------------------------------------
QT += network webkit opengl
TARGET = sagenext
TEMPLATE = app




#LibVNCServer
linux-g++ {
	LIBVNCSERVER = /home/evl/snam5/LibVNCServer-0.9.7
	message("Linking with LibVNC lib")
	INCLUDEPATH += $$LIBVNCSERVER/include
	LIBS += -L$$LIBVNCSERVER/lib -lvncclient
}
macx {
	LIBVNCSERVER = ${HOME}/Downloads/LibVNCServer
	message("Linking with LibVNC lib $$LIBVNCSERVER")
	INCLUDEPATH += $$LIBVNCSERVER/include
	LIBS += -L$$LIBVNCSERVER/lib -lvncclient
}


#Qwt
#QWT_HOME = /home/evl/snam5/Downloads/qwt-5.2
#INCLUDEPATH += $${QWT_HOME}/src
#LIBS += -L$${QWT_HOME}/lib -lqwt

linux-g++ {
	message("Linking with Linux libnuma library")
	LIBS += -L/home/evl/snam5/Downloads/numactl-2.0.6 -lnuma
}
macx {
	message("Excluding -lnuma -lm -lpthread")
	LIBS -= -lnuma -lm -lpthread
}

#CONFIG += thread
#CONFIG += copy_dir_files

BUILD_DIR = BUILD
!exists($$BUILD_DIR) {
	#message("Creating build directory")
	system(mkdir $${BUILD_DIR})
}

MOC_DIR = MOC
OBJECTS_DIR = $${BUILD_DIR}

# where to put TARGET file
DESTDIR = ../../
#CONFIG(debug, debug|release):DESTDIR += debug
#CONFIG(release, debug|release):DESTDIR += release

FORMS += \
	settingdialog.ui \
	applications/base/affinitycontroldialog.ui \
	system/resourcemonitorwidget.ui


RESOURCES += ../resources.qrc


SOURCES += \
	main.cpp \
	settingdialog.cpp \
#	graphicsviewmainwindow.cpp \
	common/commonitem.cpp \
	common/thumbnailthread.cpp \
	common/imagedoublebuffer.cpp \
	uiserver/uiserver.cpp \
	uiserver/uimsgthread.cpp \
	uiserver/launcher.cpp \
	system/resourcemonitor.cpp \
	system/sagenextscheduler.cpp \
	system/resourcemonitorwidget.cpp \
	sage/fsManager.cpp \
	sage/fsmanagermsgthread.cpp \
	sage/sageLegacy.cpp \
	applications/webwidget.cpp \
	applications/pixmapwidget.cpp \
	applications/sagestreamwidget.cpp \
	applications/sagepixelreceiver.cpp \
	applications/vncwidget.cpp \
	applications/base/perfmonitor.cpp \
	applications/base/appinfo.cpp \
	applications/base/basewidget.cpp \
	applications/base/railawarewidget.cpp \
	applications/base/affinityinfo.cpp \
	applications/base/affinitycontroldialog.cpp \
	sagenextscene.cpp \
	sagenextviewport.cpp \
	sagenextlauncher.cpp

HEADERS += \
	settingdialog.h \
#	graphicsviewmainwindow.h \
	common/commonitem.h \
	common/commondefinitions.h \
	common/imagedoublebuffer.h \
	common/thumbnailthread.h \
	uiserver/uiserver.h \
	uiserver/uimsgthread.h \
	uiserver/launcher.h \
	system/resourcemonitor.h \
	system/resourcemonitorwidget.h \
	system/sagenextscheduler.h \
	sage/fsManager.h \
	sage/fsmanagermsgthread.h \
	sage/sagecommondefinitions.h \
	applications/webwidget.h \
	applications/pixmapwidget.h \
	applications/sagestreamwidget.h \
	applications/sagepixelreceiver.h \
	applications/vncwidget.h \
	applications/base/appinfo.h \
	applications/base/perfmonitor.h \
	applications/base/affinityinfo.h \
	applications/base/affinitycontroldialog.h \
	applications/base/basewidget.h \
	applications/base/railawarewidget.h \
	sagenextscene.h \
	sagenextviewport.h \
	sagenextlauncher.h