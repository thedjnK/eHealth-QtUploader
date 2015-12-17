#ifndef DEVICESETTINGS_H
#define DEVICESETTINGS_H

#include <QDialog>

namespace Ui
{
    class DeviceSettings;
}

class DeviceSettings : public QDialog
{
    Q_OBJECT

public:
    explicit DeviceSettings(QWidget *parent = 0);
    ~DeviceSettings();

private slots:
    void on_btn_Save_clicked();
    void on_btn_Load_clicked();
    void on_btn_Cancel_clicked();

signals:
    void UpdateConfig();
    void LoadConfig();

private:
    Ui::DeviceSettings *ui;
};

#endif // DEVICESETTINGS_H
