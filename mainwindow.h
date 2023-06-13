#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QUdpSocket>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void beginConnect();
    void startConnect();
    void endConnect();
    void overConnect();
    void receivedUdp();
    void getLocalIp();

private:
    Ui::MainWindow *ui;

    const int64_t id;
    const int64_t privatePassword;
    bool leader;

    int64_t password;
    QString localIp;

    int64_t n, g;
    int64_t connectId[3];
    uint8_t connectNumber;

    QUdpSocket *udpSocket;
};
#endif // MAINWINDOW_H
