#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <string>
#include <stdio.h>
using namespace std;
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(this, &MainWindow::newMessage, this, &MainWindow::displayMessage);
    connect(ui->pushButton,  &QPushButton::clicked, this, &MainWindow::ButtonEvent);
    ui->statusBar->showMessage("Server is idle");
    m_server = new QTcpServer();
}

MainWindow::~MainWindow()
{
    foreach (QTcpSocket* socket, connection_set)
    {
        socket->close();
        socket->deleteLater();
    }

    m_server->close();
    m_server->deleteLater();

    delete ui;
}

void MainWindow::ButtonEvent()
{
    static uint8_t socketState = 0;
    if(socketState == 0)
    {
        //displayMessage(ui->comboBox->itemData(ui->comboBox->currentIndex()));
        if(m_server->listen(QHostAddress::Any, ui->comboBox->itemText(ui->comboBox->currentIndex()).toInt()))
        {
           connect(m_server, &QTcpServer::newConnection, this, &MainWindow::newConnection);
           ui->pushButton->setText("Stop Listen");
           ui->statusBar->showMessage(QString("Server is listening at port %1").arg(ui->comboBox->itemText(ui->comboBox->currentIndex())));
           ui->comboBox->setDisabled(1);
        }
        else
        {
            QMessageBox::critical(this,"QTCPServer",QString("Unable to start the server: %1.").arg(m_server->errorString()));
            exit(EXIT_FAILURE);
        }
        socketState = 1;
    }
    else
    {
        ui->pushButton->setText("Start Listen");
        ui->comboBox->setEnabled(1);
        foreach (QTcpSocket* socket, connection_set)
        {
            socket->close();
            socket->deleteLater();
        }

        m_server->close();
        ui->statusBar->showMessage("Server is idle");
        socketState = 0;
    }
}


void MainWindow::newConnection()
{
    while (m_server->hasPendingConnections())
        appendToSocketList(m_server->nextPendingConnection());
}

void MainWindow::appendToSocketList(QTcpSocket* socket)
{
    connection_set.insert(socket);
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::readSocket);
    connect(socket, &QTcpSocket::disconnected, this, &MainWindow::discardSocket);
    displayMessage(QString("INFO :: Client with sockd:%1 has just entered the room").arg(socket->socketDescriptor()));
}

void MainWindow::readSocket()
{
    QTcpSocket* socket = reinterpret_cast<QTcpSocket*>(sender());

    QByteArray buffer;

    QByteArray data;
    string str_data;

    if (socket->bytesAvailable())
    {
        data = socket->readAll();
        str_data = data.constData();
        qDebug() << "Incoming socket data: " << data;
//        handle_tcp_command(str_data);   // pass the msg
        QString message = QString("%1 :: %2").arg(socket->socketDescriptor()).arg(QString::fromStdString(str_data));
        emit newMessage(message);

    }
    else
    {
        socket->write("could not receive data");
        socket->flush();
        socket->waitForBytesWritten(50);
        socket->close();
    }

}

void MainWindow::discardSocket()
{
    QTcpSocket* socket = reinterpret_cast<QTcpSocket*>(sender());
    QSet<QTcpSocket*>::iterator it = connection_set.find(socket);
    if (it != connection_set.end()){
        displayMessage(QString("INFO :: A client has just left the room").arg(socket->socketDescriptor()));
        connection_set.remove(*it);
    }
    refreshComboBox();
    
    socket->deleteLater();
}

void MainWindow::displayError(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
        case QAbstractSocket::RemoteHostClosedError:
        break;
        case QAbstractSocket::HostNotFoundError:
            QMessageBox::information(this, "QTCPServer", "The host was not found. Please check the host name and port settings.");
        break;
        case QAbstractSocket::ConnectionRefusedError:
            QMessageBox::information(this, "QTCPServer", "The connection was refused by the peer. Make sure QTCPServer is running, and check that the host name and port settings are correct.");
        break;
        default:
            QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
            QMessageBox::information(this, "QTCPServer", QString("The following error occurred: %1.").arg(socket->errorString()));
        break;
    }
}

void MainWindow::displayMessage(const QString& str)
{
    ui->textBrowser_receivedMessages->append(str);
}

void MainWindow::refreshComboBox(){
}
