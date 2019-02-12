//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "GitLab.h"
#include "Repository.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QSslConfiguration>
#include <QSslCertificate>
#include <QSslKey>
#include <QFile>
#include <QSettings>
#include "conf/Settings.h"

namespace {

const QString kContentType = "application/json";
const QString kProjectsFmt = "/projects?membership=true&private_token=%1";

} // anon. namespace

GitLab::GitLab(const QString &username)
  : Account(username)
{
  QObject::connect(&mMgr, &QNetworkAccessManager::finished, this,
  [this](QNetworkReply *reply) {
    reply->deleteLater();

    if (reply->error() == QNetworkReply::NoError) {
      QJsonArray array = QJsonDocument::fromJson(reply->readAll()).array();
      for (int i = 0; i < array.size(); ++i) {
        QJsonObject obj = array.at(i).toObject();
        QString name = obj.value("path").toString();
        QString fullName = obj.value("path_with_namespace").toString();
        QString httpUrl = obj.value("http_url_to_repo").toString();
        QString sshUrl = obj.value("ssh_url_to_repo").toString();

        Repository *repo = addRepository(name, fullName);
        repo->setUrl(Repository::Https, httpUrl);
        repo->setUrl(Repository::Ssh, sshUrl);
      }
    } else {
      mError->setText(tr("Connection failed"), reply->errorString());
    }

    mProgress->finish();
  });
}

Account::Kind GitLab::kind() const
{
  return Account::GitLab;
}

QString GitLab::name() const
{
  return QStringLiteral("GitLab");
}

QString GitLab::host() const
{
  return QStringLiteral("gitlab.com");
}

void GitLab::connect(const QString &defaultPassword)
{
  clearRepos();

  QSslConfiguration sslConfig;
  QSslKey sslKey;
  QSslCertificate sslCertificate;
  QList<QSslCertificate> sslCaCertificates;

  QString token = defaultPassword;
  if (token.isEmpty())
    token = password();

  if (token.isEmpty()) {
    mError->setText(tr("Authentication failed"));
    return;
  }

  if (hasPkcsFile())
  {

	  bool retval;
	  QFile sslPkcsFile(pkcsFile());
	  QString sslPassphrase = pkcsKey();

	  if (sslPkcsFile.open(QFile::ReadOnly))
	  {
		retval = QSslCertificate::importPkcs12(&sslPkcsFile, &sslKey, &sslCertificate, &sslCaCertificates, QByteArray::fromStdString(sslPassphrase.toStdString()));
		if (retval) {
			sslConfig.setLocalCertificate(sslCertificate);
			sslConfig.setPrivateKey(sslKey);
			//sslConfig.setCaCertificates(sslCaCertificates);
		}
	  }
	  
	  log(QString("Importing of PKCS12 was %1").arg(retval ? "Successful!" : "Unsuccessful"));
	  log(QString("URL: %1").arg(url()));
	  log(QString("PKCS12 File: %1").arg(pkcsFile()));
	  log(QString("PKCS12 Key: %1").arg(pkcsKey()));
  }

  log(QString("Issuer: %1").arg(sslCertificate.issuerDisplayName()));

  QNetworkRequest request(url() + kProjectsFmt.arg(token));
  request.setHeader(QNetworkRequest::ContentTypeHeader, kContentType);
  request.setSslConfiguration(sslConfig);
  mMgr.get(request);
  startProgress();
}

QString GitLab::defaultUrl()
{
  return QStringLiteral("https://gitlab.com/api/v4");
}


void GitLab::log(const QString &text)
{
	QFile file(Settings::tempDir().filePath("gitlab.log"));
	if (!file.open(QFile::WriteOnly | QIODevice::Append))
		return;

	QString time = QTime::currentTime().toString(Qt::ISODateWithMs);
	QTextStream(&file) << time << " - " << text << endl;
}