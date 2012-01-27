#ifndef SAGENEXTLAUNCHER_H
#define SAGENEXTLAUNCHER_H

#include <QtCore>
#include <QtGui>

#include "common/commondefinitions.h"

class QSettings;
class SN_TheScene;

class QGraphicsItem;
class SN_BaseWidget;
class SN_PolygonArrowPointer;
class SN_PluginInterface;

class fsManager;
class fsManagerMsgThread;
class SN_SageStreamWidget;

class SN_ResourceMonitor;
class SN_SchedulerControl;
class SN_MediaStorage;

class SN_Launcher : public QObject
{
	Q_OBJECT
public:
	explicit SN_Launcher(const QSettings *s, SN_TheScene *scene, SN_MediaStorage *mediaStorage, SN_ResourceMonitor *rm = 0, SN_SchedulerControl *sc = 0, QFile *scenarioFile = 0, QObject *parent = 0);
	~SN_Launcher();

	/**
	  This is temporary for scenario stuff
	  Use UiServer's container instead
	  */
	QMap<quint64, SN_PolygonArrowPointer *> _pointerMap;

private:
	const QSettings *_settings;

        /*!
          * global
          */
	quint64 _globalAppId;

        /**
          The pointer to the scene
          */
	SN_TheScene *_scene;

        /**
          The pointer to the media storage
          */
	SN_MediaStorage *_mediaStorage;

        /**
          fsServer::checkClient()
          */
	fsManager *_fsm;

        /**
          Launcher creates sageWidget before firing SAIL application. So the sageWidget is waiting for actual SAIL connection to run.
          This is the queue for those sageWidget waiting SAIL connection. This queue is accessed only in this class.

          Be CAREFUL !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
          Let's say sageWidget_1 and sageWidget_2 are waiting in the queue.
          The SAIL_1 , SAIL_2 have been fired as well. But SAIL_2's connection got to the fsManager before SAIL_1
          Then SAIL_2 will be blindly assigned to sageWidget_1.

          This is because there's no way to co-relate sageWidget to SAIL connection...
          !!!!!!!!!!!!!!!!!!!!!!!!!!!!
          */
	QList<SN_SageStreamWidget *> _sageWidgetQueue;

	QList<QPointF> _sageWidgetPosQueue;

		/**
		  This is called once in the Constructor. It starts fsManager (QTcpServer)
		  */
	void createFsManager();

	SN_ResourceMonitor *_rMonitor;

	SN_SchedulerControl *_schedCtrl;

		/**
		  To record new widget/pointer starts.
		  This will be passed to polygonArrows
		  */
	QFile *_scenarioFile;

	QMap<QString, SN_PluginInterface *> _pluginMap;

		/**
		  This is called once in the constructor.
		  To launch an actual instance, call SN_PluginInterface::createInstance()
		  */
	void loadPlugins();


public slots:
        /**
          This slot is invoked by the signal fsManager::incomingSail() in fsManager::incomingConnection
          */
	SN_BaseWidget * launch(fsManagerMsgThread *);

        /**
          this is general launch function
          */
	SN_BaseWidget * launch(int mediatype, QString filename, const QPointF &scenepos = QPointF(30,30), qint64 filesize=0, QString senderIP="127.0.0.1", QString recvIP="", quint16 recvPort=0);

        /**
          just for VNC widget
          */
	SN_BaseWidget * launch(QString username, QString vncPasswd, int display, QString vncServerIP, int framerate = 10, const QPointF &scenepos = QPointF(30,30));

        /**
          The widget is added to the scene in here.
          _globalAppId is incremented by 1 in here
          */
	SN_BaseWidget * launch(SN_BaseWidget *, const QPointF &scenepos = QPointF(30,30));

	SN_BaseWidget * launch(void *vbw, const QPointF &scenepos = QPointF(30,30)) {
		return launch(static_cast<SN_BaseWidget *>(vbw), scenepos);
	}

		/**
		  Only with filename, this slot launches all sorts of things (media, session, recording,..)
		  */
	SN_BaseWidget * launch(const QStringList &fileList);


	SN_PolygonArrowPointer * launchPointer(quint32 uiclientid, const QString &name, const QColor &color, const QPointF &scenepos = QPointF());

		/**
		  Load a saved session
		  */
	void launchSavedSession(const QString &sessionfilename);

		/**
		  This slot should be running in a separate thread
		  */
	void launchRecording(const QString &recordingFilename);


		/*!
		  This must run in a separate thread
		  */
	void launchRatkoUserStudyData(const QString &datafile, const QString &srcaddr="", const QString &mediafile="");

	void runRatkoSlot() {
		QtConcurrent::run(this, &SN_Launcher::launchRatkoUserStudyData, QString("/home/evl/snam5/.sagenext/group1.log"), QString(), QString());
//			launchRatkoUserStudyData(QString("/home/evl/snam5/.sagenext/group1.log"), QString(), QString());
	}
};














class ScenarioThread : public QThread {
	Q_OBJECT
public:
	ScenarioThread(SN_Launcher *launcher, const QString &file, QObject *parent = 0);
	~ScenarioThread();

protected:
	void run();

private:
	SN_Launcher *_launcher;

	QFile _scenarioFile;

};

#endif // SAGENEXTLAUNCHER_H
