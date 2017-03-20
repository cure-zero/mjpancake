#ifndef P_GLOBAL_H
#define P_GLOBAL_H

#include <QObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariant>
#include <QQmlEngine>
#include <QColor>

class PGlobal : public QObject
{
    Q_OBJECT
public:
    explicit PGlobal(QObject *parent = 0);
    ~PGlobal();

    Q_PROPERTY(QString version READ version NOTIFY versionChanged)
    Q_PROPERTY(QVariant backColors READ backColors WRITE setBackColors NOTIFY backColorsChanged)
    Q_PROPERTY(bool nightMode READ nightMode WRITE setNightMode NOTIFY nightModeChanged)
    Q_PROPERTY(QColor themeBack READ themeBack NOTIFY themeBackChanged)
    Q_PROPERTY(QColor themeText READ themeText NOTIFY themeTextChanged)
    Q_PROPERTY(QString savedUsername READ savedUsername WRITE setSavedUsername\
               NOTIFY savedUsernameChanged)
    Q_PROPERTY(bool savePassword READ savePassword WRITE setSavePassword NOTIFY savePasswordChanged)
    Q_PROPERTY(QString savedPassword READ savedPassword WRITE setSavedPassword\
               NOTIFY savedPasswordChanged)
    Q_PROPERTY(QVariantList redDots READ redDots WRITE setRedDots NOTIFY redDotsChanged)
    Q_PROPERTY(bool mute READ mute WRITE setMute NOTIFY muteChanged)
    Q_PROPERTY(QVariantMap photoMap READ photoMap NOTIFY photoMapChanged)

    Q_INVOKABLE void save();
    Q_INVOKABLE static void forceImmersive();
    Q_INVOKABLE static void systemNotify();

    static QString version();

    QVariant backColors() const;
    void setBackColors(const QVariant &v);

    bool nightMode() const;
    void setNightMode(bool v);

    QColor themeBack() const;
    QColor themeText() const;

    QString savedUsername() const;
    void setSavedUsername(const QString &username);

    bool savePassword() const;
    void setSavePassword(bool v);
    QString savedPassword() const;
    void setSavedPassword(const QString &password);

    QVariantList redDots() const;
    void setRedDots(const QVariantList &v);

    bool mute() const;
    void setMute(bool v);

    QVariantMap photoMap() const;
    Q_INVOKABLE void setPhoto(const QString &girlId, int value);

signals:
    void versionChanged(); // placeholder
    void backColorsChanged();
    void nightModeChanged();
    void themeBackChanged();
    void themeTextChanged();
    void savedUsernameChanged();
    void savePasswordChanged();
    void savedPasswordChanged();
    void redDotsChanged();
    void muteChanged();
    void photoMapChanged();

public slots:

private:
    void regulateRoot();

private:
    QJsonObject mRoot;
    QJsonObject mCachedPhotoMap; // Qt cannot modify nested object, fuck
};

QObject *pGlobalSingletonProvider(QQmlEngine *engine, QJSEngine *scriptEngine);

#endif // P_GLOBAL_H


