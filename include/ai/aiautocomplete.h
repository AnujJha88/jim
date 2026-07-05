#ifndef AIAUTOCOMPLETE_H
#define AIAUTOCOMPLETE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QPlainTextEdit>

class AIAutocomplete : public QObject
{
    Q_OBJECT

public:
    struct Provider {
        QString name;
        QString baseUrl;
        QString defaultModel;
        bool isFree;
    };

    explicit AIAutocomplete(QObject *parent = nullptr);
    static QList<Provider> getProviders();
    
    void setProvider(const QString &baseUrl, const QString &apiKey, const QString &model);
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }

    void trigger(QPlainTextEdit *editor);

signals:
    void suggestionReady(const QString &suggestion);
    void errorOccurred(const QString &error);

private slots:
    void onReplyFinished(QNetworkReply *reply);
    void onTimeout();

private:
    QNetworkAccessManager *m_networkManager;
    QTimer *m_debounceTimer;
    QPlainTextEdit *m_currentEditor;
    
    bool m_enabled;
    QString m_baseUrl;
    QString m_apiKey;
    QString m_model;
    
    void fetchAutocomplete();
};

#endif // AIAUTOCOMPLETE_H
