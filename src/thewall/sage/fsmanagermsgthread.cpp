#include "sage/fsmanagermsgthread.h"
//#include "sage/fsManager.h"
//#include "sage/sageLegacy.h"

#include "applications/base/sn_sagestreamwidget.h"
#include "applications/base/sn_affinityinfo.h"

#include <QStringList>
#include <QSettings>
#include <QtCore>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> // IPROTO_TCP
#include <netinet/tcp.h> // TCP_NODELY
#include <unistd.h>


fsManagerMsgThread::fsManagerMsgThread(const quint64 sageappId, int sockfd, const QSettings *settings, QObject *parent)
    : QThread(parent)
    , _settings(settings)
    , _sageWidget(0)
    , socket(sockfd)
    , _end(false)
    , _sageAppId(sageappId)
{
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
    wait();
    qDebug() << "[" << QTime::currentTime().toString("hh:mm:ss.zzz") << "] ~fsManagerMsgThread";
}

void fsManagerMsgThread::breakWhileLoop() {
	_end = true;
}

void fsManagerMsgThread::sendSailShutdownMsg() {
	sendSailShutdownMsg(_sageAppId);
}

void fsManagerMsgThread::sendSailShutdownMsg(quint64 sageappid) {
    if ( _sageAppId != sageappid ) return;

    OldSage::sageMessage msg;
    msg.init(36); // MESSAGE_HEADER_SIZE (4 * 9Byte)
    msg.setDest(sageappid);
    msg.setCode(OldSage::APP_QUIT); // send APP_QUIT to sail. sail will send SAIL_SHUTDOWN
    if ( ::send(socket, (char *)msg.getBuffer(), msg.getBufSize(), 0) <= 0 ) {
        qDebug("fsManagerMsgThread::%s() : failed to send APP_QUIT to sageappid %llu", __FUNCTION__, sageappid);
    }
    else {
        //qDebug("fsManagerMsgThread::%s() : APP_QUIT sent for sageappid %llu", __FUNCTION__, sageappid);
    }
    msg.destroy();
    _end = true;
}

void fsManagerMsgThread::sendSailSetRailMsg(SN_AffinityInfo *aff, quint64 sageappid) {
    if (!aff) return;
    if ( _sageAppId != sageappid) return;

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
        //qDebug("fsManagerMsgThread::%s() : SAIL_SET_RAIL [%s] has sent", __FUNCTION__, (char *)msg.getData());
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
//    qDebug() << "fsmThread::sendSailMsg() : msg sent" << QString(msg.getBuffer());
//	msg.destroy();
}

void fsManagerMsgThread::sendSailMsg(int msgcode, const QString &msgdata) {
	OldSage::sageMessage msg;
	msg.init(_sageAppId, msgcode, 0, msgdata.length(), qPrintable(msgdata));
	sendSailMsg(msg);
	msg.destroy();
}

void fsManagerMsgThread::signalSageWidgetCreated() {
    quint64 gaid = 0;
    if (_sageWidget) {
        gaid = _sageWidget->globalAppId();
    }
    qDebug() << "fsManagerMsgThread::signalSageWidgetCreated() : _sageWidget GID" << gaid;
    _isSageWidgetCreated.wakeOne();
}

void fsManagerMsgThread::run() {
//	qDebug("fsManagerMsgThread::%s() : msgThread starting. sageAppId %llu, sockfd %d",  __FUNCTION__, sageAppId, socket);
	OldSage::sageMessage sageMsg;
	QByteArray msgStr(OldSage::MESSAGE_FIELD_SIZE, '\0');

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

		//qDebug("fsManagerMsgThread::%s() : received msg field [%s]", __FUNCTION__, msgStr.data());

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

		//qDebug("fsManagerMsgThread::%s() : received sageMessage, size %d, dest %d, code %d, appCode %d", __FUNCTION__, sageMsg.getSize(), sageMsg.getDest(), sageMsg.getCode(), sageMsg.getAppCode());

		parseMessage(sageMsg);
		sageMsg.destroy();
	}
//	qDebug("fsManagerMsgThread::%s() : msgThread for sageAppId %llu exit the loop", __FUNCTION__, _sageAppId);
}

void fsManagerMsgThread::parseMessage(OldSage::sageMessage &sageMsg) {

        switch(sageMsg.getCode()) {
        case OldSage::REG_APP : {
            //qDebug("fsManagerMsgThread::%s() : REG_APP : msgData [%s] \n", __FUNCTION__, qPrintable(msgStr));
            char appn[128];
            char temp[512];
            int x, y;
            int width, height;
            int dummy; // for variables not used by SAGENext
            int protocol;

            char filepath[256];

            sscanf((char *)sageMsg.getData(), "%s %d %d %d %d %d %s %d %d %d %d %d %d %s %s"
                   , appn /* SAGE app binary name (e.g. mplayer) */
                   , &x /* position x */
                   , &y /* position y */
                   , &dummy /* window size width */
                   , &dummy /* window size height */
                   , &dummy /* bandwidthReq = 0.5 Mbps + (sageConfig::frameRate * w * h * bpp) * 1e-6 */
                   , temp /* render node IP  (sageConfig::streamIP) */
                   , &width /* native width */
                   , &height /* native height */
                   , &dummy /* audio On */
                   , &protocol /* network protocol */
                   , &dummy /* config.frameRate */
                   , &dummy /* config.appID  */
                   , temp /* config.launcherID */
                   , filepath /* config.mediaFileName  (SAGE_NAME_LEN 256 Byte) */
                   );

            _sageAppName = QString(appn);

            //
            // This will trigger the SN_Launcher::launch()
            //
            emit sageAppConnectedToFSM(_sageAppName, QString(filepath), this);
            //qDebug() << "fsmThread signal emitted" << _sageAppName;

            //
            // wait until the receiver (SN_SageStreamWidget) is created
            //
            //
            // The SageStreamWidget is created either
            //
            // by the slot SN_Launcher::launch(fsmThread *) which is connected to fsManager::incomingSail(fsmThread *) signal
            // or
            // by the slot SN_Launcher::launchSageApp() which is invoked internally
            //
            _mutex.lock();
            while(!_sageWidget) {
                qDebug("%s::%s() : fsm is waiting for SN_SageStreamWidget is created", metaObject()->className(), __FUNCTION__);
                _isSageWidgetCreated.wait(&_mutex);
            }
            // SN_SageStreamWidget created at this point
            _mutex.unlock();

            Q_ASSERT(_sageWidget);


            //
            // Determine the port number for the streamnig channel
            //
            int streamPort = _settings->value("general/fsmstreambaseport", 20005).toInt() + _sageAppId;
            if(streamPort > 65535) {
                qCritical("\nfsManagerMsgThread::%s() : The streamPort  has set to %d\n", __FUNCTION__, streamPort);
                _sageWidget->close();
                _end = true;
                return;
            }

            //
            // Trigger the SN_SageStreamWidget to blocking wait (::accept()) for the streamer
            //
            //qDebug() << "fsmsgthread invoking doInitReceiver() for sage app" << _sageAppId << "streamport" << streamPort << QTime::currentTime().toString("hh:mm:ss.zzz");
            QMetaObject::invokeMethod(_sageWidget, "doInitReceiver", Qt::QueuedConnection
                                      , Q_ARG(quint64, _sageAppId)
                                      , Q_ARG(QString, _sageAppName)
                                      , Q_ARG(QRect, QRect(x, y, width, height))
                                      , Q_ARG(int, protocol)
                                      , Q_ARG(int, streamPort)
                                      );



            /*********************************************
              send the SAIL_INIT_MSG to the streamer.
              *********************************************

              The SAIL_INIT_MSG contains network parameters (send/recive socket buffer size aka window size)
              and is handled by the sail::parseMessage().
              What it does is ...

              read network parameter data in the message and calls
              pixelStreamer->setNwConfig (sageBlockStreamer::setNwConfig)

              sageBlockPartition and sageBlockGroup will be created
            */
            OldSage::sageMessage sageMsg;
            QByteArray initMsg(32, '\0');

            //
            // these parameters are set in the SettingStackedDialog::on_buttonBox_accepted()
            //
            sprintf(initMsg.data(), "%d %d %d %d",
                    (int)_sageAppId,
                    _settings->value("network/recvwindow", 4*1048576).toInt(),
                    _settings->value("network/sendwindow", 1048576).toInt(),
                    _settings->value("network/mtu", 8800).toInt()); // snd/rcv network socket size followed by MTU

            if (sageMsg.init(_sageAppId, OldSage::SAIL_INIT_MSG, 0, initMsg.size(), initMsg.data()) < 0) {
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


            //
            // wait until the SN_SageStreamWidget / SagePixelReceiver can be started
            // If this is too short, sender (the sage app) could run ::connect() before ::accept() is called (thus blocking waiting) at the SN_SageStreamWidget (receiver)
            //
            forever {
                if ( _sageWidget->isWaitingSailToConnect() ) {
                    QThread::msleep(100);
                    break;
                }
                else {
                    //QThread::yieldCurrentThread();
                    QThread::msleep(100);
                }
            }

            /*****************************************************
             * send SAIL_CONNECT_TO_RCV to the streamer
             *****************************************************/
            /*
              The SAIL_CONNECT_TO_RCV contains the port number for the streaming channel, # of receivers, receiver IPs
              and is handled by the sail::parseMessage().
              What it does is calling pixelStreamer->initNetworks() (sageStreamer::initNetworks()) followed by the sageStreamer::connectToRcv() followed by the streamLoop thread
              */
            initMsg.fill('\0');
            sprintf(initMsg.data(), "%d %d %s %d"
                    , streamPort // the port # for pixel receiving (SN_SageStreamWidget will listen on this port)
                    , -1 // the number of SDM. There is only one in SAGENext
                    , qPrintable(_settings->value("general/fsmip").toString()) // ip addr of the receiver thread (which can be different from fsManager IP)
                    /** , localPort // if sageStreamer::localPort is on then the localPort # should be placed here. SAIL_CONNECT_TO_RCV_PORT  **/
                    , 0 /* SDM id */
                    );

            if (sageMsg.init(_sageAppId, OldSage::SAIL_CONNECT_TO_RCV, 0, initMsg.size(), initMsg.constData()) < 0) {
                qCritical("fsManagerMsgThread::%s() : failed to init sageMessage", __FUNCTION__);
                _end=true;
                break;
            }
            dataSize = sageMsg.getBufSize();
            //						qDebug() << "fsmsgthread send SAIL_CONNECT_TO_RCV for sageappid" << sageAppId << QTime::currentTime().toString("hh:mm:ss.zzz");
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


