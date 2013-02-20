#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QObject>
#include <QDebug>

#include "applications/sagenextvis/VisBaseClasses/visbasewidget.h"
#include "applications/base/SN_plugininterface.h"
#include "applications/base/basewidget.h"
//#include "applications/sagenextvis/VisBaseClasses/sagenextvisbasewidget.h"

class VisTestWidget : public VisBaseWidget, SN_PluginInterface
{
    Q_OBJECT

    /*!
      A macro that tells this is a plugin
      and will implement interfaces defined in SN_PluginInterface
      */
    Q_INTERFACES(SN_PluginInterface)
    
public:

    explicit VisTestWidget(QGraphicsItem* parent = 0);
    virtual ~VisTestWidget();
    
    /*!
      This interface must be reimplemented.
      The SN_Launcher calls this function to create an instance of this plugin.
      */
    SN_BaseWidget * createInstance();

    /*!
      Drawing function
      */
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

};

#endif // MAINWINDOW_H
