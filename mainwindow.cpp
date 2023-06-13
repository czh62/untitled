#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "algorithm.h"
#include <QtNetwork/QHostInfo>
#include <QtNetwork/QNetworkInterface>
#include "QRegularExpression"

#include "QDebug"

using namespace std;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), id([]()
                                                      {
        random_device rd;
        mt19937_64 gen(rd());
        uniform_int_distribution<int64_t> dis(0x0000800000000000, 0x0000ffffffffffff);
        return dis(gen); }()),
      privatePassword([]()
                      {
        random_device rd;
        mt19937_64 gen(rd());
        uniform_int_distribution<int64_t> dis(0x0000800000000000, 0x0000ffffffffffff);
        return dis(gen); }())
{
    ui->setupUi(this);

    this->ui->privateKeyLabel->setText(QString("私有密钥： %1").arg(privatePassword));
    leader = false;
    this->ui->idLabel->setText(QString("用户ID： %1").arg(id));

    qDebug() << id;

    // 创建udp对象
    localIp = this->ui->lineEdit->text();
    int index = 0; // 查找第一个"."字符的位置
    for (int i = 0; i < 3; i++) {
        index = localIp.indexOf(".", index) + 1; // 查找下一个"."字符的位置
        qDebug() << index;
    }
    if (index != -1) {
        localIp.replace(index, localIp.length() - index, "255"); // 替换第四个"."字符之后的所有字符
    }
    udpSocket = new QUdpSocket(this);
    udpSocket->bind(QHostAddress::Any, 31119);
    qDebug() << localIp;

    // 连接对象操作
    connect(this->ui->lineEdit, &QLineEdit::editingFinished, this, &MainWindow::getLocalIp);
    connect(udpSocket, &QUdpSocket::readyRead, this, &MainWindow::receivedUdp);
    connect(this->ui->initiateButton, &QPushButton::clicked, this, &MainWindow::beginConnect);
}

void MainWindow::receivedUdp()
{
    while (udpSocket->hasPendingDatagrams())
    {

        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(datagram.data(), datagram.size());
        qDebug() << "received data" << datagram;
        QString temp = QString::fromUtf8(datagram);
        static QRegularExpression rx("\\d+");
        if (temp.endsWith("czh"))
        {
            if (temp.startsWith("n:"))
            {

                endConnect();
                // 启动正则匹配
                QRegularExpressionMatchIterator i = rx.globalMatch(temp);
                QRegularExpressionMatch match;

                // 获取大素数
                match = i.next();
                n = QString(match.captured(0)).toLongLong();
                this->ui->nNumber->setText(QString("大素数： %1").arg(n));

                // 获取原根
                match = i.next();
                g = QString(match.captured(0)).toLongLong();
                this->ui->pNumber->setText(QString("原    根： %1").arg(g));
                // 获取leader id
                match = i.next();
                int64_t receivedId = QString(match.captured(0)).toLongLong();
                connectNumber = 1;
                connectId[0] = receivedId;
                this->ui->numberLabel->setText(QString("连接数量： 1"));

                // 判断leader
                if (receivedId == id)
                {
                    leader = true;
                    disconnect(this->ui->connectButton, &QPushButton::clicked, nullptr, nullptr);
                    ui->connectButton->setText("已加入连接");
                    ui->connectButton->setEnabled(false);
                }
                else
                {
                    leader = false;
                    this->ui->textBrowser->append(QString("收到大素数和原根"));
                    disconnect(this->ui->connectButton, &QPushButton::clicked, nullptr, nullptr);
                    connect(this->ui->connectButton, &QPushButton::clicked, this, &MainWindow::startConnect);
                    ui->connectButton->setText("加入连接");
                    ui->connectButton->setEnabled(true);
                }
                ui->ID1Label->setText(QString("发起人： %1").arg(connectId[0]));
            }
            else if (temp.startsWith("end"))
            {
                this->ui->textBrowser->append(QString("收到结束信号"));
                endConnect();
            }
            else if (temp.startsWith("id:"))
            {
                QRegularExpressionMatchIterator i = rx.globalMatch(temp);
                QRegularExpressionMatch match;
                match = i.next();
                this->ui->textBrowser->append(QString("收到连接请求id:%1").arg(QString(match.captured(0))));
                // 如果连接不到上限则加入
                if (connectNumber < 3)
                {
                    bool flag = true;
                    for (int j = 0; j < connectNumber; j++)
                    {
                        if (connectId[j] == QString(match.captured(0)).toLongLong())
                        {
                            flag = false;
                            break;
                        }
                    }
                    if (flag)
                    {
                        connectNumber++;
                        connectId[connectNumber - 1] = QString(match.captured(0)).toLongLong();
                        this->ui->textBrowser->append(QString("已添加连接"));
                        this->ui->numberLabel->setText(QString("连接数量： %1").arg(connectNumber));
                        if (connectNumber == 3)
                        {
                            this->ui->textBrowser->append(QString("连接已满"));
                            this->ui->textBrowser->append(QString("等待接收检查信号"));
                            disconnect(this->ui->connectButton, &QPushButton::clicked, nullptr, nullptr);
                            this->ui->connectButton->setText("连接已满");
                            this->ui->connectButton->setEnabled(false);
                        }
                    }
                }
                else
                {
                    this->ui->textBrowser->append(QString("连接已满"));

                    disconnect(this->ui->connectButton, &QPushButton::clicked, nullptr, nullptr);
                    this->ui->connectButton->setText("连接已满");
                    this->ui->connectButton->setEnabled(false);
                }
                match = i.next();

                if (leader)
                {
                    if (id == QString(match.captured(0)).toLongLong())
                    {
                        // leader发发送检查信号
                        if (connectNumber == 3)
                        {
                            this->ui->textBrowser->append(QString("发送检查信号"));
                            QString message = QString("leader check %1 %2 %3 czh").arg(connectId[0]).arg(connectId[1]).arg(connectId[2]);
                            QByteArray datagram = message.toUtf8();
                            udpSocket->writeDatagram(datagram, QHostAddress(localIp), 31119);
                            qDebug() << "send data:" << datagram;
                            ;
                        }
                    }
                    else
                    {
                        this->ui->textBrowser->append(QString("与本地记录不匹配，重置连接"));
                        QString message = QString("end id:%1 czh").arg(id);
                        QByteArray datagram = message.toUtf8();
                        udpSocket->writeDatagram(datagram, QHostAddress(localIp), 31119);
                        qDebug() << "send data:" << datagram;
                        ;
                        endConnect();
                    }
                }
                else
                {

                    if (id == QString(match.captured(0)).toLongLong())
                    {
                        this->ui->textBrowser->append(QString("与本地记录不匹配，重置连接"));
                        QString message = QString("end id:%1 czh").arg(id);
                        QByteArray datagram = message.toUtf8();
                        udpSocket->writeDatagram(datagram, QHostAddress(localIp), 31119);
                        qDebug() << "send data:" << datagram;
                        ;
                        endConnect();
                    }
                }
                if (connectId[1])
                    ui->ID2Label->setText(QString("成员一： %1").arg(connectId[1]));
                if (connectId[2])
                    ui->ID3Label->setText(QString("成员二： %1").arg(connectId[2]));
            }
            else if (temp.startsWith("leader"))
            {
                bool selfFlag = false;
                QRegularExpressionMatchIterator i = rx.globalMatch(temp);
                QRegularExpressionMatch match;
                for (int j = 0; j < 3; j++)
                {
                    if (connectId[j] == id)
                    {
                        selfFlag = true;
                        break;
                    }
                }
                if (selfFlag)
                {
                    for (int j = 0; j < 3; j++)
                    {
                        match = i.next();
                        if (connectId[j] != QString(match.captured(0)).toLongLong())
                        {
                            selfFlag = false;
                            QString message = QString("end id:%1 czh").arg(id);
                            QByteArray datagram = message.toUtf8();
                            udpSocket->writeDatagram(datagram, QHostAddress(localIp), 31119);
                            qDebug() << "send data:" << datagram;
                            ;
                            endConnect();
                            this->ui->textBrowser->append(QString("检查信号不匹配，重置连接"));
                            break;
                        }
                    }
                    if (selfFlag)
                    {
                        disconnect(this->ui->connectButton, &QPushButton::clicked, nullptr, nullptr);
                        ui->connectButton->setText("结束连接");
                        ui->connectButton->setEnabled(true);
                        connect(this->ui->connectButton, &QPushButton::clicked, this, &MainWindow::overConnect);
                        this->ui->textBrowser->append(QString("检查信号匹配，开始协商密钥"));
                        QString message = QString("temp:%1 id:%2 id:0 id:0 czh").arg(mod_pow(g, privatePassword, n)).arg(id);
                        QByteArray datagram = message.toUtf8();
                        udpSocket->writeDatagram(datagram, QHostAddress(localIp), 31119);
                        qDebug() << "send data:" << datagram;
                    }
                }
                else
                {
                    endConnect();
                }
            }
            else if (temp.startsWith("temp"))
            {
                int64_t tmp[4];
                int32_t count = 0;
                bool flag = true;
                QRegularExpressionMatchIterator i = rx.globalMatch(temp);
                QRegularExpressionMatch match;

                for (int j = 0; j < 4; j++)
                {
                    match = i.next();
                    tmp[j] = QString(match.captured(0)).toLongLong();
                    if (tmp[j] == id)
                    {
                        flag = false;
                    }
                    if (tmp[j] == 0)
                    {
                        count++;
                    }
                }
                if (flag)
                {
                    QString message;
                    QByteArray datagram;
                    switch (count)
                    {
                    case 1:
                        password = mod_pow(tmp[0], privatePassword, n);
                        ui->passwordLabel->setText(QString("协商密钥：%1").arg(password));
                        this->ui->textBrowser->append("收到中间数据：" + temp);
                        this->ui->textBrowser->append(QString("协商密钥完成"));
                        break;

                    case 2:
                        message = QString("temp:%1 id:%2 id:%3 id:%4 czh").arg(mod_pow(tmp[0], privatePassword, n)).arg(tmp[1]).arg(id).arg(0);
                        datagram = message.toUtf8();
                        udpSocket->writeDatagram(datagram, QHostAddress(localIp), 31119);
                        qDebug() << "send data:" << datagram;
                        this->ui->textBrowser->append("收到中间数据：" + temp);
                        break;

                    default:
                        message = QString("end id:%1 czh").arg(id);
                        datagram = message.toUtf8();
                        udpSocket->writeDatagram(datagram, QHostAddress(localIp), 31119);
                        qDebug() << "send data:" << datagram;
                        ;
                        endConnect();
                        break;
                    }
                }
            }
        }
    }
}
void MainWindow::overConnect()
{
    QString message = QString("end id:%1 czh").arg(id);
    QByteArray datagram = message.toUtf8();
    udpSocket->writeDatagram(datagram, QHostAddress(localIp), 31119);
    qDebug() << "send data:" << datagram;
    ;
    endConnect();
}

void MainWindow::startConnect()
{
    QString message = QString("id:%1 leader:%2 czh").arg(id).arg(connectId[0]);
    QByteArray datagram = message.toUtf8();
    udpSocket->writeDatagram(datagram, QHostAddress(localIp), 31119);
    disconnect(this->ui->connectButton, &QPushButton::clicked, nullptr, nullptr);
    this->ui->connectButton->setText("已加入连接");
    this->ui->connectButton->setEnabled(false);
    if (connectNumber < 3)
    {
        bool flag = true;
        for (int j = 0; j < connectNumber; j++)
        {
            if (connectId[j] == id)
            {
                flag = false;
                break;
            }
        }
        if (flag)
        {
            connectNumber++;
            connectId[connectNumber - 1] = id;
            this->ui->textBrowser->append(QString("已添加连接"));
            this->ui->numberLabel->setText(QString("连接数量： %1").arg(connectNumber));
            if (connectNumber == 3)
            {
                this->ui->textBrowser->append(QString("连接已满"));
                this->ui->textBrowser->append(QString("等待接收检查信号"));
                disconnect(this->ui->connectButton, &QPushButton::clicked, nullptr, nullptr);
                this->ui->connectButton->setText("连接已满");
                this->ui->connectButton->setEnabled(false);
            }
        }
    }
    else
    {
        this->ui->textBrowser->append(QString("连接已满"));
        disconnect(this->ui->connectButton, &QPushButton::clicked, nullptr, nullptr);
        this->ui->connectButton->setText("连接已满");
        this->ui->connectButton->setEnabled(false);
    }
}

void MainWindow::endConnect()
{
    n = 0;
    g = 0;
    connectNumber = 0;
    password = 0;
    connectId[0] = 0;
    connectId[1] = 0;
    connectId[2] = 0;
    leader = false;
    disconnect(this->ui->connectButton, &QPushButton::clicked, nullptr, nullptr);
    this->ui->nNumber->setText(QString("大素数："));
    this->ui->pNumber->setText(QString("原    根："));
    this->ui->ID1Label->setText(QString("发起人："));
    this->ui->ID2Label->setText(QString("成员一："));
    this->ui->ID3Label->setText(QString("成员二："));
    this->ui->numberLabel->setText(QString("连接数量："));
    this->ui->connectButton->setText(QString("未发现会话"));
    this->ui->connectButton->setEnabled(false);
}

void MainWindow::beginConnect()
{
    // 创建原根
    n = ProduceRandomPrime();
    g = PrimitiveElement(n);
    this->ui->nNumber->setText(QString("大素数：%1").arg(n));
    this->ui->pNumber->setText(QString("原    根： %1").arg(g));
    this->ui->textBrowser->append(QString("创建大素数和原根"));

    // 广播
    QString message = QString("n:%1 p:%2 id:%3 czh").arg(n).arg(g).arg(id);
    QByteArray datagram = message.toUtf8();
    if(udpSocket->writeDatagram(datagram, QHostAddress(localIp), 31119) == -1){
        qDebug() << udpSocket->error();
    }
    this->ui->textBrowser->append(QString("发送大素数和原根"));
    // qDebug()<<"send data:" << datagram;
    connectNumber = 1;
    connectId[0] = id;
    this->ui->ID1Label->setText(QString("发起人： %1").arg(id));
    this->ui->numberLabel->setText(QString("连接数量： 1"));
    leader = true;
    disconnect(this->ui->connectButton, &QPushButton::clicked, nullptr, nullptr);
    ui->connectButton->setText("已加入连接");
    ui->connectButton->setEnabled(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::getLocalIp() {
    localIp = this->ui->lineEdit->text();
    int index = 0; // 查找第一个"."字符的位置
    for (int i = 0; i < 3; i++) {
        index = localIp.indexOf(".", index) + 1; // 查找下一个"."字符的位置

    }
    if (index != 0) {
        localIp.replace(index, localIp.length() - index, "255"); // 替换第四个"."字符之后的所有字符
        this->ui->textBrowser->append(QString("更新广播地址"));
    }else{
        this->ui->textBrowser->append(QString("非法地址"));
    }
    qDebug() << localIp;

}
