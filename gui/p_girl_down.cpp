#include "p_girl_down.h"
#include "p_global.h"

#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QNetworkReply>
#include <QDebug>



using StrConst = const char * const;
static StrConst URL_ISSUE_51 = "https://api.github.com/repos/rolevax/libsaki/issues/51/comments";
static StrConst URL_REPO_DIR_FMT = "https://api.github.com/repos/%1/contents/";

QJsonDocument replyToJson(QNetworkReply *reply)
{
    QString str = reply->readAll();
    return QJsonDocument::fromJson(str.toUtf8());
}



PGirlDown::PGirlDown(QObject *parent)
    : QObject(parent)
{
    connect(&mNet, &QNetworkAccessManager::finished,
            this, &PGirlDown::onNetReply,
            Qt::QueuedConnection);
}

PGirlDown::~PGirlDown()
{
    httpAbortAll();
}

void PGirlDown::fetchSignedRepos()
{
    mTask = std::make_unique<TaskFetchRepoList>(*this);
}

///
/// \brief Starat to download a girl repo, discard all current downloads
/// \param shortAddr GitHub repo address in form "username/repo-name"
///
void PGirlDown::downloadRepo(QString shortAddr)
{
    mTask = std::make_unique<TaskDownloadGirls>(*this, shortAddr);
}

void PGirlDown::cancelDownload()
{
    mTask = nullptr;
    httpAbortAll();
}

void PGirlDown::onNetReply(QNetworkReply *reply)
{
    mReplies.remove(reply);
    reply->deleteLater();

    if (mTask != nullptr) {
        bool working = mTask->recv(reply);
        if (!working) {
            mTask = nullptr;
            httpAbortAll();
        }
    }
}

void PGirlDown::httpGet(QUrl url)
{
    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "rolevax");
    mReplies.insert(mNet.get(request));
}

void PGirlDown::httpAbortAll()
{
    for (QNetworkReply *reply : mReplies)
        reply->abort();

    mReplies.clear();
}

PGirlDown::Task::Task(PGirlDown &girlDown)
    : mGirlDown(girlDown)
{
}

PGirlDown::TaskFetchRepoList::TaskFetchRepoList(PGirlDown &girlDown)
    : Task(girlDown)
{
    mGirlDown.httpGet(QUrl(URL_ISSUE_51));
}

bool PGirlDown::TaskFetchRepoList::recv(QNetworkReply *reply)
{
    if (reply->error()) {
        qDebug() << reply->errorString();
        return false;
    }

    QString reqUrl = reply->request().url().toString();
    if (reqUrl == URL_ISSUE_51)
        return recvRepoList(reply);

    return false;
}

bool PGirlDown::TaskFetchRepoList::recvRepoList(QNetworkReply *reply)
{
    QJsonDocument replyDoc = replyToJson(reply);
    QVariantList issues = replyDoc.array().toVariantList();

    QVariantList repos;
    for (QVariant issueVar : issues) {
        QVariantMap issue = issueVar.toMap();
        QString bodyStr = issue["body"].toString();
        QJsonDocument bodyDoc = QJsonDocument::fromJson(bodyStr.toUtf8());
        if (bodyDoc.isObject()) {
            QJsonObject bodyObj = bodyDoc.object();
            repos << bodyObj.toVariantMap();
        }
    }

    emit mGirlDown.signedReposReplied(repos);
    return false; // FUCK, calc update status
}

PGirlDown::TaskDownloadGirls::TaskDownloadGirls(PGirlDown &girlDown, const QString &shortAddr)
    : Task(girlDown)
    , mShortAddr(shortAddr)
{
    QString repoAddr = QString(URL_REPO_DIR_FMT).arg(shortAddr);
    mGirlDown.httpGet(repoAddr);
    emit mGirlDown.repoDownloadProgressed(0);
}

bool PGirlDown::TaskDownloadGirls::recv(QNetworkReply *reply)
{
    if (reply->error()) {
        qDebug() << reply->errorString();
        emit mGirlDown.repoDownloadProgressed(-1);
        return false;
    }

    if (!mGotDir) {
        mGotDir = true;
        return recvRepoDir(reply);
    }

    return recvFile(reply);
}

bool PGirlDown::TaskDownloadGirls::recvRepoDir(QNetworkReply *reply)
{
    emit mGirlDown.repoDownloadProgressed(1);
    QJsonDocument replyDoc = replyToJson(reply);
    if (!replyDoc.isArray()) {
        emit mGirlDown.repoDownloadProgressed(-1);
        return false;
    }

    QJsonArray files = replyDoc.array();
    QStringList targets;
    for (auto value : files) {
        QJsonObject file = value.toObject();
        if (file["type"].toString() != "file")
            continue;

        QString name = file["name"].toString();
        if (name.endsWith(".girl.json") || name.endsWith(".girl.lua")) {
            QString addr = file["download_url"].toString();
            targets << addr;
        }
    }

    mTotalFiles = targets.size();
    for (const auto &addr : targets)
        mGirlDown.httpGet(addr);

    return true;
}

bool PGirlDown::TaskDownloadGirls::recvFile(QNetworkReply *reply)
{
    // uri should be in format ".../user/repo/branch/filename"
    QStringList split = reply->request().url().toString().split("/");
    QString filename = split.at(split.size() - 1);
    QString repo = split.at(split.size() - 3);
    QString user = split.at(split.size() - 4);
    if (mShortAddr != user + "/" + repo) {
        emit mGirlDown.repoDownloadProgressed(-1);
        return false;
    }

    QString dirSuffix = "github.com/" + mShortAddr;
    qDebug() << "got: " << (dirSuffix + '/' + filename);

    QString content = reply->readAll();
    QFile file(PGlobal::editPath(filename, dirSuffix));
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    file.write(content.toUtf8());
    mCompletedFiles++;

    if (mCompletedFiles == mTotalFiles) {
        QJsonObject meta;

        QFile jsonFile(PGlobal::editPath("meta.json", dirSuffix));
        bool ok = jsonFile.open(QIODevice::ReadOnly | QIODevice::Text);
        if (ok) {
            auto doc = QJsonDocument::fromJson(jsonFile.readAll());
            jsonFile.close();
            if (doc.isObject())
                meta = doc.object();
        }

        meta["updated_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

        jsonFile.open(QIODevice::WriteOnly | QIODevice::Text);
        jsonFile.write(QJsonDocument(meta).toJson());
        emit mGirlDown.repoDownloadProgressed(100);
        return false;
    }

    double rate = static_cast<double>(mCompletedFiles) / mTotalFiles;
    int percent = std::min(static_cast<int>(rate * 100), 99);
    emit mGirlDown.repoDownloadProgressed(percent);
    return true;
}
