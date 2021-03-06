#include "common/sn_commonitem.h"

SN_PixmapButton::SN_PixmapButton(const QString &res, qreal desiredWidth, const QString &label, QGraphicsItem *parent)
    : QGraphicsWidget(parent)
    , _primary(0)
    , _secondary(0)
    , _mousePressFlag(false)
    , _priorityOverride(0)
{
	QPixmap orgPixmap(res);
    if (desiredWidth > 0 && orgPixmap.width() != desiredWidth)
        orgPixmap = orgPixmap.scaledToWidth(desiredWidth);

	_primary = new QGraphicsPixmapItem(orgPixmap, this);

    _init();

    setLabel(label);
}

SN_PixmapButton::SN_PixmapButton(const QPixmap &pixmap, qreal desiredWidth, const QString &label, QGraphicsItem *parent)
    : QGraphicsWidget(parent)
    , _primary(0)
    , _secondary(0)
    , _mousePressFlag(false)
{
    Q_ASSERT(!pixmap.isNull());

	if (desiredWidth > 0 && pixmap.width() != desiredWidth) {
		_primary = new QGraphicsPixmapItem(pixmap.scaledToWidth(desiredWidth), this);
	}
	else {
		_primary = new QGraphicsPixmapItem(pixmap, this);
	}
    _init();

    setLabel(label);
}

SN_PixmapButton::SN_PixmapButton(const QString &res, const QSize &size, const QString &label, QGraphicsItem *parent)
    : QGraphicsWidget(parent)
    , _primary(0)
    , _secondary(0)
    , _mousePressFlag(false)
{
	QPixmap pixmap(res);
    _primary = new QGraphicsPixmapItem(_selectiveRescale(pixmap, size), this);
    _init();
     setLabel(label);
}

SN_PixmapButton::~SN_PixmapButton() {}

QPixmap SN_PixmapButton::_selectiveRescale(const QPixmap &pixmap, const QSize &size) {

    // both width and height is 0
    if (size.isNull()) {
        // do nothing
        return pixmap;
    }
    else {
        // either widht or height is 0
        if (size.isEmpty()) {
            if (size.width() > 0 && pixmap.width() != size.width())
                return pixmap.scaledToWidth(size.width());

            else if (size.height() > 0 && pixmap.height() != size.height())
                return pixmap.scaledToHeight(size.height());
        }
        else {
            if (size != pixmap.size())
                return pixmap.scaled(size /*, Qt::KeepAspectRatio*/);
        }
    }
    return pixmap;
}

void SN_PixmapButton::_init() {
    _primary->setAcceptedMouseButtons(0);
	_primary->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
    
    setFlag(QGraphicsItem::ItemIgnoresTransformations);
    
    // this widget might want to resize but the pixmap (which is QGraphicsPixmapItem)
    // can't be resized. So this widget is fixed.
    
    // these will override whatever the QGraphicsLayoutItem::sizeHint() returns
    setMinimumSize(_primary->pixmap().size());
    setPreferredSize(_primary->pixmap().size());
    setMaximumSize(_primary->pixmap().size());
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    
	resize(_primary->pixmap().size());
}

void SN_PixmapButton::setLabel(const QString &label) {
    if (_primary && !label.isNull() && !label.isEmpty()) {
		_attachLabel(label, _primary);
	}
}

void SN_PixmapButton::setPrimaryPixmap(const QString &resource, const QSize &size) {
    QPixmap p(resource);
    setPrimaryPixmap(p, size);
}

void SN_PixmapButton::setPrimaryPixmap(const QPixmap &pixmap, const QSize &size) {
    _primary = new QGraphicsPixmapItem(_selectiveRescale(pixmap, size), this);
    _primary->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
    _primary->setAcceptedMouseButtons(0);

    resize(_primary->pixmap().size());

    setMinimumSize(_primary->pixmap().size());
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum, QSizePolicy::Frame);
}

void SN_PixmapButton::setPrimaryPixmap(const QString &resource, int width) {
    QPixmap p(resource);
    setPrimaryPixmap(p, width);
}

void SN_PixmapButton::setPrimaryPixmap(const QPixmap &pixmap, int width/*=0*/) {
    if (pixmap.isNull()) {
        qDebug() << "SN_PixmapButton::setPrimaryPixmap() : null pixmap";
        return;
    }
    if (width > 0 && pixmap.width() != width)
        _primary = new QGraphicsPixmapItem(pixmap.scaledToWidth(width), this);
    else 
        _primary = new QGraphicsPixmapItem(pixmap, this);
    
    _primary->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
    _primary->setAcceptedMouseButtons(0);

    resize(_primary->pixmap().size());

    setMinimumSize(_primary->pixmap().size());
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum, QSizePolicy::Frame);
}

void SN_PixmapButton::setSecondaryPixmap(const QString &resource) {
    QPixmap p(resource);
    setSecondaryPixmap(p);
}



void SN_PixmapButton::setSecondaryPixmap(const QPixmap &pixmap) {
    QPixmap p = pixmap.scaledToWidth(_primary->pixmap().width());
    _secondary = new QGraphicsPixmapItem(p, this);

    //
    // By default, graphics items are stacked by insertion order
    // So this ensures the _primary is shown
    //
//    _secondary->stackBefore(_primary);

    _secondary->setVisible(false);

    _secondary->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
}

void SN_PixmapButton::togglePixmap() {
    if (!_secondary) return;

    if (_secondary->isVisible()) {
        _secondary->setVisible(false);
    }
    else {
        _secondary->setVisible(true);
    }
}

void SN_PixmapButton::_attachLabel(const QString &labeltext, QGraphicsItem *parent) {
	QGraphicsSimpleTextItem *textitem = new QGraphicsSimpleTextItem(labeltext, parent);
//	t->setTransformOriginPoint(t->boundingRect().center());

	QBrush brush(Qt::white);
	textitem->setBrush(brush);

//	QFont f;
//	f.setBold(true);
//	t->setFont(f);

/*
    qreal sizediff = 0;
    if (textitem->boundingRect().width() > size().width()) {
        sizediff = textitem->boundingRect().width() - size().width();
    }
    else if (textitem->boundingRect().height() > size().height()) {
        sizediff = textitem->boundingRect().height() - size().height();
    }
    */

	QPointF center_delat = boundingRect().center() - textitem->mapRectToParent(textitem->boundingRect()).center();
	textitem->moveBy(center_delat.x(), center_delat.y());
}

void SN_PixmapButton::mousePressEvent(QGraphicsSceneMouseEvent *) {
//	setOpacity(1);
	// this isn't perfect solution
	// because flag will still be true if mouse was pressed on this widget and released at somewhere else (outside of this widget)
	// then following mouse release (which was pressed at outside of this widget) will emit the signal because of the true flag
	_mousePressFlag = true;
}
void SN_PixmapButton::mouseReleaseEvent(QGraphicsSceneMouseEvent *) {
//	setOpacity(0.5);
//	qDebug() << "pixmapbutton emitting signal";
    if (_mousePressFlag) {
		emit clicked();
    }
}






SN_ProxyScrollBar::SN_ProxyScrollBar(Qt::Orientation o, QGraphicsItem *parent)
    : SN_ProxyGUIBase(parent)
    , _scrollbar(new QScrollBar(o))
{
    setWidget(_scrollbar);
}
void SN_ProxyScrollBar::drag(const QPointF &pos) {
    qreal handlePosNorm = -1;

    //
    // calculate scroll bar's handle position based on the pointer position on the scrollbar
    //
    if (_scrollbar->orientation() == Qt::Horizontal) {
        handlePosNorm = pos.x() / boundingRect().width(); // 0 ~ 1
    }
    else if (_scrollbar->orientation() == Qt::Vertical) {
        handlePosNorm = pos.y() / boundingRect().height(); // 0 ~ 1
    }

    if (handlePosNorm < 0) return;

    // scroll bar's maximum value calculated from 0
    qreal scrollbar_max_from_zero = _scrollbar->maximum() - _scrollbar->minimum();

    // new value and new handle position
    int newpos = handlePosNorm * scrollbar_max_from_zero;

    // calculate back so that the value is correct in scroll bar's original range
    newpos += _scrollbar->minimum();

    _scrollbar->setValue(newpos); // this will change the sliderPosition !!

    emit valueChanged(newpos);
}




SN_ProxyPushButton::SN_ProxyPushButton(QGraphicsItem *parent)
    : SN_ProxyGUIBase(parent)
    , _button(new QPushButton)
{
    setWidget(_button);
}

SN_ProxyPushButton::SN_ProxyPushButton(const QString &text, QGraphicsItem *parent)
    : SN_ProxyGUIBase(parent)
    , _button(new QPushButton(text))
{
    setWidget(_button);
}



SN_ProxyRadioButton::SN_ProxyRadioButton(QGraphicsItem *parent)
    : SN_ProxyGUIBase(parent)
    , _radiobtn(new QRadioButton)
{
    setWidget(_radiobtn);
}
SN_ProxyRadioButton::SN_ProxyRadioButton(const QString &text, QGraphicsItem *parent)
    : SN_ProxyGUIBase(parent)
    , _radiobtn(new QRadioButton(text))
{
    setWidget(_radiobtn);
}


SN_ProxyLineEdit::SN_ProxyLineEdit(QGraphicsItem *parent, const QString &placeholdertext)
    : SN_ProxyGUIBase(parent)
    , _lineedit(new QLineEdit)
{
    setWidget(_lineedit);
//	_lineedit.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum, QSizePolicy::LineEdit);
    _lineedit->setMinimumHeight(64);

    if (!placeholdertext.isNull()) {
        _lineedit->setPlaceholderText(placeholdertext);
    }

//	QGraphicsLinearLayout *ll = new QGraphicsLinearLayout;
//	ll->addItem(_proxywidget);
//	setLayout(ll);
}

SN_ProxyLineEdit::~SN_ProxyLineEdit() {
}

void SN_ProxyLineEdit::setText(const QString &text, bool emitSignal /* true */) {
    _lineedit->setText(text);
    if (emitSignal) {
        emit textChanged(text);
    }
}










/*!
  TextItem
  */
SN_SimpleTextItem::SN_SimpleTextItem(int ps /* 0 */, const QColor &fontcolor /* black */, const QColor &bgcolor /* gray */, QGraphicsItem *parent /* 0 */)
	: QGraphicsSimpleTextItem(parent)
    , _fontcolor(fontcolor)
    , _bgcolor(bgcolor)
{
	if ( ps > 0 ) {
		QFont f;
		f.setStyleStrategy(QFont::OpenGLCompatible);
		f.setPointSize(ps);
		setFont(f);
	}
	setBrush(_fontcolor);
//	setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
}

void SN_SimpleTextItem::setFontPointSize(int ps) {
	QFont f;
	f.setStyleStrategy(QFont::OpenGLCompatible);
	f.setPointSize(ps);
	setFont(f);
}

void SN_SimpleTextItem::wheelEvent(QGraphicsSceneWheelEvent *event) {
	int numDegrees = event->delta() / 8;
	int numTicks = numDegrees / 15;
	//	qDebug("SwSimpleTextItem::%s() : delta %d numDegrees %d numTicks %d", __FUNCTION__, event->delta(), numDegrees, numTicks);

	qreal s = scale();
	if ( numTicks > 0 ) {
		s += 0.1;
	}
	else {
		s -= 0.1;
	}
	//	prepareGeometryChange();
	setScale(s);
}

void SN_SimpleTextItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	Q_UNUSED(option);
	Q_UNUSED(widget);

    if (_bgcolor.isValid())
        painter->fillRect(boundingRect(), QBrush(_bgcolor));

	QGraphicsSimpleTextItem::paint(painter, option, widget);
}







/*!
  TextWidget
  */
SN_SimpleTextWidget::SN_SimpleTextWidget(int pointSize, const QColor &fontcolor, const QColor &bgcolor, QGraphicsItem *parent)
	: QGraphicsWidget(parent, Qt::Widget)
	, _textItem(0)
{
	setAttribute(Qt::WA_DeleteOnClose, true);

	setFlag(QGraphicsItem::ItemIsSelectable, false);
	setFlag(QGraphicsItem::ItemIsMovable, false);
	setFlag(QGraphicsItem::ItemHasNoContents, true);

	setAcceptedMouseButtons(0);

	_textItem = new SN_SimpleTextItem(pointSize, fontcolor, bgcolor, this);
//	resize(_textItem->boundingRect().size());
}

void SN_SimpleTextWidget::setText(const QString &text) {
	if (_textItem) {
		_textItem->setText(text);
		resize(_textItem->boundingRect().size());
	}
}

