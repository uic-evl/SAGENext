#ifndef SAGENEXTVISBASEWIDGET_H
#define SAGENEXTVISBASEWIDGET_H

#include <QObject>
#include <QDebug>

#include "../base/basewidget.h"


class SageNextVisBaseWidget : public SN_BaseWidget
{
    Q_OBJECT

public:

    SageNextVisBaseWidget(Qt::WindowFlags wflags = Qt::Window);
    SageNextVisBaseWidget(quint64 globalappid, const QSettings *s, QGraphicsItem *parent = 0, Qt::WindowFlags wflags = 0);



};

#endif // SAGENEXTVISBASEWIDGET_H
