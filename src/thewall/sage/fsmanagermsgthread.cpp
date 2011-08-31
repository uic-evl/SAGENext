#include "fsmanagermsgthread.h"
//#include "sage/fsManager.h"
//#include "sage/sageLegacy.h"

#include "../applications/sagestreamwidget.h"
#include "../applications/base/affinityinfo.h"

#include <QStringList>
#include <QSettings>
#include <QtCore>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> // IPROTO_TCP
#include <netinet/tcp.h> // TCP_NODELY


fsManagerMsgThread::fsManagerMsgThread(const quint64 sageappId, int sockfd, const QSettings *settings, QObject *parent) :
		QThread(parent),
		settings(settings),
	_sageWidget(0),
		sageAppId(sageappId)
{
	this->_end = false;
	this->socket = sockfd;
//	this->fsmParam = fsmParam;

	int optval = 1;
	//socket.setSocketOption(QAbstractSocket::LowDelayOption, (const QVariant *)&optval);
	if ( setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, (socklen_t)sizeof(optval)) != 0 ) {
		qWarning("%s() : setsockopt SO_REUSEADDR failed", __FUNCTION__);
	}
	if ( setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, (socklen_t)sizeof(optval)) != 0 ) {
		qWarning("%s() : setsockopt SO_KEEPALIVE failed", __FUNCTION__);
	}
	if ( setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &optval, (socklen_t)sizeof(optval)) != 0 ) {
		qWarning("%s() : setsockopt TCP_NODELAY failed", __FUNCTION__);
	}
	//qDebug("%s() : socket error %d", __FUNCTION__, socket.error());
}

fsManagerMsgThread::~fsManagerMsgThread() {
//	sendSailShutdownMsg(sageAppId);

	if ( ::shutdown(socket, SHUT_RDWR) != 0 ) {
		qDebug("fsManagerMsgThread::%s() : ::shutdown error", __FUNCTION__);
	}
	if ( ::close(socket) == -1 ) {
		qDebug("fsManagerMsgThread::%s() : error closing socket", __FUNCTION__);
	}

	// this is important, without this, program will finishes with an error
	this->wait();
	qDebug("fsManagerMsgThread::%s() : returning",  __FUNCTION__);
}

void fsManagerMsgThread::breakWhileLoop() {
	_end = true;
}

void fsManagerMsgThread::sendSailShutdownMsg() {
	sendSailShutdownMsg(sageAppId);
}

void fsManagerMsgThread::sendSailShutdownMsg(quint64 sageappid) {
	if ( sageAppId != sageappid ) return;

	OldSage::sageMessage msg;
	msg.init(36); // MESSAGE_HEADER_SIZE (4 * 9Byte)
	msg.setDest(sageappid);
	msg.setCode(OldSage::APP_QUIT); // send APP_QUIT to sail. sail will send SAIL_SHUTDOWN
	if ( ::send(socket, (char *)msg.getBuffer(), msg.getBufSize(), 0) <= 0 ) {
			qDebug("fsManagerMsgThread::%s() : failed to send APP_QUIT to sageappid %llu", __FUNCTION__, sageappid);
	}
	else {
//		qDebug("fsManagerMsgThread::%s() : APP_QUIT sent for sageappid %llu", __FUNCTION__, sageappid);
	}
	msg.destroy();
	_end = true;
}

void fsManagerMsgThread::sendSailSetRailMsg(AffinityInfo *aff, quint64 sageappid) {
	if (!aff) return;
	if ( sageAppId != sageappid) return;

	QString data = "";
	data.append(QString::number( aff->getNumCpus()) );
	data.append(" ");
	data.append(aff->getStreamerCpuMask());

	OldSage::sageMessage msg;
	msg.init(sageappid, OldSage::SAIL_SET_RAIL, 0, data.length(), qPrintable(data));

	// MESSAGE_HEADER_SIZE 36 Byte
	// d_dddd...d (numcpu space cpumask)
//	msg.init( 36 + sizeof(char) * data.length() );
//	msg.setDest(sageappid);
//	msg.setCode(SAIL_SET_RAIL); // send APP_QUIT to sail. sail will send SAIL_SHUTDOWN
//	msg.setData(data.length(), (void *)data.data());

	if ( ::send(socket, (char *)msg.getBuffer(), msg.getBufSize(), 0) <= 0 ) {
			qDebug() << "fsmMsgThread: error sending SAIL_SET_RAIL message";
	}
	else {
//		qDebug("fsManagerMsgThread::%s() : SAIL_SET_RAIL [%s] has sent", __FUNCTION__, (char *)msg.getData());
	}
	msg.destroy();
}

void fsManagerMsgThread::sendSailMsg(OldSage::sageMessage &msg) {

	/*
	SAGE::sageMessage msg;
	msg.init(64);
	msg.setDest(sageappid);
	msg.setCode(SAIL_SET_RAIL); // send APP_QUIT to sail. sail will send SAIL_SHUTDOWN
	*/
//	msg.setDest(sageappid);
	send(socket, (char *)msg.getBuffer(), msg.getBufSize(), 0);
//	msg.destroy();
}

void fsManagerMsgThread::run() {
//	qDebug("fsManagerMsgThread::%s() : msgThread starting. sageAppId %llu, sockfd %d",  __FUNCTION__, sageAppId, socket);

	OldSage::sageMessage sageMsg;
	QByteArray msgStr(OldSage::MESSAGE_FIELD_SIZE, '\0');

	/*
	while (!_end) {
		qDebug("%s() : socket error %d", __FUNCTION__, socket.error());
		if ( socket.error() == QAbstractSocket::RemoteHostClosedError ) {
			// socket is closed automatically
			qDebug("%s() : remoteHostClosedError", __FUNCTION__);
			break;
		}
		if ( !socket.isValid() ) {
			qDebug("%s() : socket is not valid", __FUNCTION__);
			continue;
		}

		if ( socket.isOpen() && socket.waitForReadyRead(10) ) {
			qDebug("%s() : thread appID %llu, data available", __FUNCTION__, appID);

			 // read the first 9 byte (size)
			if ( socket.read(msgStr.data(), MESSAGE_FIELD_SIZE) == -1 )
				break;

			int dataSize = msgStr.toInt();
			sageMsg.init(dataSize);
			dataSize = dataSize - MESSAGE_FIELD_SIZE; // skip first MESSAGE_FIELD_SIZE byte (which is *size)

			if ( socket.read((char *)sageMsg.getBuffer()+MESSAGE_FIELD_SIZE, dataSize) == -1 ) // nonblock
				break;

			qDebug("size %d, dest %d, code %d, appCode %d, data [%s]", sageMsg.getSize(), sageMsg.getDest(), sageMsg.getCode(), sageMsg.getAppCode(), (char *)sageMsg.getData());

			**
			if (msg.getCode() < DISP_MESSAGE) {
				fprintf(stderr,"fsServer::%s() : msg code %d < %d, msgToCore\n", __FUNCTION__, msg.getCode(), DISP_MESSAGE);
				fsm->msgToCore(msg, i+SYSTEM_CLIENT_BASE);
			}
			else if (msg.getCode() < GRCV_MESSAGE) {
				fprintf(stderr,"fsServer::%s() : msg code %d < %d, msgToDisp\n", __FUNCTION__, msg.getCode(), GRCV_MESSAGE);
				fsm->msgToDisp(msg, i+SYSTEM_CLIENT_BASE);
			}
			else {
				fprintf(stderr,"fsServer::%s() : msg code %d. invoke sendMessage()\n", __FUNCTION__, msg.getCode());
				sendMessage(msg);
			}
			**

			// parse message
			this->parseMessage(sageMsg);
		}
		else {
			//qDebug("sibal");
		}
	}
	*/

	int read=0;
	while(!_end) {
		if ( (read = recv(socket, (void *)msgStr.data(), OldSage::MESSAGE_FIELD_SIZE, MSG_WAITALL)) == -1 ) {
			qCritical("fsManagerMsgThread::%s() : socket error", __FUNCTION__);
			_end=true;
		}
		else if ( read == 0 ) {
			qDebug("fsManagerMsgThread::%s() : socket disconnected while receiving 9Byte", __FUNCTION__);
			_end=true;
		}
		if (_end) break;

//		qDebug("fsManagerMsgThread::%s() : received msg field [%s]", __FUNCTION__, msgStr.data());

		sageMsg.init(atoi(msgStr.data()));
		int datasize = atoi(msgStr.data());
		datasize = datasize - OldSage::MESSAGE_FIELD_SIZE;

		if ( (read = recv(socket, (char *)sageMsg.getBuffer() + OldSage::MESSAGE_FIELD_SIZE, datasize, MSG_WAITALL)) == -1 ) {
			qCritical("fsManagerMsgThread::%s() : socket error", __FUNCTION__);
			_end=true;
		}
		else if ( read == 0 ) {
			qDebug("fsManagerMsgThread::%s() : socket disconnected", __FUNCTION__);
			_end=true;
		}
		if (_end) break;

//		qDebug("fsManagerMsgThread::%s() : received sageMessage, size %d, dest %d, code %d, appCode %d", __FUNCTION__, sageMsg.getSize(), sageMsg.getDest(), sageMsg.getCode(), sageMsg.getAppCode());

		parseMessage(sageMsg);
		sageMsg.destroy();
	}
	qDebug("fsManagerMsgThread::%s() : msgThread for sageAppId %llu exit the loop", __FUNCTION__, sageAppId);
}

void fsManagerMsgThread::parseMessage(OldSage::sageMessage &sageMsg) {

	QString msgStr((char *)sageMsg.getData());

	switch(sageMsg.getCode()) {
	case OldSage::REG_APP : {
			qDebug("fsManagerMsgThread::%s() : REG_APP : msgData [%s] \n", __FUNCTION__, qPrintable(msgStr));
			char appn[128];
			char temp[512];
			int x, y;
			int width, height;
			int dummy;
			int protocol;
			sscanf((char *)sageMsg.getData(), "%s %d %d %d %d %d %s %d %d %d %d",
				   appn, &x, &y, &dummy, &dummy, &dummy, temp, &width, &height, &dummy, &protocol);

			/*
			sscanf((char *)msg.getData(), "%s %d %d %d %d %d %s %d %d %d %d %d %d %s %d",
				   app->appName,
				   &app->x,
				   &app->y,
				   &app->width,
				   &app->height,
				   &app->bandWidth,
				   app->renderNodeIP,
				   &app->imageWidth,
				   &app->imageHeight,
				   (int *)&app->audioOn,
				   (int *)&app->protocol,
				   &app->frameRate,
				   &app->instID,
				   app->launcherID,
				   &app->portForwarding);
				   */





			/** send SAIL_INIT_MSG to sender. It will trigger... **/

			//sageNwConfig nwCfg;
			// reading network parameters set in fsManager.conf
			//sscanf(msgData, "%d %d %d %d", &winID, &nwCfg.rcvBufSize, &nwCfg.sendBufSize, &nwCfg.mtuSize);
			//fprintf(stderr,"sail::%s() : SAIL_INIT_MSG(code %d) : received [%s]\n", __FUNCTION__, msg.getCode(), msgData);

			//if (config.rendering) {
			//pixelStreamer->setWinID(winID);
			//pixelStreamer->setNwConfig(nwCfg); // block partition and block group objects are created here

			/**
			  send SAIL_INIT_MSG
			 **/
			OldSage::sageMessage sageMsg;
			QByteArray initMsg(32, '\0');
			sprintf(initMsg.data(), "%d %d %d %d",
					(int)sageAppId,
					settings->value("network/recvwindow", 16777216).toInt(),
					settings->value("network/sendwindow", 1048576).toInt(),
					settings->value("network/mtu", 1450).toInt()); // snd/rcv network socket size followed by MTU

			if (sageMsg.init(sageAppId, OldSage::SAIL_INIT_MSG, 0, initMsg.size(), initMsg.data()) < 0) {
				qCritical("fsManagerMsgThread::%s() : failed to init sageMessage", __FUNCTION__);
				_end=true;
				break;
			}

			int dataSize = sageMsg.getBufSize();
			int sent = send(socket, (char *)sageMsg.getBuffer(), dataSize, 0);
			if ( sent == -1 ) {
				qCritical("fsManagerMsgThread::%s() : error while sending SAIL_INIT_MSG", __FUNCTION__);
				_end=true;
			}
			if ( sent == 0 ) {
				// remote side disconnected
				qCritical("fsManagerMsgThread::%s() : while sending SAIL_INIT_MSG, sail disconnected.", __FUNCTION__);
				_end = true;
			}
			sageMsg.destroy();
			if(_end) return;


			/**
			 starts SageStreamWidget
			**/
			QString appName(appn);
			int streamPort = settings->value("general/fsmstreambaseport", 20005).toInt() + sageAppId;
			if(streamPort > 65535) {
				qCritical("\nfsManagerMsgThread::%s() : streamPort  has set to %d\n", __FUNCTION__, streamPort);
				return;
			}

			// fsManager receives this signal and emit forwardSailConnection signal to MainWindow, then MainWindow invoke startSageApp()
			QRect initRect(x, y, width, height);

			Q_ASSERT(_sageWidget);
			QMetaObject::invokeMethod(_sageWidget, "initialize", Qt::QueuedConnection,
									  Q_ARG(quint64, sageAppId),
									  Q_ARG(QString, appName),
									  Q_ARG(QRect, initRect),
									  Q_ARG(int, protocol),
									  Q_ARG(int, streamPort));



			/*

			emit sailConnected(sageAppId, appName,  protocol, streamPort, initRect);

			*/

			// wait a bit so that SageStreamWidget / SagePixelReceiver can be started
			/* If this is too short, sender could connect() before accept() is called at the sageStreamWidget */
//			QCoreApplication::sendPostedEvents();
//			QThread::yieldCurrentThread();
			QThread::msleep(300);



			/** send SAIL_CONNECT_TO_RCV to sender **/
			//pixelStreamer->initNetworks(msgData)
			// sail will send back SAIL_CONNECTED_TO_RCV
			// [22000 1 131.193.78.140 0 ]
			initMsg.fill('\0');
			sprintf(initMsg.data(), "%d %d %s %d",
					streamPort, // streamer port
					1, // number of SDM
//					qobject_cast<QTcpServer *>(parent())->serverAddress().toString().toAscii().constData(),
					qPrintable(settings->value("general/fsmip").toString()), // ip addr of receiver (which can be different from fsManager IP)
					0); // SDM id

			if (sageMsg.init(sageAppId, OldSage::SAIL_CONNECT_TO_RCV, 0, initMsg.size(), initMsg.constData()) < 0) {
				qCritical("fsManagerMsgThread::%s() : failed to init sageMessage", __FUNCTION__);
				_end=true;
				break;
			}
			dataSize = sageMsg.getBufSize();
			sent = send(socket, (char *)sageMsg.getBuffer(), dataSize, 0);
			if ( sent == -1 ) {
				qCritical("fsManagerMsgThread::%s() : error while sending SAIL_CONNECT_TO_RCV", __FUNCTION__);
				_end=true;
			}
			if ( sent == 0 ) {
				// remote side disconnected
				qCritical("fsManagerMsgThread::%s() : while sending SAIL_CONNECT_TO_RCV, sail disconnected.", __FUNCTION__);
				_end = true;
			}
			sageMsg.destroy();

			break;
		}
	case OldSage::SAIL_SHUTDOWN : {
			_end = true;
			break;
		}
	}

	/**
	switch(msg.getCode()) {

	case REG_APP : {
			qDebug("%s() : REG_APP(%d) : msgData [%s] \n", __FUNCTION__, REG_APP, (char *)msg.getData());

			sscanf((char *)msg.getData(), "%s %d %d %d %d %d %s %d %d %d %d %d %d %s %d",
				   app->appName,
				   &app->x,
				   &app->y,
				   &app->width,
				   &app->height,
				   &app->bandWidth,
				   app->renderNodeIP,
				   &app->imageWidth,
				   &app->imageHeight,
				   (int *)&app->audioOn,
				   (int *)&app->protocol,
				   &app->frameRate,
				   &app->instID,
				   app->launcherID,
				   &app->portForwarding);

			// adjust app window size considering image resolution
			float ar = (float)(app->imageWidth) / (float)(app->imageHeight);
			if ((app->imageWidth > MAX_SAGE_WINDOW_SIZE && app->width < MAX_SAGE_WINDOW_SIZE) ||
				(app->imageHeight > MAX_SAGE_WINDOW_SIZE && app->height < MAX_SAGE_WINDOW_SIZE)) {
				if (ar > 1) {
					app->width = MAX_SAGE_WINDOW_SIZE;
					app->height = (int)(MAX_SAGE_WINDOW_SIZE/ar);
				}
				else {
					app->height = MAX_SAGE_WINDOW_SIZE;
					app->width = (int)(MAX_SAGE_WINDOW_SIZE*ar);
				}
			}

			app->sailClient = clientID; // i+SYSTEM_CLIENT_BASE in fsServer::checkClientShit()

			char sailInitMsg[TOKEN_LEN];
			memset(sailInitMsg, 0, TOKEN_LEN);

			// RE-GENERATING ID (2/2)
			//fsm->m_execIndex = getAvailableInstID();
			//fsm->m_execIDList[fsm->m_execIndex] = 1;
			app->fsInstID  = fsm->m_execIndex;
			//std::cout << "[fsCore::parseMessage] REG_APP inst id : " << app->fsInstID << std::endl;

			sprintf(sailInitMsg, "%d %d %d %d", fsm->m_execIndex, fsm->nwInfo->rcvBufSize, fsm->nwInfo->sendBufSize, fsm->nwInfo->mtuSize);

			if (fsm->sendMessage(clientID, SAIL_INIT_MSG, sailInitMsg) < 0) {
				sage::printLog("fsCore::parseMessage() : REG_APP : %s is stuck or shutdown", app->appName);
			}
			else {
				fprintf(stderr,"fsCore::%s() : REG_APP : sailInitMsg [%s] has sent to clientID %d\n", __FUNCTION__, sailInitMsg, clientID);
				fsm->execList.push_back(app);
			}

			if (fsm->NRM) {
				char rcvIP[SAGE_IP_LEN];
				fsm->vdtList[0]->getNodeIPs(0, rcvIP);
				char msgStr[TOKEN_LEN];
				memset(msgStr, 0, TOKEN_LEN);
				sprintf(msgStr, "%s %s %d %d", app->renderNodeIP, rcvIP, app->bandWidth, fsm->m_execIndex);

				int uiNum = fsm->uiList.size();
				for (int j=0; j<uiNum; j++) {
					if (fsm->uiList[j] < 0)
						continue;

					if (fsm->sendMessage(fsm->uiList[j], REQUEST_BANDWIDTH, msgStr) < 0) {
						sage::printLog("fsCore : uiClient(%d) is stuck or shutdown", j);
						fsm->uiList[j] = -1;
					}
					else {
						fprintf(stderr, "fsCore::%s() : REG_APP : msg [%s] sent to client %d\n", __FUNCTION__, msgStr, fsm->uiList[j]);
					}
				}
			}
			else {
				initDisp(app);
				if (app->audioOn) {
					std::cout << "initAudio is called" << std::endl;
					initAudio();
				}

				//windowChanged(fsm->execList.size()-1);
				//bringToFront(fsm->execList.size()-1);
				fsm->m_execIndex++;
			}

			break;
		}
	}
	**/
}


