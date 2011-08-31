#include "sagestreamwidget.h"
#include "sagepixelreceiver.h"

#include "../sage/fsmanagermsgthread.h"

#include "../sage/sagecommondefinitions.h"

#include "../common/commonitem.h"
#include "../common/imagedoublebuffer.h"

#include "base/appinfo.h"
#include "base/affinityinfo.h"
#include "base/perfmonitor.h"

#include "../system/resourcemonitor.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <sys/time.h>
#include <sys/resource.h>


//SageStreamWidget::SageStreamWidget(quint64 sageAppId, QString appName, int protocol, int port, const QRect initRect, const quint64 globalAppId, const QSettings *s, ResourceMonitor *rm, QGraphicsItem *parent/*0*/, Qt::WindowFlags wFlags/*0*/) :

//	RailawareWidget(globalAppId, s, rm, parent, wFlags), // QSettings *s is const

//	_fsmMsgThread(0),
//	_sageAppId(sageAppId),
//	receiverThread(0),
//	image(0),
//	doubleBuffer(new ImageDoubleBuffer),
//	serversocket(0),
//	streamsocket(0),
//	imageSize(0),
//	frameCounter(0)
//{
////	Q_UNUSED(appName);

//	if ( port > 65535 ) {
//		qCritical("SageStreamWidget::%s() : Invalid port number %d", __FUNCTION__, port);
//		deleteLater();
//	}

//	/* receive regMsg from sail and create buffer */

//	if ( initialize(protocol, port) != 0 ) {
//		qCritical("SageStreamWidget::%s() : failed to initialize", __FUNCTION__);
//		deleteLater();
//	}
//	else {
//		// AffinityInfo is created in RailawareWidget
//		/* when there's no resource monitor, affinityInfo isn't created */
//		if(_affInfo)
//			_affInfo->setWidgetID(sageAppId);

//		Q_ASSERT(_appInfo);
//		_appInfo->setFrameSize(image->width(), image->height(), image->depth()); // == _image->byteCount()

////		qDebug("SageStreamWidget::%s() : sageappid %llu, groupsize %d, frameSize(SAIL) %d, frameSize(QImage) %d, expectedFps %.2f", __FUNCTION__, sageAppId, _appInfo->getNetworkUserBufferLength(), imageSize, _appInfo->getFrameBytecount(), _perfMon->getExpetctedFps());

//		_appInfo->setExecutableName( appName );
//		if ( appName == "imageviewer" ) {
//			_appInfo->setMediaType(MEDIA_TYPE_IMAGE);
//		}
//		else {
//			_appInfo->setMediaType(MEDIA_TYPE_VIDEO);
//		}



//		/* starting receiving thread */

//		// image->bits() will do deep copy (detach)
//		receiverThread = new SagePixelReceiver(protocol, streamsocket, /*image*/ doubleBuffer, _appInfo, _perfMon, _affInfo, /*this, mutex, wc,*/ settings);
////		qDebug("SageStreamWidget::%s() : SagePixelReceiver thread has begun",  __FUNCTION__);

//		connect(receiverThread, SIGNAL(finished()), this, SLOT(fadeOutClose())); // WA_Delete_on_close is defined

//		// don't do below.
////		connect(receiverThread, SIGNAL(finished()), receiverThread, SLOT(deleteLater()));


////		if (!scheduler) {
//			// This is queued connection because receiverThread reside outside of the main thread
//			if ( ! connect(receiverThread, SIGNAL(frameReceived()), this, SLOT(scheduleUpdate())) ) {
//				qCritical("%s::%s() : Failed to connect frameReceived() signal and scheduleUpdate() slot", metaObject()->className(), __FUNCTION__);
//			}
//			else {
////				qDebug("%s::%s() : frameReceived() -> scheduleUpdate() are connected", metaObject()->className(), __FUNCTION__);
//			}
////		}
//		receiverThread->start();


//		/*
//		QFuture<void> future = QtConcurrent::run(this, &SageStreamWidget::pixelRecvThread);
//		futureWatcher.setFuture(future);
//		connect(&futureWatcher, SIGNAL(finished()), this, SLOT(close()));
//		connect(this, SIGNAL(pauseThread()), &futureWatcher, SLOT(pause()));
//		connect(this, SIGNAL(resumeThread()), &futureWatcher, SLOT(resume()));
//		*/
//		setPos(initRect.x(), initRect.y());
//	}
//}

//void SageStreamWidget::stopPixelReceiver() {
//	if ( ::shutdown(socket, SHUT_RDWR) != 0 )
//		qDebug("SageStreamWidget::%s() : error while shutdown recv socket", __FUNCTION__);

//	if (receiverThread && receiverThread->isRunning()) {
//		QMetaObject::invokeMethod(receiverThread, "endReceiver",  Qt::QueuedConnection);
//		qApp->sendPostedEvents();
////		receiverThread->wait();
//	}
//}


SageStreamWidget::SageStreamWidget(QString filename, const quint64 globalappid, const QSettings *s, QString senderIP, ResourceMonitor *rm, QGraphicsItem *parent, Qt::WindowFlags wFlags) :

	RailawareWidget(globalappid, s, rm, parent, wFlags),

	_fsmMsgThread(0),
	_sageAppId(0),
	receiverThread(0),
	image(0),
	doubleBuffer(0),
	serversocket(0),
	streamsocket(0),
	imageSize(0),
	frameCounter(0)
{
}


void SageStreamWidget::fadeOutClose() {

	/* signal APP_QUIT throught fsmanagerMsgThread
This signal is connected in GraphicsViewMain::startSageApp() */
//	emit destructor(_sageAppId);

	disconnect(receiverThread, SIGNAL(frameReceived()), this, SLOT(scheduleUpdate()));
//	disconnect(this, SLOT(scheduleReceive()));

//  connect(sageWidget->affInfo(), SIGNAL(cpuOfMineChanged(RailawareWidget *,int,int)), resourceMonitor, SLOT(updateAffInfo(RailawareWidget *,int,int)));
//  connect(sageWidget->affInfo(), SIGNAL(streamerAffInfoChanged(AffinityInfo*, quint64)), fsm, SIGNAL(sailSendSetRailMsg(AffinityInfo*,quint64)));
	if (_affInfo) _affInfo->disconnect();

	if (doubleBuffer) {
		doubleBuffer->releaseBackBuffer();
		doubleBuffer->releaseLocks();
	}

	if (rMonitor) {
//		_affInfo->disconnect();

		rMonitor->removeSchedulableWidget(this); // remove this from ResourceMonitor::widgetMultiMap
		rMonitor->removeApp(this); // will emit appRemoved(int) which is connected to Scheduler::loadBalance()
//		qDebug() << "affInfo removed from resourceMonitor";

		// don't do below
//		if (_affInfo) {
//			_affInfo->setWidgetPtr(0);
//			//		qDebug() << "affInfo setWidgetPtr(0)";
//			delete _affInfo;
//			_affInfo = 0;
//			//		qDebug() << "affInfo =0 ";
//		}
	}
	RailawareWidget::fadeOutClose();
	qDebug() << "SageStreamWidget::fadeOutClose()";
}

SageStreamWidget::~SageStreamWidget()
{
//	if (doubleBuffer) delete doubleBuffer;

	if (receiverThread && receiverThread->isRunning()) {
		receiverThread->endReceiver();
//		QMetaObject::invokeMethod(receiverThread, "endReceiver",  Qt::QueuedConnection);
//		qApp->sendPostedEvents(static_cast<QObject *>(receiverThread), QEvent::MetaCall);
//		receiverThread->wait();
//		receiverThread->terminate();
//		delete receiverThread;
	}


//	if (receiverThread /*&& receiverThread->isFinished()*/)
//		delete receiverThread;


	/***
	if ( futureWatcher.isPaused() ) {
		futureWatcher.resume();
	}
//	futureWatcher.cancel(); // This isn't work for a thread run by QtConcurrent::run()
	if ( futureWatcher.isRunning())
		futureWatcher.waitForFinished(); // deadlock
		***/

	/* terminate pixelReceiver thread */
	/*
	::shutdown(socket, SHUT_RDWR);
	if ( receiverThread && receiverThread->isRunning() ) {
//		qDebug("SageStreamWidget::%s() : %llu, %d terminating receiver thread", __FUNCTION__,globalAppId, sageAppId);
		receiverThread->stopThread();
//		receiverThread->terminate();
//		receiverThread->wait(); // deadlock
	}
	receiverThread = 0;
 */

	/* with scheduler on, this must be here */
	if (doubleBuffer) {
		doubleBuffer->releaseBackBuffer();
		doubleBuffer->releaseLocks();
	}
	if (doubleBuffer) delete doubleBuffer;

//	if (image) delete image;

//	delete receiverThread;
	receiverThread->deleteLater();

	qDebug("SageStreamWidget::%s() ",  __FUNCTION__);
}

void SageStreamWidget::setFsmMsgThread(fsManagerMsgThread *thread) {
	_fsmMsgThread = thread;
	_fsmMsgThread->start();
}


void SageStreamWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {

//	struct timeval s,e;
//	gettimeofday(&s, 0);

	if (_perfMon) {
		_perfMon->getDrawTimer().start();
//		perfMon->startPaintEvent();
	}

	if (isSelected()) {
		// setBrush hurts performance badly
//		painter->setBrush( QBrush(Qt::lightGray, Qt::Dense2Pattern) ); // very bad

//		painter->drawRect( windowFrameRect() ); // will add 0.5~1 msec when 4K
//		painter->fillRect(windowFrameRect(), QBrush(Qt::lightGray, Qt::Dense6Pattern)); // bad
//		painter->fillRect(windowFrameRect(), Qt::lightGray); // will adds 2~3 msec when 4K res
//		painter->fillRect(windowFrameRect(), Qt::Dense6Pattern); // bad

//		shadow->setEnabled(true);
	}
	else {
//		shadow->setEnabled(false);
	}

//	if ( currentScale != 1.0 ) painter->scale(currentScale, currentScale);

//	Q_ASSERT(image && !image->isNull());
//	painter->drawImage(0, 0, *image2); // Implicit Sharing

	// slower than painter->scale()
//	painter->drawImage(QPointF(0, 0), _image->scaled(cs.toSize(), Qt::KeepAspectRatio));

//	  This is bettern than
//	  pixmap->convertFromImage(*image) followed by painter->drawPixmap(*pixmap)
//	  in terms of the total latency for a frame.
//	  HOWEVER, paint() can be called whenever move, resize
//	  So, it is wise to have paint() function fast
//	painter->drawPixmap(0, 0, QPixmap::fromImage(*_image));


//	Q_ASSERT(pixmap && !pixmap->isNull());
	if(!_pixmap.isNull())
		painter->drawPixmap(0, 0, _pixmap); // the best so far


	if ( showInfo  &&  !infoTextItem->isVisible() ) {
#if defined(Q_OS_LINUX)
		_appInfo->setDrawingThreadCpu(sched_getcpu());
#endif
		infoTextItem->show();
	}
	else if (!showInfo && infoTextItem->isVisible()){
		infoTextItem->hide();
	}
	if (_perfMon)
		_perfMon->updateDrawLatency(); // drawTimer.elapsed() will be called.

//	gettimeofday(&e, 0);
//	qreal el = ((double)e.tv_sec + (double)e.tv_usec * 0.000001) - ((double)s.tv_sec+(double)s.tv_usec*0.000001);
//	qDebug() << "drawing : " << el * 1000.0 << " msec";

/**
	struct timeval s,e;
	gettimeofday(&s, 0);

	glEnable(GL_TEXTURE_2D);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
//	glOrtho(0, 1600, 1200, 0, 0, 100);
	glMatrixMode(GL_MODELVIEW);

	glBindTexture(GL_TEXTURE_2D, texhandle);

	glBegin(GL_QUADS);
	// top left
	glTexCoord2f(0, 0);
	glVertex3f(0, 0, -1);

	// top right (0,0)
	glTexCoord2f(0, 1);
	glVertex3f(0, image->height(), -1);

	// bottom right
	glTexCoord2f(1, 1);
	glVertex3f(image->width(), image->height(), -1);

	// bottom left
	glTexCoord2f(1, 0);
	glVertex3f(image->width(), 0, -1);

	glEnd();
	glFlush();
	glDisable(GL_TEXTURE_2D);

	gettimeofday(&e, 0);
	qreal el = ((double)e.tv_sec + (double)e.tv_usec * 0.000001) - ((double)s.tv_sec+(double)s.tv_usec*0.000001);
	qDebug() << "darwing : " << el * 1000.0 << " msec";
	**/
}

//void SageStreamWidget::resizeEvent(QGraphicsSceneResizeEvent *event) {
//	glViewport(0, 0, event->newSize().width(), event->newSize().height());
//	glMatrixMode(GL_PROJECTION);
//	glLoadIdentity();
//	glMatrixMode(GL_MODELVIEW);

//	BaseGraphicsWidget::resizeEvent(event);
//}


//void SageStreamWidget::scheduleUpdate() {
//	QImage *imgPtr = 0;

//	if ( !pixmap || !doubleBuffer || !receiverThread || !receiverThread->isRunning() || receiverThread->isFinished() )
//		return;

//	/*
//	  will wait until producer produces one
//	 */
//	imgPtr = static_cast<QImage *>(doubleBuffer->getBackBuffer());

//	//		qDebug() << QTime::currentTime().toString("mm:ss.zzz") << " widget retrieved " << frameCounter + 1;
////	qDebug() << QTime::currentTime().toString("mm:ss.zzz") << "backbuffer received";

//	if ( imgPtr && !imgPtr->isNull() ) {
//		_perfMon->getConvTimer().start();

////		qDebug() << QTime::currentTime().toString("mm:ss.zzz") << "back buffer valid";

//		// converts to QPixmap if you're gonna paint same QImage more than twice.
//		if (! pixmap->convertFromImage(*imgPtr, Qt::AutoColor | Qt::ThresholdDither) ) {
//			qDebug("SageStreamWidget::%s() : pixmap->convertFromImage() error", __FUNCTION__);
//		}
//		else {
//			//++frameCounter;
////			qDebug() << QTime::currentTime().toString("mm:ss.zzz") << "pixmap has converted";

//			_perfMon->updateConvDelay();

//			_perfMon->getEqTimer().start();
//			//qDebug() << QTime::currentTime().toString("mm:ss.zzz") << " widget is about to call update() for frame " << frameCounter;

//			// Schedules a redraw. This is not an immediate paint. This actually is postEvent()
//			// QGraphicsView will process the event
//			update(); // post paint event to myself (QEvent::MetaCall)


//			/****
//			  With scheduler on, below makes sagestreamwidget hangs !!!!
//			  ****/
//			// dispatch immediately
////			qApp->sendPostedEvents(this, QEvent::MetaCall);
//		}
//	}
//	else {
//		qCritical("SageStreamWidget::%s() : globalAppId %llu, sageAppId %llu : imgPtr is null. Failed to retrieve back buffer from double buffer", __FUNCTION__, globalAppId(), sageAppId);
//	}

//	/*
//	  will make doubleBuffer->swapBuffer() returns
//	 */
//	doubleBuffer->releaseBackBuffer();
////	qDebug() << QTime::currentTime().toString("mm:ss.zzz") << "back buffer released";
//	imgPtr = 0;
//}


void SageStreamWidget::scheduleReceive() {
//	qDebug() << "widget wakeOne";
//	if(wc) wc->wakeOne();
	receiverThread->receivePixel();
}

/**
  * this slot connected to the signal PixelReceiver::frameReceived()
  */
void SageStreamWidget::scheduleUpdate() {
//	struct timeval s,e;
//	gettimeofday(&s, 0);
	QImage *imgPtr = 0;

	if ( /*!image*/  !doubleBuffer || !receiverThread || receiverThread->isFinished() )
		return;

	else {
		imgPtr = static_cast<QImage *>(doubleBuffer->getBackBuffer());

//		qDebug() << QTime::currentTime().toString("mm:ss.zzz") << " widget retrieved " << frameCounter + 1;

//		qDebug() << globalAppId << ", " << sageAppId << " : here";
		if (imgPtr && !imgPtr->isNull() ) {

			_perfMon->getConvTimer().start();

			// converts to QPixmap if you're gonna paint same QImage more than twice.
			if (! _pixmap.convertFromImage(*imgPtr, Qt::AutoColor | Qt::ThresholdDither) )
				qDebug("SageStreamWidget::%s() : pixmap->convertFromImage() error", __FUNCTION__);
			else {

				setScheduled(false); // reset scheduling flag

				++frameCounter;
//				qDebug() << QTime::currentTime().toString("mm:ss.zzz") << " widget : " << frameCounter << " has converted";

				_perfMon->updateConvDelay();

				/*
				 Maybe I should schedule update() and releaseBackBuffer in the scheduler
				 */
				doubleBuffer->releaseBackBuffer();
				imgPtr = 0;

//				_perfMon->getEqTimer().start();
//				QDateTime::currentMSecsSinceEpoch();
//				qDebug() << QTime::currentTime().toString("mm:ss.zzz") << " widget is about to call update() for frame " << frameCounter;

				// Schedules a redraw. This is not an immediate paint. This actually is postEvent()
				// QGraphicsView will process the event
				update(); // post paint event to myself
//				qApp->sendPostedEvents(this, QEvent::MetaCall);
//				qApp->flush();
//				qApp->processEvents();

//				this->scene()->views().front()->update( mapRectToScene(boundingRect()).toRect() );
			}
		}
		else {
			qCritical("SageStreamWidget::%s() : globalAppId %llu, sageAppId %llu : imgPtr is null. Failed to retrieve back buffer from double buffer", __FUNCTION__, globalAppId(), _sageAppId);
		}
//		doubleBuffer->releaseBackBuffer();
//		imgPtr = 0;
	}

//	pixmap->convertFromImage(image->copy());

	/* resizing is handled here. This seems very slow */
//	pixmap->convertFromImage( image->scaledToWidth(currentScale * getNativeSize().width()) );
//	prepareGeometryChange();
//	resize(pixmap->width(), pixmap->height());

//	*image2 = image->copy();

	/* resizing is handled by QGraphicsItem::setScale(). 2 msec drawing latency becomes 8 msec as soon as scale changes from 1.0 */

//	convertedToPixmap->release();
//	pixmap->convertFromImage( *image2 );
//	pixmap->loadFromData(image->constBits(), image->byteCount()); // ~33 msec for 4K


//	glEnable(GL_TEXTURE_2D);
//	glBindTexture(GL_TEXTURE_2D, texhandle);
//	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image->width(), image->height(), GL_RGB, GL_UNSIGNED_BYTE, image->bits()); // f*cking 90 msec for 4K ???


//	gettimeofday(&e, 0);
//	qreal el = ((double)e.tv_sec + (double)e.tv_usec * 0.000001) - ((double)s.tv_sec+(double)s.tv_usec*0.000001);
//	qDebug() << "converting : " << el * 1000.0 << " msec";


//	if ( qApp->hasPendingEvents() ) {
//		UpdateEvent *ue = new UpdateEvent(QEvent::User + 1);
//		ue->widgetPtr = this;
//		ue->priority = 100;
//		qApp->postEvent(qApp, ue, ue->priority);
//	}
//	else {
		// draw immediately
//		QGraphicsView *gv = scene()->views().first();
//		Q_ASSERT(gv);
//		gv->viewport()->repaint(
//				gv->mapFromScene(
//						mapRectToScene(boundingRect())
//						).boundingRect()
//				);
//	}
}


int SageStreamWidget::createImageBuffer(int resX, int resY, sagePixFmt pixfmt) {
	int bytePerPixel = getPixelSize(pixfmt);
	int memwidth = resX * bytePerPixel; //Byte (single row of frame)

	imageSize = memwidth * resY; // Byte (a frame)

	qDebug("SageStreamWidget::%s() : recved regMsg. size %d x %d, pixfmt %d, pixelSize %d, memwidth %d, imageSize %d", __FUNCTION__, resX, resY, pixfmt, bytePerPixel, memwidth, imageSize);

	if (!doubleBuffer) doubleBuffer = new ImageDoubleBuffer;

	/*
	 Do not draw ARGB32 images into the raster engine.
	 ARGB32_premultiplied and RGB32 are the best ! (they are pixel wise compatible)
	 http://labs.qt.nokia.com/2009/12/18/qt-graphics-and-performance-the-raster-engine/
		 */
	switch(pixfmt) {
	case PIXFMT_888 : { // GL_RGB
			doubleBuffer->initBuffer(resX, resY, QImage::Format_RGB888);
			//		image = new QImage(resX, resY, QImage::Format_RGB32); // x0FFRRGGBB
			break;
		}
	case PIXFMT_888_INV : { // GL_BGR
			doubleBuffer->initBuffer(resX, resY, QImage::Format_RGB888);
			doubleBuffer->rgbSwapped();
			break;
		}
	case PIXFMT_8888 : { // GL_RGBA
			doubleBuffer->initBuffer(resX, resY, QImage::Format_RGB32);
			break;
		}
	case PIXFMT_8888_INV : { // GL_BGRA
			doubleBuffer->initBuffer(resX, resY, QImage::Format_RGB32);
			doubleBuffer->rgbSwapped();
			break;
		}
	case PIXFMT_555 : { // GL_RGB, GL_UNSIGNED_SHORT_5_5_5_1
			doubleBuffer->initBuffer(resX, resY, QImage::Format_RGB555);
			break;
		}
	default: {
			doubleBuffer->initBuffer(resX, resY, QImage::Format_RGB888);
			break;
		}
	}

//	if ( ! image || image->isNull() ) {
//		return -1;
//	}
	image = static_cast<QImage *>(doubleBuffer->getFrontBuffer());

//	glGenTextures(1, &texhandle);
//	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
//	glEnable(GL_TEXTURE_2D);
//	glBindTexture(GL_TEXTURE_2D, texhandle);
//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->width(), image->height(), 0, GL_RGB, GL_UNSIGNED_BYTE, image->bits());

	if (image && !image->isNull())
		return 0;
	else
		return -1;
}



int SageStreamWidget::initialize(quint64 sageappid, QString appname, QRect initrect, int protocol, int port) {

	_sageAppId = sageappid;


	/* accept connection from sageStreamer */
	serversocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if ( serversocket == -1 ) {
		qCritical("SageStreamWidget::%s() : couldn't create socket", __FUNCTION__);
		return -1;
	}

	// setsockopt
	int optval = 1;
	if ( setsockopt(serversocket, SOL_SOCKET, SO_REUSEADDR, &optval, (socklen_t)sizeof(optval)) != 0 ) {
		qWarning("SageStreamWidget::%s() : setsockopt SO_REUSEADDR failed",  __FUNCTION__);
	}

	// bind to port
	struct sockaddr_in localAddr, clientAddr;
	memset(&localAddr, 0, sizeof(localAddr));
	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	localAddr.sin_port = htons(protocol + port);

	// bind
	if( bind(serversocket, (struct sockaddr *)&localAddr, sizeof(struct sockaddr_in)) != 0) {
		qCritical("SageStreamWidget::%s() : bind error",  __FUNCTION__);
		return -1;
	}

	// put in listen mode
	listen(serversocket, 15);

	// accept
	/** accept will BLOCK **/
//	qDebug("SageStreamWidget::%s() : Blocking waiting for sender to connect to TCP port %d", __FUNCTION__,protocol+port);
	memset(&clientAddr, 0, sizeof(clientAddr));
	int addrLen = sizeof(struct sockaddr_in);
	if ((streamsocket = accept(serversocket, (struct sockaddr *)&clientAddr, (socklen_t*)&addrLen)) == -1) {
		qCritical("SageStreamWidget::%s() : accept error", __FUNCTION__);
		perror("accept");
		return -1;
	}

//	struct hostent *he = gethostbyaddr( (void *)&clientAddr, addrLen, AF_INET);
//	Q_ASSERT(he);
//	qDebug("SageStreamWidget::%s() : %s", __FUNCTION__, he->h_name);

	// read regMsg 1024Byte
	/*char regMsg[REG_MSG_SIZE];
		  sprintf(regMsg, "%d %d %d %d %d %d %d %d %d %d %d",
				  config.streamType, // HARD_SYNC
				  config.frameRate,
				  winID,
				  config.groupSize,
				  blockSize,
				  config.nodeNum,
				  (int)config.pixFmt,
				  config.blockX,
				  config.blockY,
				  config.totalWidth,
				  config.totalHeight);


				   [103 60 1 131072 12416 1 5 64 64 400 400]
		  */


	QByteArray regMsg(OldSage::REG_MSG_SIZE, '\0');
	int read = recv(streamsocket, (void *)regMsg.data(), regMsg.size(), MSG_WAITALL);
	if ( read == -1 ) {
		qCritical("SageStreamWidget::%s() : error while reading regMsg. %s",__FUNCTION__, "");
		return -1;
	}
	else if ( read == 0 ) {
		qCritical("SageStreamWidget::%s() : sender disconnected, while reading 1KB regMsg",__FUNCTION__);
		return -1;
	}

	QString regMsgStr(regMsg);
	QStringList regMsgStrList = regMsgStr.split(" ", QString::SkipEmptyParts);
	qDebug("SageStreamWidget::%s() : recved regMsg from sageStreamer::connectToRcv() [%s]",  __FUNCTION__, regMsg.constData());
	int framerate = regMsgStrList.at(1).toInt();
	int groupsize = regMsgStrList.at(3).toInt(); // this is going to be the network user buffer size
	_appInfo->setNetworkUserBufferLength(groupsize);
	int pixfmt = regMsgStrList.at(6).toInt();
	int resX = regMsgStrList.at(9).toInt();
	int resY = regMsgStrList.at(10).toInt();
	Q_ASSERT(resX > 0 && resY > 0);

//	qDebug() << "sd;fkljasdf;lkasjdf;    " << framerate << "\n";
	_perfMon->setExpectedFps( (qreal)framerate );
	_perfMon->setAdjustedFps( (qreal)framerate );

	resize(resX, resY);


	/* create double buffer */
	if ( createImageBuffer(resX, resY, (sagePixFmt)pixfmt) != 0 ) {
		qCritical("%s::%s() : imagedoublebuffer is not valid", metaObject()->className(), __FUNCTION__);
		::shutdown(streamsocket, SHUT_RDWR);
		QMetaObject::invokeMethod(_fsmMsgThread, "sendSailShutdownMsg", Qt::QueuedConnection);
		deleteLater();
		return -1;
	}



	if(_affInfo)
		_affInfo->setWidgetID(_sageAppId);

	Q_ASSERT(_appInfo);
	_appInfo->setFrameSize(image->width(), image->height(), image->depth()); // == _image->byteCount()

//		qDebug("SageStreamWidget::%s() : sageappid %llu, groupsize %d, frameSize(SAIL) %d, frameSize(QImage) %d, expectedFps %.2f", __FUNCTION__, sageAppId, _appInfo->getNetworkUserBufferLength(), imageSize, _appInfo->getFrameBytecount(), _perfMon->getExpetctedFps());

	_appInfo->setExecutableName( appname );
	if ( appname == "imageviewer" ) {
		_appInfo->setMediaType(MEDIA_TYPE_IMAGE);
	}
	else {
		_appInfo->setMediaType(MEDIA_TYPE_VIDEO);
	}



	/* starting receiving thread */

	// image->bits() will do deep copy (detach)
	receiverThread = new SagePixelReceiver(protocol, streamsocket, /*image*/ doubleBuffer, _appInfo, _perfMon, _affInfo, /*this, mutex, wc,*/ settings);
//		qDebug("SageStreamWidget::%s() : SagePixelReceiver thread has begun",  __FUNCTION__);

	Q_ASSERT(receiverThread);

	connect(receiverThread, SIGNAL(finished()), this, SLOT(fadeOutClose())); // WA_Delete_on_close is defined

	// don't do below.
//		connect(receiverThread, SIGNAL(finished()), receiverThread, SLOT(deleteLater()));


//		if (!scheduler) {
		// This is queued connection because receiverThread reside outside of the main thread
		if ( ! connect(receiverThread, SIGNAL(frameReceived()), this, SLOT(scheduleUpdate())) ) {
			qCritical("%s::%s() : Failed to connect frameReceived() signal and scheduleUpdate() slot", metaObject()->className(), __FUNCTION__);
			return -1;
		}
		else {
//				qDebug("%s::%s() : frameReceived() -> scheduleUpdate() are connected", metaObject()->className(), __FUNCTION__);
		}
//		}
	receiverThread->start();


	/*
	QFuture<void> future = QtConcurrent::run(this, &SageStreamWidget::pixelRecvThread);
	futureWatcher.setFuture(future);
	connect(&futureWatcher, SIGNAL(finished()), this, SLOT(close()));
	connect(this, SIGNAL(pauseThread()), &futureWatcher, SLOT(pause()));
	connect(this, SIGNAL(resumeThread()), &futureWatcher, SLOT(resume()));
	*/
	setPos(initrect.x(), initrect.y());

	return 0;
}
















//void SageStreamWidget::pixelRecvThread() {

//	/*
//	 * Initially store current affinity settings of this thread using NUMA API
//	 */
//	if (_affInfo)
//		_affInfo->figureOutCurrentAffinity();


//	/*
//	QThread *thread = this->thread();
//	// The operating system will schedule the thread according to the priority parameter. The effect of the priority parameter is dependent on the operating system's scheduling policy. In particular, the priority will be ignored on systems that do not support thread priorities (such as on Linux, see http://linux.die.net/man/2/sched_setscheduler for more details).
//	qDebug("SagePixelReceiver::%s() : priority %d", __FUNCTION__, thread->priority());


//	pthread_setschedparam();
//	*/

//	struct rusage ru_start, ru_end;


//	int byteCount = _appInfo->getFrameSize();
////	int byteCount = pBuffer->width() * pBuffer->height() * 3;
////	unsigned char *buffer = (unsigned char *)malloc(sizeof(unsigned char) * byteCount);
////	memset(buffer, 127, byteCount);


//	Q_ASSERT(doubleBuffer);
//	unsigned char *bufptr = static_cast<QImage *>(doubleBuffer->getFrontBuffer())->bits();


////	QMutex mutex;

//	while( ! threadEnd() ) {
//		if ( _affInfo ) {
//			if ( _affInfo->isChanged() ) {
//				// apply new affinity;
//				//			qDebug("SagePixelReceiver::%s() : applying new affinity parameters", __FUNCTION__);
//				_affInfo->applyNewParameters(); // use NUMA lib

//				// update info in _affInfo
//				// this function must be called in this thread
//				_affInfo->figureOutCurrentAffinity(); // use NUMA lib
//			}
//			else {
//#if defined(Q_OS_LINUX)
//				/* this is called too many times */
//				// if cpu has changed, AffinityInfo::cpuOfMineChanged() will be emitted
//				// which is connected to ResourceMonitor::update_affInfo()
//				_affInfo->setCpuOfMine( sched_getcpu() );
//#endif
//			}
//		}


//		if(_perfMon) {
//			_perfMon->getRecvTimer().start(); //QTime::start()

//	#if defined(Q_OS_LINUX)
//			getrusage(RUSAGE_THREAD, &ru_start); // that of calling thread. Linux specific
//	#elif defined(Q_OS_MAC)
//			getrusage(RUSAGE_SELF, &ru_start);
//	#endif
//		}

//		/**
//		  this must happen after _affInfo->applyNewParameters()
//		  **/
//		if (scheduler) {
//			mutex.lock();
//			waitCond.wait(&mutex);
//		}


//		ssize_t totalread = 0;
//		ssize_t read = 0;

////		gettimeofday(&s, 0);
////		ToPixmap->acquire();
////		gettimeofday(&e, 0);
////		qreal el = ((double)e.tv_sec + (double)e.tv_usec * 0.000001) - ((double)s.tv_sec+(double)s.tv_usec*0.000001);
////		qDebug() << "acquire: " << el * 1000.0 << " msec";

////		mutex->lock();
////		unsigned char *bufptr = (imageArray[*arrayIndex])->bits();

////		if ( !image ||  !(image->bits()) ) {
////			qDebug() << "QImage is null";
////			break;
////		}
////		unsigned char *bufptr = image->bits(); // will detach().. : deep copy


////		int byteCount = pBuffer->width() * pBuffer->height() * 3;
////		pBuffer->detach();

////		qDebug() << (QTime::currentTime()).toString("mm:ss.zzz") << " recevier start receiving " << frameCounter + 1;

//		// PRODUCER
//		while (totalread < byteCount ) {
//			// If remaining byte is smaller than user buffer length (which is groupSize)
//			if ( byteCount-totalread < _appInfo->getNetworkUserBufferLength() ) {
//				read = recv(socket, bufptr, byteCount-totalread , MSG_WAITALL);
//			}
//			// otherwise, always read groupSize bytes
//			else {
//				read = recv(socket, bufptr, _appInfo->getNetworkUserBufferLength(), MSG_WAITALL);
//			}
//			if ( read == -1 ) {
//				qCritical("SagePixelReceiver::%s() : error while reading.", __FUNCTION__);
//				break;
//			}
//			else if ( read == 0 ) {
//				qDebug("SagePixelReceiver::%s() : sender disconnected", __FUNCTION__);
//				break;
//			}

//			// advance pointer
//			bufptr += read;
//			totalread += read;
//		}
//		if ( totalread < byteCount ) break;
//		read = totalread;

////		++frameCounter;
////		qDebug() << (QTime::currentTime()).toString("mm:ss.zzz") << " recevier : " << frameCounter << " received";

//		// _ts_nextframe, _deadline_missed are also updated in this function
////		qDebug() << _perfMon->set_ts_currframe() * 1000.0 << " ms";
//		_perfMon->set_ts_currframe();


//		if (_perfMon) {
//#if defined(Q_OS_LINUX)
//			getrusage(RUSAGE_THREAD, &ru_end);
//#elif defined(Q_OS_MAC)
//			getrusage(RUSAGE_SELF, &ru_end);
//#endif
//			// calculate
//			_perfMon->updateRecvLatency(read, ru_start, ru_end); // QTimer::restart()
////			ru_start = ru_end;

////			qDebug() << "SageStreamWidget" << _perfMon->getRecvFpsVariance();
//		}


//		if ( doubleBuffer ) {

//			// will wait until consumer (SageStreamWidget) consumes the data
//			doubleBuffer->swapBuffer();
////			qDebug("%s() : swapBuffer returned", __FUNCTION__);

////			emit this->frameReceived(); // Queued Connection. Will trigger SageStreamWidget::updateWidget()
////			qDebug("%s() : signal emitted", __FUNCTION__);

//			if ( ! QMetaObject::invokeMethod(this, "updateWidget", Qt::QueuedConnection) ) {
//				qCritical("%s::%s() : invoke updateWidget() failed", metaObject()->className(), __FUNCTION__);
//			}
////			static_cast<SageStreamWidget *>(this)->updateWidget();


//			// getFrontBuffer() will return immediately. There's no mutex waiting in this function
//			bufptr = static_cast<QImage *>(doubleBuffer->getFrontBuffer())->bits(); // bits() will detach
////			qDebug("%s() : grabbed front buffer", __FUNCTION__);
//		}
//		else {
//			break;
//		}


//		if(scheduler) {
//			mutex.unlock();
//		}
//	}


//	/* pixel receiving thread exit */
//	qDebug("SageStreamWidget::%s() : thread exit", __FUNCTION__);
//}

int SageStreamWidget::getPixelSize(sagePixFmt type)
{
   int bytesPerPixel = 0;
   switch(type) {
	  case PIXFMT_555:
	  case PIXFMT_555_INV:
	  case PIXFMT_565:
	  case PIXFMT_565_INV:
	  case PIXFMT_YUV:
		 bytesPerPixel = 2;
		 break;
	  case PIXFMT_888:
	  case PIXFMT_888_INV:
		 bytesPerPixel = 3;
		 break;

	  case PIXFMT_8888:
	  case PIXFMT_8888_INV:
		 bytesPerPixel = 4;
		 break;

	  case PIXFMT_DXT:
		 bytesPerPixel = 8;
		 break;

	  default:
		 bytesPerPixel = 3;
		 break;
   }
   return bytesPerPixel;
}



