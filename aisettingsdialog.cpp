#include "aisettingsdialog.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>

AISettingsDialog::AISettingsDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("AI Autocomplete Settings");
    setMinimumWidth(400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QFormLayout *formLayout = new QFormLayout();

    enabledCheck = new QCheckBox("Enable AI Autocomplete");
    formLayout->addRow("", enabledCheck);

    providerCombo = new QComboBox();
    auto providers = AIAutocomplete::getProviders();
    for (const auto &p : providers) {
        providerCombo->addItem(p.name + (p.isFree ? " (Free)" : ""), p.baseUrl);
    }
    formLayout->addRow("Provider:", providerCombo);

    baseUrlEdit = new QLineEdit();
    formLayout->addRow("Base URL:", baseUrlEdit);

    apiKeyEdit = new QLineEdit();
    apiKeyEdit->setEchoMode(QLineEdit::Password);
    formLayout->addRow("API Key:", apiKeyEdit);

    modelEdit = new QLineEdit();
    formLayout->addRow("Model ID:", modelEdit);

    mainLayout->addLayout(formLayout);

    connect(providerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AISettingsDialog::onProviderChanged);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    // Initial state
    if (providerCombo->count() > 0) onProviderChanged(0);
}

void AISettingsDialog::onProviderChanged(int index)
{
    QString url = providerCombo->itemData(index).toString();
    baseUrlEdit->setText(url);
    
    auto providers = AIAutocomplete::getProviders();
    if (index >= 0 && index < providers.size()) {
        modelEdit->setText(providers[index].defaultModel);
    }
}

QString AISettingsDialog::getBaseUrl() const { return baseUrlEdit->text(); }
QString AISettingsDialog::getApiKey() const { return apiKeyEdit->text(); }
QString AISettingsDialog::getModel() const { return modelEdit->text(); }
bool AISettingsDialog::isEnabled() const { return enabledCheck->isChecked(); }

void AISettingsDialog::setSettings(const QString &baseUrl, const QString &apiKey, const QString &model, bool enabled)
{
    baseUrlEdit->setText(baseUrl);
    apiKeyEdit->setText(apiKey);
    modelEdit->setText(model);
    enabledCheck->setChecked(enabled);
    
    // Try to match provider
    for (int i = 0; i < providerCombo->count(); ++i) {
        if (providerCombo->itemData(i).toString() == baseUrl) {
            providerCombo->setCurrentIndex(i);
            break;
        }
    }
}
