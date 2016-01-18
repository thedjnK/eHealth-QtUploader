#include "devicesettings.h"
#include "ui_devicesettings.h"

DeviceSettings::DeviceSettings(QWidget *parent) : QDialog(parent), ui(new Ui::DeviceSettings)
{
    //
    ui->setupUi(this);
}

DeviceSettings::~DeviceSettings()
{
    //
    delete ui;
}

void DeviceSettings::on_btn_Save_clicked()
{
    //Save device settings
    emit UpdateConfig(ui->edit_DevID->text());
}

void DeviceSettings::on_btn_Load_clicked()
{
    //Load device settings
    emit LoadConfig();
}

void DeviceSettings::on_btn_Cancel_clicked()
{
    //Cancel (losing changes)
    this->close();
}

void DeviceSettings::UpdateInputs(QString DevID)
{
    //Updates the input boxes
    ui->edit_DevID->setText(DevID);
}
