#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QString>
#include <QTableWidget>
#include <QPushButton>
#include <QJsonArray>

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(const QString& configFilePath, QWidget* parent = nullptr);
    ~SettingsDialog();

    QJsonArray getConfigData() const;

private slots:
    void loadConfig();
    void saveConfig();
    void addRow();
    void removeRow();

private:
    QString configFilePath;
    QTableWidget* tableWidget;
    QPushButton* addButton;
    QPushButton* removeButton;
    QPushButton* saveButton;
};

#endif // SETTINGSDIALOG_H

