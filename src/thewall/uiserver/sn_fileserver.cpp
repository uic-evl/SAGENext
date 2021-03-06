#include "uiserver/sn_fileserver.h"
#include "uiserver/sn_uiserver.h"
#include "sn_sagenextlauncher.h"
#include "sn_mediastorage.h"

#include <QSettings>
#include <QTcpSocket>
#include <QFile>
#include <QDir>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>


SN_FileServerThread::SN_FileServerThread(int sockfd, const quint32 uiclientid, QObject *parent)
    : QThread(parent)
    , _uiclientid(uiclientid)
    , _dataSock(sockfd)
    , _end(false)
{
}

SN_FileServerThread::~SN_FileServerThread() {
	_end = true;
	::close(_dataSock);
//	qDebug() << "~FileServerThread" << _uiclientid;
}

void SN_FileServerThread::endThread() {
	_end = true;
	::shutdown(_dataSock, SHUT_RDWR);
	::close(_dataSock);
}

int SN_FileServerThread::_recvFile(SAGENext::MEDIA_TYPE mediatype, const QString &filename, qint64 filesize) {
	//
	// if it's just web url
	//
	if (mediatype == SAGENext::MEDIA_TYPE_WEBURL) {
		emit fileReceived((int)mediatype, filename);
		return 0;
	}

	//
	// if it's a real file
	//
	if (filesize <= 0) return -1;

	QString destdir = QDir::homePath().append("/.sagenext/");

	switch(mediatype) {
	case SAGENext::MEDIA_TYPE_IMAGE : {
		destdir.append("media/image/");
		break;
	}
	case SAGENext::MEDIA_TYPE_LOCAL_VIDEO : {
		destdir.append("media/video/");
		break;
	}
	case SAGENext::MEDIA_TYPE_PDF : {
		destdir.append("media/pdf/");
		break;
	}
	case SAGENext::MEDIA_TYPE_PLUGIN : {
		destdir.append("media/plugins/");
		break;
	}
	default: {
		qDebug() << "FileServerThread::_recvFile() : unknown media type";
		break;
	}
	} // end switch

	QFile file( destdir.append(filename) );
	if ( ! file.open(QIODevice::WriteOnly) ) {
		qDebug() << "FileServerThread::_recvFile() : file can't be opened for writing";
		return -1;
	}
	qDebug() << "FileServerThread::_recvFile() :  will receive" << file.fileName() << filesize << "Byte";

	QByteArray buffer(filesize, 0);

    char *bufptr = buffer.data();
    qint64 remained = filesize;
    qint64 chunksize = 10485760; // 10 MB

    while (remained > 0) {
        if (remained < chunksize) {
            chunksize = remained;
        }

//        file.write(bufptr, chunksize);

        if ( ::recv(_dataSock, bufptr, chunksize, MSG_WAITALL) <= 0 ) {
            qCritical("%s::%s() : error while receiving the file.", metaObject()->className(), __FUNCTION__);
            emit bytesWrittenToFile(_uiclientid, filename, -1); //meaning it's cancelled
            return -1;
        }

        bufptr += chunksize;

        remained -= chunksize;

        emit bytesWrittenToFile(_uiclientid, filename, filesize - remained);
    }


	file.write(buffer);


	if (!file.exists() || file.size() <= 0) {
		qDebug("%s::%s() : %s is not a valid file", metaObject()->className(), __FUNCTION__, qPrintable(file.fileName()));
		return -1;
	}

	emit fileReceived((int)mediatype, file.fileName());

	file.close();

	return 0;
}

int SN_FileServerThread::_sendFile(const QString &filepath) {
	QFile f(filepath);
	if (! f.open(QIODevice::ReadOnly)) {
		qDebug() << "FileServerThread::_sendFile() : couldn't open the file" << filepath;
		return -1;
	}

    qDebug() << "FileServerThread::_sendFile() : will send" << f.fileName() << f.size() << "Byte";

	//
	// very inefficient way to send file !!
	//
	if ( ::send(_dataSock, f.readAll().constData(), f.size(), 0) <= 0 ) {
		qDebug() << "FileServerThread::_sendFile() : ::send error";
		return -1;
	}

	f.close();
	return 0;
}


void SN_FileServerThread::run() {
//	qDebug() << "FileServerThread is running for uiclient" << _uiclientid;

	// always receive filename, filesize, media type first
	char header[EXTUI_MSG_SIZE];

	while(!_end) {
		int recv = ::recv(_dataSock, header, EXTUI_MSG_SIZE, MSG_WAITALL);
		if ( recv == -1 ) {
			qDebug("%s::%s() : socket error", metaObject()->className(), __FUNCTION__);
			_end = true;
			break;
		}
		else if (recv == 0) {
			qDebug("%s::%s() : socket disconnected", metaObject()->className(), __FUNCTION__);
			_end = true;
			break;
		}

		int mode; // up or down
//		int mediatype;
		SAGENext::MEDIA_TYPE mediatype;
		char filename[256];
		qint64 filesize;
		::sscanf(header, "%d %d %s %lld", &mode, (int*)&mediatype, filename, &filesize);



		if (mode == 0) {
			// download from uiclient
//			qDebug() << "FileServerThread received the header for downloading" << mediatype << filename << filesize << "Byte";
			if ( _recvFile(mediatype, QString(filename), filesize) != 0) {
				qDebug() << "FileServerThread failed to receive a file" << filename;
			}
		}
		else if (mode == 1) {
			// upload to uiclient
//			qDebug() << "FileServerThread received the header for uploading" << filename << filesize << "Byte";

			 // filename must be full absolute path name
			if ( _sendFile(QString(filename)) != 0 ) {
				qDebug() << "FileServerThread failed to send a file" << filename;
			}
		}
	}
}















SN_FileServer::SN_FileServer(const QSettings *s, SN_Launcher *l, SN_UiServer *uiserver, SN_MediaStorage *ms, QObject *parent)
    : QTcpServer(parent)
    , _settings(s)
    , _fileServerPort(0)
    , _launcher(l)
    , _uiServer(uiserver)
    , _mediaStorage(ms)
{
	_fileServerPort = _settings->value("general/fileserverport", 46000).toInt();

	if ( ! listen(QHostAddress::Any, _fileServerPort) ) {
        qCritical("FileServer::%s() : listen failed", __FUNCTION__);
        deleteLater();
    }
	else {
		qWarning() << "FileServer has started." << serverAddress() << serverPort();
	}
}

SN_FileServer::~SN_FileServer() {
	close(); // stop listening

	qDebug() << "In ~FileServer(), deleting fileserver threads";
	foreach(SN_FileServerThread *thread, _uiFileServerThreadMap.values()) {
		if (thread) {
			if (thread->isRunning()) {
//				thread->exit();
				thread->endThread();
				thread->wait();
			}

			if(thread->isFinished()) {
				delete thread;
			}
		}
	}

	qDebug() << "~FileServer";
}

void SN_FileServer::incomingConnection(int handle) {

	// receive uiclientid from the client
	quint32 uiclientid = 0;
	char msg[EXTUI_MSG_SIZE];
	if ( ::recv(handle, msg, EXTUI_MSG_SIZE, MSG_WAITALL) < 0 ) {
		qDebug("%s::%s() : error while receiving ", metaObject()->className(), __FUNCTION__);
		return;
	}
	::sscanf(msg, "%u", &uiclientid); // read uiclientid
	qDebug("%s::%s() : The ui client %u has connected to FileServer", metaObject()->className(), __FUNCTION__, uiclientid);


	SN_FileServerThread *thread = new SN_FileServerThread(handle, uiclientid);
	QObject::connect(thread, SIGNAL(finished()), this, SLOT(threadFinished()));

    //
    // signal the launcher to launch the media
    //
	QObject::connect(thread, SIGNAL(fileReceived(int,QString)), _launcher, SLOT(launch(int,QString)));

    //
    // signal the media storage to store the media
    //
    QObject::connect(thread, SIGNAL(fileReceived(int,QString)), _mediaStorage, SLOT(addNewMedia(int,QString)));

    //
    // give the uiclient (sagenextPointer) the feedback
    //
    QObject::connect(thread, SIGNAL(bytesWrittenToFile(qint32,QString,qint64)), this, SLOT(sendRecvProgress(qint32,QString,qint64)));

	_uiFileServerThreadMap.insert(uiclientid, thread);

	thread->start();
}

void SN_FileServer::sendRecvProgress(qint32 uiclientid, QString filename, qint64 bytes) {
    QByteArray msg(EXTUI_MSG_SIZE, 0);
    sprintf(msg.data(), "%d %s %lld", SAGENext::FILESERVER_RECVING_FILE, qPrintable(filename), bytes);
    /*
    QMetaObject::invokeMethod(_uiServer, "sendMsgToUiClient", Qt::AutoConnection
                              , Q_RETURN_ARG(int, retval)
                              , Q_ARG(quint32, uiclientid)
                              , Q_ARG(QByteArray, msg)
                              );
                              */
    _uiServer->sendMsgToUiClient(uiclientid, msg);
}

void SN_FileServer::threadFinished() {
	foreach(const SN_FileServerThread *thread, _uiFileServerThreadMap.values()) {
		if (thread) {
			if(thread->isFinished()) {
				_uiFileServerThreadMap.erase( _uiFileServerThreadMap.find( thread->uiclientid() ) );
				delete thread;
			}
		}
	}
}
