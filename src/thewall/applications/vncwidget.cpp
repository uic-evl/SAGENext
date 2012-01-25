#include "vncwidget.h"

#include "base/perfmonitor.h"
#include "base/appinfo.h"
#include "../common/commonitem.h"

#include <QtGui>

#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <QGLPixelBuffer>


rfbBool SN_VNCClientWidget::got_data = FALSE;
QString SN_VNCClientWidget::username = "";
QString SN_VNCClientWidget::vncpasswd = "evl123";

SN_VNCClientWidget::SN_VNCClientWidget(quint64 globalappid, const QString senderIP, int display, const QString username, const QString passwd, int frate, const QSettings *s, QGraphicsItem *parent, Qt::WindowFlags wflags)
	: SN_RailawareWidget(globalappid, s, parent, wflags)
	, vncclient(0)
	, serverPort(5900)
	, _image(0)
    , _texid(-1)
    , _usePbo(true)
    , _pboBufIdx(0)
	, _end(false)
	, _framerate(frate)
    , _buttonMask(0)
//    , _glbuffers(0)
//    , _myGlWidget(0)
//    , _viewportWidget(0)
//    , _useGLBuffer(false)
{
	setWidgetType(SN_BaseWidget::Widget_RealTime);

	_appInfo->setMediaType(SAGENext::MEDIA_TYPE_VNC);
	
	if ( username == "user" )
		SN_VNCClientWidget::username = "";
	else
		SN_VNCClientWidget::username = username;
	SN_VNCClientWidget::vncpasswd = passwd;
	
	_appInfo->setVncUsername(SN_VNCClientWidget::username);
	_appInfo->setVncPassword(SN_VNCClientWidget::vncpasswd);

//	qDebug() << "vnc widget constructor " <<  username << passwd << VNCClientWidget::username << VNCClientWidget::vncpasswd;

	// 8 bit/sample
	// 3 samples/pixel
	// 4 Byte/pixel
	vncclient = rfbGetClient(8, 3, 4);

	vncclient->canHandleNewFBSize = false;
	vncclient->appData.useRemoteCursor = true;
	vncclient->MallocFrameBuffer = SN_VNCClientWidget::resize_func;
	vncclient->GotFrameBufferUpdate = SN_VNCClientWidget::update_func;
	vncclient->HandleCursorPos = SN_VNCClientWidget::position_func;
	vncclient->GetPassword = SN_VNCClientWidget::password_func;

	serverAddr.setAddress(senderIP);
	_appInfo->setSrcAddr(senderIP);
	
	

	if (!SN_VNCClientWidget::username.isEmpty()  &&  !SN_VNCClientWidget::vncpasswd.isEmpty()) {
		vncclient->GetCredential = SN_VNCClientWidget::getCredential;
		const uint32_t authSchemes[] = {rfbARD, rfbVncAuth, rfbNoAuth};
		int numSchemes = 3;
		SetClientAuthSchemes(vncclient, authSchemes, numSchemes);
	}
	else {
		const uint32_t authSchemes[] = {rfbVncAuth, rfbNoAuth};
		int numSchemes = 2;
		SetClientAuthSchemes(vncclient, authSchemes, numSchemes);
	}

	vncclient->FinishedFrameBufferUpdate = SN_VNCClientWidget::frame_func;

	int margc = 2;
	char *margv[2];
	margv[0] = strdup("vnc");
	margv[1] = (char *)malloc(256);
	memset(margv[1], 0, 256);
	sprintf(margv[1], "%s:%d", qPrintable(serverAddr.toString()), display);

	if ( ! rfbInitClient(vncclient, &margc, margv) ) {
		qCritical() << "rfbInitClient() error init";
		deleteLater();
	}

	if (vncclient->serverPort == -1 )
		vncclient->vncRec->doNotSleep = true;

	_image = new QImage(vncclient->width, vncclient->height, QImage::Format_RGB32);

	/**
	  Don't forget to call resize() once you know the size of image you're displaying.
	  Also BaseWidget::resizeEvent() will call setTransformOriginPoint();
     */
	resize(vncclient->width, vncclient->height);
	_appInfo->setFrameSize(vncclient->width, vncclient->height, 32);


	setWidgetType(SN_BaseWidget::Widget_RealTime);
	if (_perfMon) {
		_perfMon->setExpectedFps( (qreal)_framerate );
		_perfMon->setAdjustedFps( (qreal)_framerate );
	}

	_usePbo = s->value("graphics/openglpbo", false).toBool();

	QTimer::singleShot(10, this, SLOT(startThread()));
}


void SN_VNCClientWidget::startThread() {
	/***********
	//
	// QGLContext is accessible from this widget ONLY after the constructor returns
	//
	QGLContext *context = const_cast<QGLContext *>(QGLContext::currentContext());
	if (context && context->isValid())
		 _viewportWidget = static_cast<QGLWidget *>(context->device());

	if (_viewportWidget && _viewportWidget->paintEngine()->type() == QPaintEngine::OpenGL2) {
//		qDebug() << "\n\nOpenGL viewport available\n\n";

		_useGLBuffer = true;
	}
	else {
		// this is going to be the pixel buffer when OpenGL isn't available
		_image = new QImage(vncclient->width, vncclient->height, QImage::Format_RGB32);
		_useGLBuffer = false;
	}
	************/


	if (_useOpenGL) {

		glGenTextures(1, &_texid);
		glBindTexture(GL_TEXTURE_2D, _texid);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size().width(), size().height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, (void *)0);
		glBindTexture(GL_TEXTURE_2D, 0);

		_pboIds[0] = -1;
		_pboIds[1] = -1;

		if (_usePbo) {

			qDebug() << "VNCWidget : OpenGL pbuffer extension is present. Using PBO";
			glGenBuffersARB(2, _pboIds);

			glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, _pboIds[0]);
			glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, _appInfo->frameSizeInByte(), 0, GL_STREAM_DRAW_ARB);

			glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, _pboIds[1]);
			glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, _appInfo->frameSizeInByte(), 0, GL_STREAM_DRAW_ARB);

			glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
		}
	}

	// starting thread.
	future = QtConcurrent::run(this, &SN_VNCClientWidget::receivingThread);
}

SN_VNCClientWidget::~SN_VNCClientWidget() {
	_end = true;
	future.cancel();
	future.waitForFinished();

	if (vncclient) {
		rfbClientCleanup(vncclient);
	}

	if (_image) delete _image;

	if (_useOpenGL && glIsTexture(_texid)) {
		glDeleteTextures(1, &_texid);
	}

	if (_usePbo) {
		glDeleteBuffersARB(2, _pboIds);
	}

	/*
	if (_glbuffers) {
		for (int i=0; i<2; i++) {
			_glbuffers[i]->release();
			_glbuffers[i]->destroy();
			delete _glbuffers[i];
		}
		free(_glbuffers);
	}

	if (_myGlWidget) {
		delete _myGlWidget;
	}
	*/
}

/*
int SN_VNCClientWidget::initGLBuffers(int bytecount) {
	if (!_viewportWidget) return -1;

	if (_viewportWidget->paintEngine()->type() != QPaintEngine::OpenGL2) {
		qWarning("%s::%s() : OpenGL2 paintEngine is not available");
		return -1;
	}

	_myGlWidget = new QGLWidget(QGLFormat::defaultFormat(), 0, _viewportWidget);
	_myGlWidget->makeCurrent();

	//
	// init glbuffer (at the GLServer)
	//
	_glbuffers = (QGLBuffer **)malloc(sizeof(QGLBuffer *) * 2);
	for (int i=0; i<2; i++) {
		_glbuffers[i] = new QGLBuffer(QGLBuffer::PixelUnpackBuffer);
		_glbuffers[i]->setUsagePattern(QGLBuffer::StreamDraw);

		//
		// glGenBufferARB
		//
		if (!_glbuffers[i]->create()) {
			qDebug() << "glbuffer->create() failed";
			return -1;
		}

		//
		// glBindBufferARB
		//
		if (!_glbuffers[i]->bind()) {
			qDebug() << "glbuffer->bind() failed";
			return -1;
		}

		//
		// glBufferDataARB
		//
		_glbuffers[i]->allocate(bytecount);

		_glbuffers[i]->release();
	}

	return 0;
}
*/

void SN_VNCClientWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *w) {
	if (_perfMon) {
		_perfMon->getDrawTimer().start();
	}

	//	painter->setRenderHint(QPainter::Antialiasing);
	//	painter->setRenderHint(QPainter::HighQualityAntialiasing);
	painter->setRenderHint(QPainter::SmoothPixmapTransform); // important -> this will make text smoother


	if (_useOpenGL && painter->paintEngine()->type() == QPaintEngine::OpenGL2 /* || painter->paintEngine()->type() == QPaintEngine::OpenGL */) {
		painter->beginNativePainting();

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, _texid);

		glBegin(GL_QUADS);
		//
		// below is same with QGLContext::InvertedYBindOption
		//
		glTexCoord2f(0.0, 1.0); glVertex2f(0, size().height());
		glTexCoord2f(1.0, 1.0); glVertex2f(size().width(), size().height());
		glTexCoord2f(1.0, 0.0); glVertex2f(size().width(), 0);
		glTexCoord2f(0.0, 0.0); glVertex2f(0, 0);

		//
		// below is normal (In OpenGL, 0,0 is bottom-left, In Qt, 0,0 is top-left)
		//
		//			glTexCoord2f(0.0, 1.0); glVertex2f(0, 0);
		//			glTexCoord2f(1.0, 1.0); glVertex2f(_imagePointer->width(), 0);
		//			glTexCoord2f(1.0, 0.0); glVertex2f(_imagePointer->width(), _imagePointer->height());
		//			glTexCoord2f(0.0, 0.0); glVertex2f(0, _imagePointer->height());

		glEnd();

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);

		painter->endNativePainting();
	}
	else {
		if (!_pixmapForDrawing.isNull()) {
			painter->drawPixmap(0, 0, _pixmapForDrawing);
		}
	}

	SN_BaseWidget::paint(painter, o, w);

	if (_perfMon)
		_perfMon->updateDrawLatency(); // drawTimer.elapsed() will be called.
}

void SN_VNCClientWidget::scheduleUpdate() {
	/*
#if QT_VERSION < 0x040700
    pixmap = QPixmap::fromImage(*_image);
    if (pixmap.isNull()) {
        qDebug("SageStreamWidget::scheduleUpdate() : QPixmap::fromImage() error");
#else
    if (! _pixmap.convertFromImage(*_image, Qt::AutoColor | Qt::OrderedDither) ) {
        qDebug("VNCClientWidget::%s() : pixmap->convertFromImage() error", __FUNCTION__);
#endif
*/


	_perfMon->getConvTimer().start();

	if (_useOpenGL && !_usePbo) {
		// QGLContext::InvertedYBindOption Because In OpenGL 0,0 is bottom left, In Qt 0,0 is top left
		//
		// Below is awfully slow. Probably because QImage::scaled() inside qgl.cpp to convert image size near power of two
		// _textureid = glContext->bindTexture(constImageRef->convertToFormat(QImage::Format_RGB32), GL_TEXTURE_2D, QGLContext::InvertedYBindOption);

		//glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		//glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, _texid);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		//
		// Note that it's QImage::Format_RGB888 we're getting from SAGE app
		//
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, vncclient->width, vncclient->height, GL_RGBA, GL_UNSIGNED_BYTE, vncclient->frameBuffer);
		//GLenum error = glGetError();
		//if(error != GL_NO_ERROR) {
		//		qCritical("texture upload failed. error code 0x%x\n", error);
		//}

		glBindTexture(GL_TEXTURE_2D, 0);
	}
	else if (_useOpenGL && _usePbo) {
		_pboBufIdx = (_pboBufIdx + 1) % 2;
		int nextbufidx = (_pboBufIdx + 1) % 2;

		glBindTexture(GL_TEXTURE_2D, _texid);
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, _pboIds[_pboBufIdx]);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, vncclient->width, vncclient->height, GL_RGBA, GL_UNSIGNED_BYTE, 0);



		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, _pboIds[nextbufidx]);
		glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, _appInfo->frameSizeInByte(), 0, GL_STREAM_DRAW_ARB);
		GLubyte *ptr = (GLubyte *)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
		if (ptr) {
			::memcpy(ptr, vncclient->frameBuffer, _appInfo->frameSizeInByte());
			glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
		}
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	}
	else {
		if (!_image || _image->isNull()) {
			return;
	    }
		const QImage *constImageRef = _image;
		_pixmapForDrawing.convertFromImage(*constImageRef, Qt::ColorOnly | Qt::ThresholdDither);
	}

	_perfMon->updateConvDelay();

	// Schedules a redraw. This is not an immediate paint. This actually is postEvent()
	// QGraphicsView will process the event
	update();
}

void SN_VNCClientWidget::receivingThread() {
	struct timeval lats, late;
	struct rusage ru_start, ru_end;

	if(_perfMon) {
		_perfMon->getRecvTimer().start(); //QTime::start()

#if defined(Q_OS_LINUX)
		getrusage(RUSAGE_THREAD, &ru_start); // that of calling thread. Linux specific
#elif defined(Q_OS_MAC)
		getrusage(RUSAGE_SELF, &ru_start);
#endif
	}

/*
	if (_useGLBuffer) {
		if ( initGLBuffers(_appInfo->frameSizeInByte()) != 0 ) {
			_useGLBuffer = false;
			_image = new QImage(vncclient->width, vncclient->height, QImage::Format_RGB32);
			_texid = 0;
			_viewportWidget = 0;
		}
	}
	QGLBuffer *currGLBuf = 0;
	int bufidx = 0;
	unsigned char *bufptr = 0; // I will copy to this buffer
*/

	while (!_end) {
//		if (_perfMon) gettimeofday(&lats, 0);

		// sleep to ensure desired fps
		qint64 now = 0;
        
#if QT_VERSION < 0x040700
        struct timeval tv;
        gettimeofday(&tv, 0);
        now = tv.tv_sec * 1000  +  tv.tv_usec * 0.0001;
#else
        now = QDateTime::currentMSecsSinceEpoch();
#endif
        qint64 time = 0;
		forever {
			if (!vncclient) {
				_end = true;
				break;
			}

            int i = WaitForMessage(vncclient, 100000); // 100 microsecond == 0.1 msec
			if ( i<0 ) {
				rfbClientLog("VNC error. quit\n");
				_end = true;
				break;
			}

			if (i) {
				if(!HandleRFBServerMessage(vncclient)) {
					rfbClientLog("HandleRFBServerMessage quit\n");
					_end = true;
					break;
				}
			}
            
#if QT_VERSION < 0x040700
            struct timeval tv;
            gettimeofday(&tv, 0);
            time = tv.tv_sec * 1000  +  tv.tv_usec * 0.0001;
#else
            time = QDateTime::currentMSecsSinceEpoch();
#endif
            if ( (time-now) >= (1000/_framerate) ) break;
            
		}
		if (_end) break;



		if (_perfMon)
			gettimeofday(&lats, 0);

		unsigned char * vncpixels = (unsigned char *)vncclient->frameBuffer;

		if ( _useOpenGL) {
			// do nothing
		}
		else {
			QRgb *rgbbuffer = (QRgb *)(_image->scanLine(0)); // A RGB32 quadruplet on the format 0xffRRGGBB, equivalent to an unsigned int.
			Q_ASSERT(vncpixels && rgbbuffer);

				/*
	  for (int k =0 ; k<vncclient->width * vncclient->height; k++) {
	   // QImage::Format_RGB32 format : 0xffRRGGBB
	   rgbbuffer[4*k + 3] = 0xff;
	   rgbbuffer[4*k + 2] = vncpixels[ 4*k + 0]; // red
	   rgbbuffer[4*k + 1] = vncpixels[ 4*k + 1]; // green
	   rgbbuffer[4*k + 0] = vncpixels[ 4*k + 2]; // blue

	   // QImage::Format_RGB888
	//			buffer[3*k + 0] = vncpixels[ 4*k + 0];
	//			buffer[3*k + 1] = vncpixels[ 4*k + 1];
	//			buffer[3*k + 2] = vncpixels[ 4*k + 2];
	  }
	  */
			for (int i=0; i<vncclient->width * vncclient->height; i++) {
				rgbbuffer[i] = qRgb(vncpixels[4*i+0], vncpixels[4*i+1], vncpixels[4*i+2]);
			}
		}

		QMetaObject::invokeMethod(this, "scheduleUpdate", Qt::QueuedConnection);

		if (_perfMon) {
			gettimeofday(&late, 0);
#if defined(Q_OS_LINUX)
			getrusage(RUSAGE_THREAD, &ru_end);
#elif defined(Q_OS_MAC)
			getrusage(RUSAGE_SELF, &ru_end);
#endif
			qreal networkrecvdelay = ((double)late.tv_sec + (double)late.tv_usec * 0.000001) - ((double)lats.tv_sec + (double)lats.tv_usec * 0.000001); // second

			// calculate
			_perfMon->updateObservedRecvLatency(vncclient->width * vncclient->height, networkrecvdelay, ru_start, ru_end);

			ru_start = ru_end;
			lats = late;
		}


	} // end of while (_end)

//	if (_myGlWidget)
//		_myGlWidget->doneCurrent();

//	qDebug() << "LibVNCClient receiving thread finished";
//	QMetaObject::invokeMethod(this, "fadeOutClose", Qt::QueuedConnection);
	QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
}



void SN_VNCClientWidget::signal_handler(int signal)
{
	rfbClientLog("Cleaning up.\n");
}

rfbBool SN_VNCClientWidget::resize_func(rfbClient* client)
{
        static rfbBool first = TRUE;
        if(!first) {
                rfbClientLog("I don't know yet how to change resolutions!\n");
                exit(1);
        }
        signal(SIGINT,signal_handler);

        int width=client->width;
        int height=client->height;
        int depth=client->format.bitsPerPixel;

//	depth = depth / 8; // Byte

        client->updateRect.x = client->updateRect.y = 0;
        client->updateRect.w = width; client->updateRect.h = height;

        client->frameBuffer = (uint8_t*)malloc(width*height*depth );
        memset(client->frameBuffer, 0, width*height*depth);

        rfbClientLog("Allocate %d bytes: %d x %d x %d\n", width*height*depth, width,height,depth);
        return TRUE;
}


rfbCredential * SN_VNCClientWidget::getCredential(struct _rfbClient *client, int credentialType) {
	Q_UNUSED(client);
	Q_UNUSED(credentialType);
	rfbCredential *res = (rfbCredential *)malloc(sizeof(rfbCredential));
	if ( ! SN_VNCClientWidget::username.isEmpty())
		res->userCredential.username = strdup(qPrintable(SN_VNCClientWidget::username));
	else
		res->userCredential.username = NULL;
	if (! SN_VNCClientWidget::vncpasswd.isEmpty())
		res->userCredential.password = strdup(qPrintable(SN_VNCClientWidget::vncpasswd));
	else
		res->userCredential.password = NULL;
	return res;
}

void SN_VNCClientWidget::frame_func(rfbClient *)
{
//        rfbClientLog("Received a frame\n");
}

rfbBool SN_VNCClientWidget::position_func(rfbClient *, int x, int y)
{
        Q_UNUSED(x);
        Q_UNUSED(y);

        //rfbClientLog("Received a position for %d,%d\n",x,y);
        return TRUE;
}

char * SN_VNCClientWidget::password_func(rfbClient *)
{
        char *str = (char*)malloc(64);
        memset(str, 0, 64);
        strncpy(str, qPrintable(SN_VNCClientWidget::vncpasswd), 64);
        return str;
}

void SN_VNCClientWidget::update_func(rfbClient* client,int x,int y,int w,int h)
{
        rfbPixelFormat* pf=&client->format;
        int bpp=pf->bitsPerPixel/8;
        int row_stride = client->width*bpp;

        SN_VNCClientWidget::got_data = TRUE;

        //rfbClientLog("Received an update for %d,%d,%d,%d.\n",x,y,w,h);
}

/*
void SN_VNCClientWidget::handlePointerDrag(SN_PolygonArrowPointer *pointer, const QPointF &point, qreal pointerDeltaX, qreal pointerDeltaY, Qt::MouseButton btn, Qt::KeyboardModifier mod) {
	Q_UNUSED(btn);
	Q_UNUSED(mod);



	SendPointerEvent(vncclient, mapFromScene(point));
}


void SN_VNCClientWidget::mousePressEvent(QGraphicsSceneMouseEvent *event) {

	if (event->buttons() & Qt::LeftButton) {
		_buttonMask |= rfbButton1Mask; // 1
	}
	else if (event->buttons() & Qt::RightButton) {
		_buttonMask |= rfbButton3Mask; // 3
	}
	SendPointerEvent(vncclient, event->pos().x(), event->pos().y(), 1);
	_buttonMask &= ~(rfbButton4Mask | rfbButton5Mask);
}

void SN_VNCClientWidget::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {

	if (event->buttons() & Qt::LeftButton) {
		_buttonMask &= ~rfbButton1Mask; // 1
	}
	else if (event->buttons() & Qt::RightButton) {
		_buttonMask &= ~rfbButton3Mask; // 3
	}

	int x,y;

//	event->buttonDownPos(Qt::LeftButton);
//	qDebug() << "releaseEvent" << event->pos();
	x = event->pos().x();
	y = event->pos().y();

	SendPointerEvent(vncclient, x, y, 0);
	SendPointerEvent(vncclient, x, y, 0);
	SendPointerEvent(vncclient, x, y, 0);
	SendPointerEvent(vncclient, x, y, 0);
	SendPointerEvent(vncclient, x, y, 0);
	SendPointerEvent(vncclient, x, y, 0);
	_buttonMask &= ~(rfbButton4Mask | rfbButton5Mask);
}

*/
