#ifndef SN_STREAMDISPLAY_H
#define SN_STREAMDISPLAY_H

#include <qxmpp/QXmppCallManager.h>
#include "sn_displaystreambase.h"
#include "receivethread.h"
#include "sendthread.h"

class SN_StreamDisplay : public SN_DisplayStreamBase
{
    Q_OBJECT
public:
    SN_StreamDisplay(const quint64 globalAppId, const QSettings* s, SN_ResourceMonitor* rm=0, QGraphicsItem* parent=0, Qt::WindowFlags wFlags=0, ImageBuffer* receive_buf=0, ImageBuffer* send_buf=0);
    ~SN_StreamDisplay();
    
public slots:
    void videoRecvd(QIODevice::OpenMode mode);
    void callRecvd(QXmppCall* call);
    void callStd(QXmppCall* call);
    void call_connected();
private:
    QXmppCall* _currCall;
    ReceiveThread* _recvThread;
    SendThread* _send_thread;
    ImageBuffer* _send_buffer;
};

#endif // SN_STREAMDISPLAY_H
