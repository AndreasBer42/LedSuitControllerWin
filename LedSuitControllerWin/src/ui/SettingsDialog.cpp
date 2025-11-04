#include "include/ui/SettingsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QHeaderView>

SettingsDialog::SettingsDialog(const QString& configFilePath, QWidget* parent)
    : QDialog(parent), configFilePath(configFilePath), tableWidget(new QTableWidget(this)) {
    setWindowTitle("Suit Settings");

    QVBoxLayout* layout = new QVBoxLayout(this);

    // Table setup
    tableWidget->setColumnCount(4);
    tableWidget->setHorizontalHeaderLabels({"Name", "IP Address", "Red", "Green"});
    tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget->setEditTriggers(QAbstractItemView::DoubleClicked);

    layout->addWidget(tableWidget);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    addButton = new QPushButton("Add Row", this);
    removeButton = new QPushButton("Remove Row", this);
    saveButton = new QPushButton("Save", this);

    connect(addButton, &QPushButton::clicked, this, &SettingsDialog::addRow);
    connect(removeButton, &QPushButton::clicked, this, &SettingsDialog::removeRow);
    connect(saveButton, &QPushButton::clicked, this, &SettingsDialog::saveConfig);

    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(removeButton);
    buttonLayout->addWidget(saveButton);

    layout->addLayout(buttonLayout);

    loadConfig();
}

SettingsDialog::~SettingsDialog() {}

void SettingsDialog::loadConfig() {
    QFile file(configFilePath);
    qWarning() << "Config file path:" << configFilePath;

    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error", "Failed to load config file.");
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        QMessageBox::warning(this, "Error", "Invalid config file format.");
        return;
    }

    QJsonArray array = doc.array();
    tableWidget->setRowCount(0);

    for (const QJsonValue& value : array) {
        if (!value.isObject()) continue;
        QJsonObject obj = value.toObject();

        int row = tableWidget->rowCount();
        tableWidget->insertRow(row);

        tableWidget->setItem(row, 0, new QTableWidgetItem(obj["name"].toString()));
        tableWidget->setItem(row, 1, new QTableWidgetItem(obj["ip"].toString()));

        QCheckBox* redCheckBox = new QCheckBox();
        redCheckBox->setChecked(obj["color"].toObject()["red"].toBool());
        tableWidget->setCellWidget(row, 2, redCheckBox);

        QCheckBox* greenCheckBox = new QCheckBox();
        greenCheckBox->setChecked(obj["color"].toObject()["green"].toBool());
        tableWidget->setCellWidget(row, 3, greenCheckBox);
    }
}

void SettingsDialog::saveConfig() {
    QJsonArray array;

    for (int row = 0; row < tableWidget->rowCount(); ++row) {
        QJsonObject obj;
        obj["name"] = tableWidget->item(row, 0)->text();
        obj["ip"] = tableWidget->item(row, 1)->text();

        QJsonObject color;
        color["red"] = qobject_cast<QCheckBox*>(tableWidget->cellWidget(row, 2))->isChecked();
        color["green"] = qobject_cast<QCheckBox*>(tableWidget->cellWidget(row, 3))->isChecked();
        obj["color"] = color;

        array.append(obj);
    }

    QFile file(configFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Error", "Failed to save config file.");
        return;
    }

    QJsonDocument doc(array);
    file.write(doc.toJson());
    file.close();

    QMessageBox::information(this, "Success", "Config saved successfully.");
}

void SettingsDialog::addRow() {
    int row = tableWidget->rowCount();
    tableWidget->insertRow(row);

    tableWidget->setItem(row, 0, new QTableWidgetItem("New Suit"));
    tableWidget->setItem(row, 1, new QTableWidgetItem("192.168.1.XXX"));

    tableWidget->setCellWidget(row, 2, new QCheckBox());
    tableWidget->setCellWidget(row, 3, new QCheckBox());
}

void SettingsDialog::removeRow() {
    int currentRow = tableWidget->currentRow();
    if (currentRow >= 0) {
        tableWidget->removeRow(currentRow);
    }
}

QJsonArray SettingsDialog::getConfigData() const {
    QJsonArray array;

    for (int row = 0; row < tableWidget->rowCount(); ++row) {
        QJsonObject obj;
        obj["name"] = tableWidget->item(row, 0)->text();
        obj["ip"] = tableWidget->item(row, 1)->text();

        QJsonObject color;
        color["red"] = qobject_cast<QCheckBox*>(tableWidget->cellWidget(row, 2))->isChecked();
        color["green"] = qobject_cast<QCheckBox*>(tableWidget->cellWidget(row, 3))->isChecked();
        obj["color"] = color;

        array.append(obj);
    }

    return array;
}

