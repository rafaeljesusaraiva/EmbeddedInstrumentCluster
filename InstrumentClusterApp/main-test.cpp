#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QProcess>
#include <QCanBus>
#include <QCanBusDeviceInfo>

class Cluster : public QWidget {
    bool canFound = false;
    QString status = "SYSTEM BOOTING...";

public:
    Cluster() {
        // Force the window size for 1920x720
        setFixedSize(1920, 720);

        QTimer *detectTimer = new QTimer(this);
        connect(detectTimer, &QTimer::timeout, this, &Cluster::checkHardware);
        detectTimer->start(1500);
    }

    void checkHardware() {
        if (canFound) return;

        auto devices = QCanBus::instance()->availableDevices("socketcan");
        bool foundCan0 = false;

        // Loop through the device objects and check their names
        for (const QCanBusDeviceInfo &info : devices) {
            if (info.name() == "can0") {
                foundCan0 = true;
                break;
            }
        }

        if (foundCan0) {
            status = "CAN MODULE DETECTED\nSTARTING INTERFACE...";
            update();

            // Updated QProcess syntax to clear the deprecation warnings
            QProcess::execute("ip", QStringList() << "link" << "set" << "can0" << "type" << "can" << "bitrate" << "500000");
            QProcess::execute("ip", QStringList() << "link" << "set" << "can0" << "up");

            canFound = true;
            status = "CAN ONLINE\nWAITING FOR OBDII DATA";
        } else {
            status = "SEARCHING FOR CAN MODULE...\nCHECK SPI/GPIO WIRING";
        }
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        // Draw a dark blue gradient so we can see the screen bounds
        QLinearGradient grad(0, 0, 0, 720);
        grad.setColorAt(0, QColor(0, 0, 40));
        grad.setColorAt(1, Qt::black);
        p.fillRect(rect(), grad);

        // Draw a border
        p.setPen(QPen(canFound ? Qt::cyan : Qt::red, 4));
        p.drawRect(10, 10, 1900, 700);

        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 45, QFont::Bold));
        p.drawText(rect(), Qt::AlignCenter, status);
    }
};

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    Cluster w;
    w.showFullScreen();
    return a.exec();
}
