/**
  @file
  @author Stefan Frings
*/

#include "httplistener.h"
#include "httpconnectionhandler.h"
#include "httpconnectionhandlerpool.h"
#include <QCoreApplication>

HttpListener::HttpListener(HttpRequestHandler* requestHandler, QObject *parent)
    : QTcpServer(parent)
{
    qDebug() << "HttpListener::HttpListener";
    // Create connection handler pool
	this->settings=QApplicationSettings::getInstance();
    pool=new HttpConnectionHandlerPool(requestHandler);
    
	
}

HttpListener::~HttpListener() {
    close();
    qDebug("HttpListener: closed");
    delete pool;
    qDebug("HttpListener: destroyed");
}


void HttpListener::incomingConnection(int socketDescriptor) {
#ifdef SUPERVERBOSE
    qDebug("HttpListener: New connection");
#endif
    HttpConnectionHandler* freeHandler=pool->getConnectionHandler();

    // Let the handler process the new connection.
    if (freeHandler) {
        // The descriptor is passed via signal/slot because the handler lives in another
        // thread and cannot open the socket when called by another thread.

        connect(this,SIGNAL(handleConnection(int)),freeHandler,SLOT(handleConnection(int)));
        emit handleConnection(socketDescriptor);
        disconnect(this,SIGNAL(handleConnection(int)),freeHandler,SLOT(handleConnection(int)));
    }
    else {
        // Reject the connection
        qDebug("HttpListener: Too many incoming connections");
        QTcpSocket* socket=new QTcpSocket(this);
        socket->setSocketDescriptor(socketDescriptor);
        connect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));
        socket->write("HTTP/1.1 503 too many connections\r\nConnection: close\r\n\r\nToo many connections\r\n");
        socket->disconnectFromHost();
    }
}

void HttpListener::Start()
{
	// Start listening
	qDebug() << settings->value("WebControl","web_port");
	int port=settings->value("WebControl","web_port").toInt();
	listen(QHostAddress::Any, port);
	if (!isListening()) {
		qCritical("HttpListener: Cannot bind on port %i: %s",port,qPrintable(errorString()));
	}
	else {
		qDebug("HttpListener: Listening on port %i",port);
	}
}
