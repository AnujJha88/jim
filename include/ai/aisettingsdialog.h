#ifndef AISETTINGSDIALOG_H
#define AISETTINGSDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include "aiautocomplete.h"

class AISettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AISettingsDialog(QWidget *parent = nullptr);
    
    QString getBaseUrl() const;
    QString getApiKey() const;
    QString getModel() const;
    bool isEnabled() const;

    void setSettings(const QString &baseUrl, const QString &apiKey, const QString &model, bool enabled);

private slots:
    void onProviderChanged(int index);

private:
    QComboBox *providerCombo;
    QLineEdit *baseUrlEdit;
    QLineEdit *apiKeyEdit;
    QLineEdit *modelEdit;
    QCheckBox *enabledCheck;
};

#endif // AISETTINGSDIALOG_H
