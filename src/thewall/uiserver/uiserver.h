#ifndef UISERVER_H
#define UISERVER_H


#include "uimsgthread.h"
//#include "graphicsviewmainwindow.h"
#include "common/commondefinitions.h"
#include "common/commonitem.h"

#include <QTcpServer>


#define EXTUI_MSG_SIZE 1280

class SAGENextScene;
class SAGENextLauncher;
class QSettings;


/* transfer file / stream pixel / stream file */
enum EXTUI_TRANSFER_MODE { FILE_TRANSFER, FILE_STREAM, PIXEL_STREAM };


/* types of messages between external UI and the wall */
enum EXTUI_MSG_TYPE { MSG_NULL, REG_FROM_UI, ACK_FROM_WALL, DISCONNECT_FROM_WALL, WALL_IS_CLOSING, TOGGLE_APP_LAYOUT, RESPOND_APP_LAYOUT, VNC_SHARING, POINTER_PRESS, POINTER_RIGHTPRESS, POINTER_RELEASE, POINTER_RIGHTRELEASE, POINTER_CLICK, POINTER_RIGHTCLICK, POINTER_DOUBLECLICK, POINTER_DRAGGING, POINTER_MOVING, POINTER_SHARE, POINTER_WHEEL, POINTER_UNSHARE};

class UiServer : public QTcpServer
{
	Q_OBJECT
public:
	explicit UiServer(const QSettings *s, SAGENextLauncher *snl, SAGENextScene *sns);
	~UiServer();

	/**
	  * This function is called by the main process (GraphicsViewMain::TimerEvent()) every 1sec (TimerEvent of the main process)
	  * It updates this->appLayout and *BROADCAST* this buffer to ALL ui clients
	  */
	void updateAppLayout();

protected:
	/**
	  * overrides QTcpServer::incomingConnection()
	  * The base implementation emits newConnection() signal
	  *
	  * creates a ui msg thread for each connection
	  * insert this connection to clientMap
	  */
	void incomingConnection(int sockfd);

private:
	//QHostAddress uiServerIP;
	const QSettings *settings;

	/*!
	  * Each ui client has to have unique identifier
	  */
	quint64 uiClientId;

	/*!
	  * uiclientid and corresponding msg thread
	  */
	QMap<quint64, UiMsgThread *> uiThreadsMap;

	/*!
	  * uiclientid and corresponding shared pointer
	  */
	QMap<quint64, PolygonArrow *> pointers;

	/*!
	  uiclientid and corresponding app layout flag
	  appLayout is sent to those clients who set the flag
	  */
	QMap<quint64, bool> appLayoutFlagMap;


//	QGraphicsScene *scene;

	/**
	  * This buffer is updated periodically by this->updateAppLayout() and broadcasted to ALL ext UI clients (RESPOND_APP_LAYOUT)
	  */
	QByteArray *appLayout;

	SAGENextScene *scene;

	SAGENextLauncher *launcher;

	UiMsgThread * getUiMsgThread(quint64 uiclientid);
	PolygonArrow * arrow(quint64 uiclientid);



signals:
	/**
	  * REG_FROM_UI handler.
	  * will invoke GraphicsViewMain::startApp()
	  */
	void registerApp(MEDIA_TYPE type, QString filename, qint64 filesize, QString senderIP, QString recvIP, quint16 recvPort);

private slots:
	void handleMessage(const quint64 id, UiMsgThread *, const QByteArray msg);

	/**
	  * invoked by the signal UiMsgThread::clientDisconnected()
	  * update uiThreadsMap and remove PolygonArrow from the scene
	  */
	void removeFinishedThread(quint64);
};

#endif // UISERVER_H