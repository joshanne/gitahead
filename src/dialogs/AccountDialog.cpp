//
//          Copyright (c) 2018, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "AccountDialog.h"
#include "cred/CredentialHelper.h"
#include "host/Accounts.h"
#include "ui/ExpandButton.h"
#include <QApplication>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QCheckBox>

AccountDialog::AccountDialog(Account *account, QWidget *parent)
  : QDialog(parent)
{
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(tr("Add Remote Account"));

  mHost = new QComboBox(this);
  mHost->setMinimumWidth(mHost->sizeHint().width() * 2);
  mHost->addItem("GitHub", Account::GitHub);
  mHost->addItem("Bitbucket", Account::Bitbucket);
  mHost->addItem("Beanstalk", Account::Beanstalk);
  mHost->addItem("GitLab", Account::GitLab);

  Account::Kind kind = account ? account->kind() : Account::GitHub;
  setKind(kind);

  mUsername = new QLineEdit(this);
  mUsername->setText(account ? account->username() : QString());
  connect(mUsername, &QLineEdit::textChanged,
          this, &AccountDialog::updateButtons);

  mPassword = new QLineEdit(this);
  mPassword->setEchoMode(QLineEdit::Password);
  mPassword->setText(account ? account->password() : QString());
  connect(mPassword, &QLineEdit::textChanged,
          this, &AccountDialog::updateButtons);

  auto signal = QOverload<int>::of(&QComboBox::currentIndexChanged);
  mLabel = new QLabel(Account::helpText(kind), this);
  mLabel->setWordWrap(true);
  mLabel->setOpenExternalLinks(true);
  mLabel->setVisible(!mLabel->text().isEmpty());
  connect(mHost, signal, [this] {
    Account::Kind kind =
    static_cast<Account::Kind>(mHost->currentData().toInt());
    mLabel->setText(Account::helpText(kind));
    mLabel->setVisible(!mLabel->text().isEmpty());
  });

  ExpandButton *expand = new ExpandButton(this);
  QWidget *advanced = new QWidget(this);
  advanced->setVisible(false);

  QFormLayout *form = new QFormLayout;
  form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
  form->addRow(tr("Host:"), mHost);
  form->addRow(tr("Username:"), mUsername);
  form->addRow(tr("Password:"), mPassword);
  form->addRow(mLabel);
  form->addRow(tr("Advanced:"), expand);

  mUrl = new QLineEdit(advanced);
  mUrl->setText(account ? account->url() : Account::defaultUrl(kind));
  connect(mHost, signal, [this] {
    Account::Kind kind =
    static_cast<Account::Kind>(mHost->currentData().toInt());
    mUrl->setText(Account::defaultUrl(kind));
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    resize(sizeHint());
  });

  QHBoxLayout *lPkcs = new QHBoxLayout();
  QCheckBox *mPkcsEnabled = new QCheckBox();
  mPkcsFile = new QLineEdit();
  mPkcsFile->setText(account ? account->pkcsFile() : "");
  mPkcsEnabled->setChecked(account ? account->hasPkcsFile() : false);
  lPkcs->addWidget(mPkcsEnabled);
  lPkcs->addWidget(mPkcsFile);

  QHBoxLayout *lPkcsKey = new QHBoxLayout();
  QCheckBox *mPkcsKeyEnabled = new QCheckBox();
  mPkcsKey = new QLineEdit();
  mPkcsKey->setText(account ? account->pkcsKey() : "");
  mPkcsKeyEnabled->setChecked(account ? account->hasPkcsKey() : false);
  lPkcsKey->addWidget(mPkcsKeyEnabled);
  lPkcsKey->addWidget(mPkcsKey);

  QHBoxLayout *lCert = new QHBoxLayout();
  QCheckBox *mCertEnabled = new QCheckBox();
  mCertFile = new QLineEdit();
  mCertFile->setText(account ? account->certFile() : "");
  mCertEnabled->setChecked(account ? account->hasCertFile() : false);
  lCert->addWidget(mCertEnabled);
  lCert->addWidget(mCertFile);

  QHBoxLayout *lCertKey = new QHBoxLayout();
  QCheckBox *mCertKeyEnabled = new QCheckBox();
  mCertKeyFile = new QLineEdit();
  mCertKeyFile->setText(account ? account->certFile() : "");
  mCertKeyEnabled->setChecked(account ? account->hasCertKeyFile() : false);
  lCertKey->addWidget(mCertKeyEnabled);
  lCertKey->addWidget(mCertKeyFile);

  QHBoxLayout *lCaCert = new QHBoxLayout();
  QCheckBox *mCaCertEnabled = new QCheckBox();
  mCaCertFile = new QLineEdit();
  mCaCertFile->setText(account ? account->certFile() : "");
  mCaCertEnabled->setChecked(account ? account->hasCaCertFile() : false);
  lCaCert->addWidget(mCaCertEnabled);
  lCaCert->addWidget(mCaCertFile);

  QFormLayout *advancedForm = new QFormLayout(advanced);
  advancedForm->setContentsMargins(-1,0,0,0);
  advancedForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
  advancedForm->addRow(tr("URL:"), mUrl);
  advancedForm->addRow(tr("PKCS12 File:"), lPkcs);
  advancedForm->addRow(tr("PKCS12 Password:"), lPkcsKey);
  advancedForm->addRow(tr("Certificate File:"), lCert);
  advancedForm->addRow(tr("Certificate Key File:"), lCertKey);
  advancedForm->addRow(tr("CA Certificate File:"), lCaCert);


  connect(expand, &ExpandButton::toggled, [this, advanced](bool checked) {
    advanced->setVisible(checked);
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    resize(sizeHint());
  });

  QDialogButtonBox::StandardButtons buttons =
    QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
  mButtons = new QDialogButtonBox(buttons, this);
  mButtons->button(QDialogButtonBox::Ok)->setEnabled(false);
  connect(mButtons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(mButtons, &QDialogButtonBox::rejected, this, &QDialog::reject);

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->addLayout(form);
  layout->addWidget(advanced);
  layout->addWidget(mButtons);

  updateButtons();
}

void AccountDialog::accept()
{
  // Validate account.
  Account::Kind kind =
    static_cast<Account::Kind>(mHost->currentData().toInt());
  QString username = mUsername->text();
  QString url = (mUrl->text() != Account::defaultUrl(kind)) ?
    mUrl->text() : QString();

  if (Account *account = Accounts::instance()->lookup(username, kind)) {
    QMessageBox mb(QMessageBox::Information, tr("Replace?"),
                   tr("An account of this type already exists."));
    mb.setInformativeText(tr("Would you like to replace the previous account?"));
    QPushButton *remove =
      mb.addButton(tr("Replace"), QMessageBox::AcceptRole);
    mb.addButton(tr("Cancel"), QMessageBox::RejectRole);
    mb.setDefaultButton(remove);
    mb.exec();

    if (mb.clickedButton() != remove)
      return;

    Accounts::instance()->removeAccount(account);
  }

  Account *account = Accounts::instance()->createAccount(kind, username, url);
  account->setPkcsFile(mPkcsFile->text());
  account->setPkcsKey(mPkcsKey->text());
  account->setCertFile(mCertFile->text());
  account->setCertKeyFile(mCertKeyFile->text());
  account->setCaCertFile(mCaCertFile->text());

  AccountProgress *progress = account->progress();
  connect(progress, &AccountProgress::finished, this, [this, account] {
    AccountError *error = account->error();
    if (error->isValid()) {
      QString text = error->text();
      QString title = tr("Connection Failed");
      QMessageBox msg(QMessageBox::Warning, title, text, QMessageBox::Ok);
      msg.setInformativeText(error->detailedText());
      msg.exec();

      Accounts::instance()->removeAccount(account);
      return;
    }

    // Store password.
    QUrl url;
    url.setScheme("https");
    url.setHost(account->host());

    CredentialHelper *helper = CredentialHelper::instance();
    helper->store(url.toString(), account->username(), mPassword->text());

    QDialog::accept();
  });

  // Start asynchronous connection.
  account->connect(mPassword->text());
}

void AccountDialog::setKind(Account::Kind kind)
{
  mHost->setCurrentIndex(mHost->findData(kind));
}

void AccountDialog::updateButtons()
{
  mButtons->button(QDialogButtonBox::Ok)->setEnabled(
    !mUsername->text().isEmpty() && !mPassword->text().isEmpty());
}
