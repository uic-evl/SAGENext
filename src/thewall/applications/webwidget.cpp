#include "webwidget.h"
#include "base/appinfo.h"

#include <QtGui>
#include <QtWebKit>
//#include <QWebPage>

SN_WebWidget::SN_WebWidget(const quint64 gaid, const QSettings *setting, QGraphicsItem *parent /*0*/, Qt::WindowFlags wFlags/* Qt::Window */)
    : SN_BaseWidget(gaid, setting, parent, wFlags)
    , linearLayout(0)
    , gwebview(0)
    , urlbox(0)
    , urlboxproxy(0)
{

	qDebug() << "SN_WebWidget() : QtWebKit version" << QTWEBKIT_VERSION_STR;
	setWidgetType(SN_BaseWidget::Widget_Web);

	_appInfo->setMediaType(SAGENext::MEDIA_TYPE_WEBURL);

        /**
          When you create a child item, it is by default top most item (stacking on top of the parent)
          So, all the mouse/keyboard events are grabbed by the child item (because it's top most).

          To prevent this, you can call
                  TheChildItem->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
          and then reimplement event handlers of this item.


          OR just hide() the child item (QGraphicsItem::hide()). Refer BaseWidget's infoTextItem


          OR
          You can intercept all the events before it even gets to the childs by doing
// TheChildItem->installSceneEventFilter(this); // TheChildItem's events are filtered by this->sceneEventFilter(). This is goold if you want to filter specific child item only.



          OR
// this->setFileterChildEvents(true); // This object(WebWidget) will filter *ALL* events for all its children, so childs don't need to install event filter (installSceneEventFilter()) explicitly
         **/
	setFiltersChildEvents(true);

	// shared pointer won't be able to interact with window frames
//	setWindowFrameMargins(0, 0, 0, 0); // left, top, right, bottom

	// use this instead
//	setContentsMargins(0, 0, 0, 0);


//	connect(&futureWatcher, SIGNAL(finished()), this, SLOT(pageLoaded()));


	/* URL box with QLineEdit widget */
	urlbox = new QLineEdit("http://");
	QFont f;
	f.setPointSize(18);
	urlbox->setFont(f);
	connect(urlbox, SIGNAL(returnPressed()), this, SLOT(setUrlFromLineEdit()));


	/* Corresponding proxy widget for the URL box */
	urlboxproxy = new QGraphicsProxyWidget(); // this is bad for graphics perfomance. But it's the only way
	//proxyWidget->setFlag(QGraphicsItem::ItemStacksBehindParent, true);

	// urlboxproxy takes ownership of urlbox
	urlboxproxy->setWidget(urlbox); // widget(urlbox) must be top-level widget whose parent is 0
	urlboxproxy->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred, QSizePolicy::LineEdit);



	/* webkit related */
	QWebSettings *ws = QWebSettings::globalSettings();

	/*
	  **
	  I recommend using Java plugin from the Java SE @ oracle
	  instead of IcedTea plugin from OpenJDK that comes with your OS.

	  Use $JAVAHOME/jre/lib/amd64/libnpjp2.so
	  and make symbolic link in plugin directory or a directory pointed by $QTWEBKIT_PLUGIN_PATH env variable.
	  e.g.
	  cd /usr/lib64/browser-plugins
	  ln -s $JAVAHOME/jre/lib/amd64/lipnpjp2.so javaplugin.so
	  ****
	  **/
	ws->setAttribute(QWebSettings::JavaEnabled, true);
	ws->setAttribute(QWebSettings::JavascriptEnabled, true);
	ws->setAttribute(QWebSettings::PluginsEnabled, true);
	ws->setAttribute(QWebSettings::JavascriptCanOpenWindows, true);

	ws->setAttribute(QWebSettings::LocalContentCanAccessFileUrls, true);
	ws->setAttribute(QWebSettings::LocalContentCanAccessRemoteUrls, true);

	ws->setAttribute(QWebSettings::AcceleratedCompositingEnabled, true);
#if QT_VERSION >= 0x040800
	qDebug() << "SN_WebWidget() : Using Qt 4.8.0.. Enabling WebGL";
	ws->setAttribute(QWebSettings::WebGLEnabled, true);
#endif

//	setCacheMode(QGraphicsItem::ItemCoordinateCache);
//	QPixmapCache::setCacheLimit(1024000);

//	setFlag(QGraphicsItem::ItemHasNoContents);

	/**
      gwebview needs to be able to receive mouse events because users want to click links on web pages
      That's why webwidget filter childs' event instead of stacking children behind the parent
     */
	gwebview = new QGraphicsWebView; // it is now the top most item unless ItemStacksBehindParent is true
//	gwebview->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
//	gwebview->installSceneEventFilter(this);
//	webPage = gwebview->page();
//	gwebview->setCacheMode(QGraphicsItem::ItemCoordinateCache);


	gwebview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding, QSizePolicy::Frame);
	connect(gwebview, SIGNAL(urlChanged(QUrl)), this, SLOT(urlChanged(QUrl)));


	linearLayout = new QGraphicsLinearLayout(Qt::Vertical, this);
	linearLayout->setSpacing(4);
	linearLayout->setContentsMargins(20,20,20,40);

	// The layout takes ownership of these items.
	linearLayout->addItem(urlboxproxy);
	linearLayout->addItem(gwebview);

	// Any existing layout is deleted before the new layuout is assigned, and this widget will take ownership of the layout
	setLayout(linearLayout);


	resize(1280, 1024);
			
	/* This means nothing. Because wheel event won't be handled by BaseGraphicsWidget, and this widget redefines resizeEvent */
	//	setNativeSize(810,650); // w/o frame
}


SN_WebWidget::~SN_WebWidget() {
//	if ( gwebview ) delete gwebview;
//	delete urlbox;

	/** I shouldn't do below. Below objects are deleted by Qt **/
//	if (linearLayout) delete linearLayout;
//	if (gwebview) delete gwebview;
//	if (urlboxproxy) delete urlboxproxy;
//	if (urlbox) delete urlbox;

//	qDebug("WebWidget::%s()", __FUNCTION__);
}



/**
  Because what this function does is intercepting events delivered to its children, the events must be delivered to the children.
  Therefore, this function won't work if childs are stacked behind the parent (WebWidget)
 */
bool SN_WebWidget::sceneEventFilter(QGraphicsItem *watched, QEvent *event) {

	//
	// I'll only filter events delivered to the gwebview
	// So this won't be executed if the gwebview is set to be stacked behind the parent
	//
	if (watched == gwebview ) {
		//	qDebug("WebWidget::%s() : gwebview is going to receive event %d", __FUNCTION__, event->type());

		if ( event->type() == QEvent::GraphicsSceneContextMenu) {
			QGraphicsSceneContextMenuEvent *e = static_cast<QGraphicsSceneContextMenuEvent *>(event);

			SN_BaseWidget::contextMenuEvent(e);

			//
			// Don't forward this event to the gwebview. (meaning that SN_WebWidget is going to filter this event)
			//
			// Right Click is used to toggle 'selection' of the application
			//
			return true;
		}
/*

  I don't need below else if block as long as I'm not going to filter

		else if (event->type() == QEvent::GraphicsSceneMousePress) {
//			qDebug() << "gwebview received pressEvent";

			//
			// First, deliver this event to BaseWidget to keep the behaviors defined in the BaseWidget
			//
			SN_BaseWidget::mousePressEvent((QGraphicsSceneMouseEvent *)event);

			//
			// The press event should reach to gwebview otherwise no mouse click can be done on gwebview.
			// So, return false to forward this event to the gwebview. (meaning that SN_WebWidget is not going to filter this event)
			//
			return false;
		}
		*/

		else if (event->type() == QEvent::GraphicsSceneMouseDoubleClick) {
//			qDebug() << "webwidget received doubleClick event";
			SN_BaseWidget::mouseDoubleClickEvent((QGraphicsSceneMouseEvent *)event);

			//
			// gwebview will not handle doubleclick
			// The WebWidget should handle this.
			//
			return true;
		}
		/*
 // resizing of the WebWidget is done by window frame. Notice that windowFlag of WebWidget is Qt::Window
 else if (event->type() == QEvent::GraphicsSceneWheel) {
   return false; // don't filter so that gwebview can grab this event
 }
  // release event should reach gwebview to fire mouse clicking
 else if (event->type() == QEvent::GraphicsSceneMouseRelease) {
   return false; // Don't filter this event. release event should reach to gwebview
 }
 else if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
   return false; // Don't filter this event. key press should be handled by gwebview
 }
	     */
	}

	if (watched == urlboxproxy) {
		if (event->type() == QEvent::GraphicsSceneMousePress)
			SN_BaseWidget::setTopmost();
	}

	return false; // All other events won't be filtered by WebWidget
}

void SN_WebWidget::handlePointerPress(SN_PolygonArrowPointer *pointer, const QPointF &point, Qt::MouseButton btn) {
	SN_BaseWidget::handlePointerPress(pointer, point, btn); // keep the base implementation
}

void SN_WebWidget::handlePointerDrag(SN_PolygonArrowPointer *pointer, const QPointF &point, qreal pointerDeltaX, qreal pointerDeltaY, Qt::MouseButton button, Qt::KeyboardModifier modifier) {

	if (gwebview && gwebview->geometry().contains(point)) {
		//
		// I need to generate system mouse move event and send it to QGraphicsView (viewport of me)
		//

		Q_ASSERT(scene());
		QGraphicsView *view = 0;
		QPointF scenePos = mapToScene(point);

		foreach(QGraphicsView *v, scene()->views()) {
			if (!v) continue;

			// geometry of widget relative to its parent
			//v->geometry();

			// internal geometry of widget
			//v->rect();

			if ( v->rect().contains( v->mapFromScene(scenePos) ) ) {
				// mouse click position is within this view's bounding rectangle
				view = v;
				break;
			}
		}
		Q_ASSERT(view);


		/*************************

		  Note that intuitive way of implementing system mouse dragging would be
		  1. reimplement SN_BaseWidget::handlePointerPress() to generate system mouse press event thereby gwebview is the mouseGrabber
		  2. reimplement SN_BaseWidget::handlePointerDrag() to generate system mouse move event
		  3. reimplement SN_BaseWidget::handlePointerRelease() to generate system mouse release event thereby gwebview ungrab mouse

		  But doing #1 makes SN_PolygonArrowPointer::pointerClick() fails when user does 'mouse click' (PRESS followed by RELEASE).

		  For instance,
		  If #1 is implemented, and user executes 'mouse click' then mousePressEvent is fired (in SN_WebWidget::handlePointerPress()) upon receiving PRESS
		  And the second mousePressEvent will be fired upon receiving following RELEASE (in SN_PolygonArrowPointer::pointerClick())
		  This second mousePressEvent will fail and pointerClick() will return unsuccessfully.

		  So the workaround is either simply not reimplementing SN_BaseWidget::handlePointerPress() or not generating system mouse press in the function.
		  This is why mousePressEvent has to be placed in here.

		  ************/
		if (scene()->mouseGrabberItem() != gwebview) {
			QMouseEvent press(QEvent::MouseButtonPress, view->mapFromScene(scenePos), button, button | Qt::NoButton, Qt::NoModifier);
			if ( ! QApplication::sendEvent(view->viewport(), &press)) {
				qDebug() << "SN_WebWidget::handlePointerDrag() : failed to send press event";
			}
			else {
				// Now I'm mouse grabber
				qDebug() << "Pressed, mouse grabber is" << scene()->mouseGrabberItem();
			}
		}

		QMouseEvent moveEvent(QEvent::MouseMove, view->mapFromScene(scenePos), button, button | Qt::NoButton, modifier);

		// sendEvent doesn't delete event object, so event should be created in stack space
		// also sendEvent blocks

		if ( ! QApplication::sendEvent(view->viewport(), &moveEvent) ) {
			qDebug("SN_WebWidget::%s() : sendEvent MouseMove on (%.1f,%.1f) failed", __FUNCTION__, scenePos.x(), scenePos.y());
		}
		else {
			//qDebug() << "SN_WebWidget::handlePointerDrag() : moveEvent sent";
		}
	}

	else {
		SN_BaseWidget::handlePointerDrag(pointer, point, pointerDeltaX, pointerDeltaY, button, modifier);
	}
}

void SN_WebWidget::handlePointerRelease(SN_PolygonArrowPointer *, const QPointF &point, Qt::MouseButton btn) {

	if (gwebview && gwebview->geometry().contains(point)) {
		Q_ASSERT(scene());
		QGraphicsView *view = 0;
		QPointF scenePos = mapToScene(point);

		foreach(QGraphicsView *v, scene()->views()) {
			if (!v) continue;

			// geometry of widget relative to its parent
			//        v->geometry();

			// internal geometry of widget
			//        v->rect();

			if ( v->rect().contains( v->mapFromScene(scenePos) ) ) {
				// mouse click position is within this view's bounding rectangle
				view = v;
				break;
			}
		}
		Q_ASSERT(view);

		QMouseEvent release(QEvent::MouseButtonRelease, view->mapFromScene(scenePos), btn, btn | Qt::NoButton, Qt::NoModifier);
		if ( ! QApplication::sendEvent(view->viewport(), &release)) {
			qDebug() << "SN_WebWidget::handlePointerRelease() failed";
		}
		else {
			// Now I'm mouse grabber
//			qDebug() << "Released, mouse grabber is" << scene()->mouseGrabberItem();
		}

		gwebview->ungrabMouse();
	}
}



/*!
  pointer wheel will generate real system wheel event and post it to the QGraphicsView
  So, if pointer wheel event is occurring on SN_WebWidget then gwebview (which is child and stacked above its parent)
  will receive the system wheel event generated from SN_PolygonArrowPointer
 */
/*
void SN_WebWidget::wheelEvent(QGraphicsSceneWheelEvent *event) {
        // WebWidget will not respond to this event.
        // shouldn't do anything..
	event->ignore();
}
*/


void SN_WebWidget::pageLoaded() {
	//	gwebview->setContent( gwebview->page()->mainFrame()->toHtml().toAscii() );

	gwebview->setPage( webPage );
	qDebug() << gwebview->page()->mainFrame()->toHtml().toAscii();

	update(); // schedule paint()
}

void SN_WebWidget::setUrl(const QString &u) {
	QUrl url;
	if ( u.startsWith("http://", Qt::CaseInsensitive)
	     || u.startsWith("https://", Qt::CaseInsensitive)
	     || u.startsWith("file://", Qt::CaseInsensitive))
	{
		url.setUrl(u, QUrl::TolerantMode);
	}
	else {
		QString http("http://");
		url.setUrl(http.append(u), QUrl::TolerantMode);
	}

	if (!url.isEmpty() && url.isValid()) {

		appInfo()->setWebUrl( url );
		//	gwebview->setUrl(url);
		gwebview->load(url);

		_appInfo->setWebUrl(url);


		/**
          QObject: Cannot create children for a parent that is in a different thread.
          (Parent is QGraphicsWebView(0x87ddd0), parent's thread is QThread(0x6877a0), current thread is QThread(0x8c5bd0)
         */
		//	QFuture<void> future = QtConcurrent::run(gwebview, &QGraphicsWebView::load, url);
	}
	else {
		qWarning("WebWidget::%s() : URL [%s] is not valid", __FUNCTION__, qPrintable(url.toString()) );
	}
}

void SN_WebWidget::setUrlFromLineEdit() {
	setUrl(urlbox->text());
}

void SN_WebWidget::urlChanged(const QUrl &url) {
	urlbox->setText( url.toString());
}



