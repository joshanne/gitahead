//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef ACCOUNT_H
#define ACCOUNT_H

#include "Repository.h"
#include <QNetworkAccessManager>
#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <QTcpServer>
#include <QTimer>

class AccountError;
class AccountProgress;

class Account : public QObject
{
  Q_OBJECT

public:
  enum Kind
  {
    GitHub,
    Bitbucket,
    Beanstalk,
    GitLab
  };

  struct Comment
  {
    QString body;
    QString author;
  };

  using Comments = QMap<QDateTime,Comment>;
  using FileComments = QMap<int,Comments>;

  struct CommitComments
  {
    Comments comments;
    QMap<QString,FileComments> files;
  };

  Account(const QString &username);
  virtual ~Account();

  AccountError *error() const { return mError; }
  AccountProgress *progress() const { return mProgress; }

  QString username() const { return mUsername; }
  QString password() const;

  bool hasPkcsFile() const;
  QString pkcsFile() const;
  void setPkcsFile(const QString &pkcsFile);

  bool hasPkcsKey() const;
  QString pkcsKey() const;
  void setPkcsKey(const QString &pkcsKey);

  bool hasCertFile() const;
  QString certFile() const;
  void setCertFile(const QString &certFile);

  bool hasCertKeyFile() const;
  QString certKeyFile() const;
  void setCertKeyFile(const QString &certKeyFile);

  bool hasCaCertFile() const;
  QString caCertFile() const;
  void setCaCertFile(const QString &caCertFile);

  bool hasCustomUrl() const;
  QString url() const;
  void setUrl(const QString &url);

  int repositoryCount() const;
  int indexOf(Repository *repo) const;
  Repository *repository(int index) const;
  Repository *addRepository(const QString &name, const QString &fullName);

  QString repositoryPath(int index) const;
  void setRepositoryPath(int index, const QString &path);

  virtual Kind kind() const = 0;
  virtual QString name() const = 0;
  virtual QString host() const = 0;
  virtual void connect(const QString &password = QString()) = 0;

  virtual void requestForkParents(Repository *repo) {}
  virtual void createPullRequest(
    Repository *repo,
    const QString &ownerRepo,
    const QString &title,
    const QString &body,
    const QString &head,
    const QString &base,
    bool canModify)
  {}

  virtual void requestComments(Repository *repo, const QString &oid) {}

  virtual void authorize();
  virtual bool isAuthorizeSupported();

  static QIcon icon(Kind kind);
  static QString name(Kind kind);
  static QString helpText(Kind kind);
  static QString defaultUrl(Kind kind);

signals:
  void repositoryAboutToBeAdded();
  void repositoryAdded();

  void repositoryPathChanged(int index, const QString &path);

  void commentsReady(
    Repository *repo,
    const QString &oid,
    const CommitComments &comments);

  void forkParentsReady(const QMap<QString,QString> &parents);
  void pullRequestError(const QString &name, const QString &message);

protected:
  void clearRepos();
  void startProgress();
  bool setHeaders(QNetworkRequest &request, const QString &defaultPassword);

  QString mPkcsFile;
  QString mPkcsKey;
  QString mCertFile;
  QString mCertKeyFile;
  QString mCaCertFile;
  QString mUrl;
  QString mUsername;
  QString mAccessToken;

  QList<Repository *> mRepos;
  mutable QMap<int,QString> mRepoPaths;

  AccountError *mError;
  AccountProgress *mProgress;
  QNetworkAccessManager mMgr;
  QTcpServer mRedirectServer;
};

class AccountError : public QObject
{
  Q_OBJECT

public:
  AccountError(Account *parent);

  Account *account() const;
  bool isValid() const { return !mText.isEmpty(); }

  QString text() const { return mText; }
  QString detailedText() const { return mDetailedText; }
  void setText(const QString &text, const QString &detailedText = QString());

private:
  QString mText;
  QString mDetailedText;
};

class AccountProgress : public QObject
{
  Q_OBJECT

public:
  AccountProgress(Account *parent);

  Account *account() const;
  bool isValid() const { return (mValue >= 0); }

  int value() const { return mValue; }

  void start();
  void finish();

signals:
  void started();
  void finished();
  void valueChanged(int value);

private:
  QTimer mTimer;
  int mValue = -1;
};

Q_DECLARE_METATYPE(Account::Kind);

#endif
