#include "mainwindow.h"
#include <QApplication>

#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>

#include <QCoreApplication>
#include <QTcpServer>
#include <QWebSocketServer>
#include <QWebSocket>

#include <thread>
#include <chrono>

int main(int argc, char *argv[])
{

    QApplication a(argc, argv);

    QWebSocketServer server("ws://localhost:8080", QWebSocketServer::NonSecureMode);

    QHostAddress address = QHostAddress("localhost:8080");

    if (server.listen(address, 8080))
    {
        qDebug() << "WebSocket сервер запущен на порту 8080";

        QObject::connect(&server, &QWebSocketServer::newConnection, [&server]()
                         {
                             QWebSocket *socket = server.nextPendingConnection();
                             qDebug() << "Подключено новое WebSocket-соединение";

                             QDateTime currentDateTime = QDateTime::currentDateTime();

                             QJsonObject jsonObject1;
                             jsonObject1["type"] = "text";
                             jsonObject1["time"] = currentDateTime.toString("yyyy-MM-dd hh:mm:ss");
                             jsonObject1["data"] = 30;

                             socket->sendTextMessage(QJsonDocument(jsonObject1).toJson()); });
    }
    else
    {
        qDebug() << "Не удалось запустить WebSocket сервер";
    }

    MainWindow w;
    w.setWindowTitle("CRM Store");
    w.setWindowIcon(QIcon("C:/Users/Admin/Desktop/crm-store/client/desktop/GUI/icons/favicon.png"));

    w.show();
    return a.exec();
}
