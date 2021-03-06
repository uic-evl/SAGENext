#ifndef EXTERNALGUIMAIN_H
#define EXTERNALGUIMAIN_H

#include <QtGui>
//#include <QHostAddress>
//#include <QAbstractSocket>
#include <QSettings>
#include <QTcpSocket>
#include <QTcpServer>

#include "../thewall/common/commondefinitions.h"



namespace Ui {
	class SN_PointerUI;
}


/**
  Provides drag & drop feature
  */
class SN_PointerUI_DropFrame : public QLabel {
	Q_OBJECT
public:
	explicit SN_PointerUI_DropFrame(QWidget *parent = 0);

protected:
	void dragEnterEvent(QDragEnterEvent *);
	void dropEvent(QDropEvent *);

signals:
	void mediaDropped(QList<QUrl> mediaurls);
};






/*!
  Win Capture pipe
  */
class SN_WinCaptureTcpServer : public QTcpServer
{
	Q_OBJECT
public:
	SN_WinCaptureTcpServer(QTcpSocket *s, QObject *parent=0) : QTcpServer(parent), _socket(s) {}

private:
	QTcpSocket *_socket;

protected:
	void incomingConnection(int handle) {
//		qDebug() << "incoming connection";
		Q_ASSERT(_socket);
		_socket->setSocketDescriptor(handle);
	}
};





class SN_PointerUI_StrDialog : public QDialog
{
	Q_OBJECT
public:
	SN_PointerUI_StrDialog(QWidget *parent=0);

	inline QString text() {return _text;}

private:
	QLineEdit *_lineedit;
	QPushButton *_okbutton;
	QPushButton *_cancelbutton;

	QString _text;

public slots:
	void setText();
};





/**
  sageNextPointer MainWindow
  */
class SN_PointerUI : public QMainWindow
{
	Q_OBJECT
	
public:
	explicit SN_PointerUI(QWidget *parent = 0);
	~SN_PointerUI();
	
protected:
	void mouseMoveEvent(QMouseEvent *e);
	void mousePressEvent(QMouseEvent *e);
	void mouseReleaseEvent(QMouseEvent *e);
	void mouseDoubleClickEvent(QMouseEvent *e);
	void wheelEvent(QWheelEvent *e);
	
	/**
	  Override this to provide mouse right click
	 */
	void contextMenuEvent(QContextMenuEvent *event);
	
	
	void sendMouseMove(const QPoint globalPos, Qt::MouseButtons btns = Qt::NoButton);

	/**
	  This will trigger pointer->setAppUnderPointer()
	  */
	void sendMousePress(const QPoint globalPos, Qt::MouseButtons btns = Qt::LeftButton);

	/**
	  This is NOT the mouseRelease that results pointerClick()
	  This is to know the point where mouse draggin has finished.
	  */
	void sendMouseRelease(const QPoint globalPos, Qt::MouseButtons btns = Qt::LeftButton);

	/**
	  The mouse press followed by mouse release will triggers this.
	  */
	void sendMouseClick(const QPoint globalPos, Qt::MouseButtons btns = Qt::LeftButton | Qt::NoButton);

	void sendMouseDblClick(const QPoint globalPos, Qt::MouseButtons btns = Qt::LeftButton | Qt::NoButton);

	void sendMouseWheel(const QPoint globalPos, int delta);

private:
	Ui::SN_PointerUI *ui;

	QSettings *_settings;

        /**
          unique ID of me used by UiServer to differentiate multiple ui clients.
          Note that uiclientid is unique ONLY within the wall represented by sockfd.
          It is absolutely valid and likely that multiple message threads have same uiclientid value.

          This should be map data structure to support multiple wall connections
          */
	quint32 _uiclientid;
	
	QSplashScreen _inSAGEsplash;

	/**
	  receives this from UiServer upon connection.
	  It is defined in "general/fileserverport'
	  */
	int fileTransferPort;

        /**
          * SHIFT + CMD + ALT + m
          */
	QAction *ungrabMouseAction;


	/**
      The socket for the message channel.
	  Handshaking messgaes, all the mouse events is sent throught this socket.
      */
	QTcpSocket _tcpMsgSock;


	/**
	  The socket for file transferring.
	  */
	QTcpSocket _tcpDataSock;


//    QUdpSocket _udpSocket;


	/**
       When I send my coord to the wall, I'll multiple this to map my local coord to the wall coord.
	  */
	qreal scaleToWallX;
	qreal scaleToWallY;

        /**
          * when I receive app geometry from the wall
          */
//	qreal scaleFromWallX;
//	qreal scaleFromWallY;


		/**
		  Width, Height of the wall in pixel
		  */
	QSizeF wallSize;

        /**
          * non-modal file dialog
          */
	QFileDialog *fdialog;


        /**
          The file transfer thread. _tcpDataSock is used.
          */
//	SN_PointerUI_DataThread *sendThread;

		/**
		  to keep track mouse dragging start and end position
		  */
	QPoint mousePressedPos;
		
		/**
		  true if pointer sharing is on
		  */
	bool isMouseCapturing;

		/**
		  This is QLabel that accept dropEvent
		  */
	SN_PointerUI_DropFrame *mediaDropFrame;

	QString _wallAddress;

    quint16 _wallPort;

	QString _pointerName;

	QString _pointerColor;

	QString _sharingEdge;
		
		/**
		  For mac users, start macCapture process written by Ratko to intercept mac's mouse events
		  11 :start capturing 
		  12 : no capturing (mouse is back to my desktop)
		  MOVE(1) x(0~1) y(0~1)
          CLICK(2) LEFT(1) PRESS
		  CLICK(2) LEFT(1) RELEASE
		  CLICK(2) RIGHT(2) PRESS
		  CLICK(2) RIGHT(2) RELEASE
		  WHEEL(3) 
		  DOUBLE_CLICK(5) int
		  **/
	QProcess *macCapture;

	/*!
	  winCapture.exe (from winCapture.py)
	  built by py2exe
	  */
	QProcess *_winCapture;

	/*!
	  winCapture.exe will connect to localhost:44556 upon starting.
      This is because winCapture binary made by py2exe doesn't support stdout/err outputs.
      So data from winCapture is sent through TCP socket
	  */
	SN_WinCaptureTcpServer *_winCaptureServer;

	/*!
	  winCapture.exe and this program will communicate through socket.
	  It's because py2exe disables STDOUT channel !!!!
	  */
	QTcpSocket _winCapturePipe;

	/*!
	  if Q_OS_MAC then it's macCapture (which is QProcess)
	  if Q_OS_WIN then it's _winCapturePipe( which is QTcpSocket)
	  */
	QIODevice *_iodeviceForMouseHook;
		
		/**
		  This is for macCapture
		  to figure out mouse button state when using macCapture
		  0 nothing
		  1 left button pressed
		  2 right button pressed
		  */
	int mouseBtnPressed;
		
		/**
		  This is for macCapture
		  to keep track current mouse position when click/dblclick/wheel happens
		  */
	QPoint currentGlobalPos;


    /*!
      In Linux, where Qt's mouse event handlers call sendMouseXXXX(),
      The double click event deliver following messages
      PRESS -> CLICK -> DBLCLICK -> CLICK

      The last CLICK, which shouldn't be there, is because of the mouseReleaseEvent

      So, this flag prevents sending CLICK in mouseReleaseEvent if
      the event preceding is sendDblClick
      */
    bool _wasDblClick;
	
	/*!
	  This is for Win32.
	  winCapture doesn't send DblClick so it's handled in readFromMouseHook
	  */
	qint64 _prevClickTime;


		/**
		  Queue invoking MessageThread::sendMsg()
		  */
//	void queueMsgToWall(const QByteArray &msg);
		
//	void connectToWall(const char * ipaddr, quint16 port);

    /*!
      Assuming file transferring is sequential.
      This is the filename (w/o space characters) currently being transferred
      */
    QPair<QString, qint64> _fileBeingSent;

    /*!
      file sending thread should wait until the SAGENext finishes receiving the file
      */
    QSemaphore _fileTransferSemaphore;

    QProgressDialog *_progressDialog;

    /*!
      This function calls sendFileToWall() for each item in the QList.
      The runSendFileThread() runs this function in a separate thread.
      */
	void sendFilesToWall(const QList<QUrl> &);

	/*!
	  This function sends a file to the wall. Sending consists of two operations: Sending a header (through data socket) followed by the actual file.
	  */
	void sendFileToWall(const QUrl &);

	/*!
	  The filepath is the absoulte path name of the file reside at the wall.
	  */
	void recvFileFromWall(const QString &filepath, qint64 filesize);
	
	
	void m_deleteMouseHookProcess();

public slots:
	/*!
	  Upon connection, receive wall size, uiclientid and sets scaleToWallX/Y
	  */
	void initialize(quint32 uiclientid, int wallwidth, int wallheight, int ftpPort);

	/*!
	  send message to the wall. messages are usually pointer operations.
	  The size of the message from uiclient to wall is EXTUI_SMALL_MSG_SIZE = 128 Byte. This is defined in src/thewall/common/commondefinitions.h
	  */
	void sendMessage(const QByteArray &msg);

	/*!
	  read message sent from the wall.
	  The size of the message from wall to uiclient is EXTUI_MSG_SIZE = 1280 Byte. This is defined in src/thewall/common/commondefinitions.h
	  */
	void readMessage();

	/*!
	  This function runs SN_PointerUI::sendFiles() in a separate thread using QtConcurrent::run()
	  */
	void runSendFileThread(const QList<QUrl> &);

	void runRecvFileThread(const QString &filepath, qint64 filesize);


private slots:
    void handleSocketError(QAbstractSocket::SocketError error);

    void handleSocketStateChange(QAbstractSocket::SocketState newstate);

        /*!
          CMD + N triggers the connection dialog.
		  Upon accepting the dialog, it attempts to connect to the wall with the _tcpMsgSock socket.

		  If there already exist the message channel. The _tcpMsgSock will be closed.
          */
	void on_actionNew_Connection_triggered();


    /*!
      This function keeps trying to connect to a TCP server in a forever loop.
      */
//    void connectToTheWall();


        /**
          * This is for Windows OS temporarily
          */
	void on_hookMouseBtn_clicked();

	void hookMouse();

	void unhookMouse();


    /*!
      The send thread is about to send()
      */
    void fileSendingBegins(QString filename, qint64 filesize);

    /*!
      SAGENext informed me that it received the file _fileBeingSent
      So this function releases _fileTransferSemaphore for the send thread can return
      */
//    void fileSendingCompleted();


        /**
          shortcut : CTRL(CMD) + O
		  The action (ui->actionOpen_Media) is defined in the sn_pointer.ui
          */
	void on_actionOpen_Media_triggered();

        /**
          * when fileDialog returns this function is invoked.
          * This functions will invoke MessageThread::registerApp() slot
          */
	void readLocalFiles(QStringList);
		
		//void sendFile(const QString &f, int mediatype);
		
		/**
		  respond to macCapture (read from stdout, translate msg, send to the wall)
		  This slot is connected to QProcess::readyReadStandardOutput() signal
		  */
	void readFromMouseHook();
		

	void readFromWinCapture();

	/*!
	  VNC sharing.
	  The action (ui->actionShare_Desktop) is defined in the sn_pointerui.ui
	  */
	void on_actionShare_desktop_triggered();


	/*!
	  send text to the current application.
	  It assumes that a user 'clicked' a GUI item ,to which the user wants to send text, of the application.
	  The action (ui->actionSend_text is defined in the sn_pointerui.ui
	  */
	void on_actionSend_text_triggered();
	
};








#endif // EXTERNALGUIMAIN_H
