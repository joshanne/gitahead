//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "AboutDialog.h"
#include "IconLabel.h"
#include "conf/Settings.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPointer>
#include <QSettings>
#include <QTabBar>
#include <QTextBrowser>
#include <QVBoxLayout>

#ifdef Q_OS_MAC
#define TAB_BAR_ALIGNMENT Qt::AlignCenter
#else
#define TAB_BAR_ALIGNMENT Qt::Alignment()
#endif

namespace {

const QString kEmail = "support@gitahead.com";

const QString kUrl =
  "https://stackoverflow.com/questions/tagged/gitahead?sort=frequent";

const QString kSubtitleFmt =
  "<h4 style='margin-top: 0px; color: gray'>%2</h4>";

const QString kTextFmt =
  "<p style='white-space: nowrap'><b style='font-size: large'>%1 v%2</b> - %3 - %4<br>"
  "Copyright © 2016-2018 Scientific Toolworks, Inc. All rights reserved.</p>"
  "<p> If you have a question that might benefit the community, consider "
  "asking it on <a href='%5'>Stack Overflow</a> by including 'gitahead' in the "
  "tags. Otherwise, contact us at <a href='mailto:%6'>%6</a>";

const QString kStyleSheet =
  "h3 {"
  "  text-decoration: underline"
  "}"
  "h4 {"
  "  color: #696969"
  "}";

const Qt::TextInteractionFlags kTextFlags =
  Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse;

} // anon. namespace

AboutDialog::AboutDialog(QWidget *parent)
  : QDialog(parent)
{
  QString name = QCoreApplication::applicationName();
  QString version = QCoreApplication::applicationVersion();

  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(tr("About %1").arg(name));

  QIcon icon(":/GitAhead.iconset/icon_128x128.png");
  IconLabel *iconLabel = new IconLabel(icon, 128, 128, this);

  QIcon title(":/logo-type.png");
  IconLabel *titleLabel = new IconLabel(title, 163, 38, this);

  QString subtitleText = kSubtitleFmt.arg(tr("Understand your history!"));
  QLabel *subtitle = new QLabel(subtitleText, this);
  subtitle->setAlignment(Qt::AlignHCenter);

  QVBoxLayout *left = new QVBoxLayout;
  left->addWidget(iconLabel);
  left->addWidget(titleLabel);
  left->addWidget(subtitle);
  left->addStretch();

  QString revision = GITAHEAD_BUILD_REVISION;
  QDateTime dateTime = QDateTime::fromString(GITAHEAD_BUILD_DATE, Qt::ISODate);
  QString date = dateTime.date().toString(Qt::DefaultLocaleLongDate);
  QString text = kTextFmt.arg(name, version, date, revision, kUrl, kEmail);
  QLabel *label = new QLabel(text, this);
  label->setWordWrap(true);
  label->setTextInteractionFlags(kTextFlags);
  connect(label, &QLabel::linkActivated, QDesktopServices::openUrl);

  QTabBar *tabs = new QTabBar(this);
  tabs->setTabData(tabs->addTab(tr("Changelog")), "changelog.html");
  tabs->setTabData(tabs->addTab(tr("Acknowledgments")), "acknowledgments.html");
  tabs->setTabData(tabs->addTab(tr("Privacy")), "privacy.html");

  QTextBrowser *browser = new QTextBrowser(this);
  browser->setOpenLinks(false);
  browser->document()->setDocumentMargin(12);
  browser->document()->setDefaultStyleSheet(kStyleSheet);
  connect(browser, &QTextBrowser::anchorClicked, [this](const QUrl &url) {
    if (url.isLocalFile() &&
        QFileInfo(url.toLocalFile()).fileName() == "opt-out") {
      QSettings().setValue("tracking/enabled", false);
      QString text =
        tr("Usage tracking has been disabled. Restart "
           "the app for changes to take effect.");
      QMessageBox::information(this, tr("Usage Tracking Disabled"), text);
      return;
    }

    QDesktopServices::openUrl(url);
  });

  connect(tabs, &QTabBar::currentChanged, [tabs, browser](int index) {
    QString url = Settings::docDir().filePath(tabs->tabData(index).toString());
    browser->setSource(QUrl::fromLocalFile(url));
  });

  // Load the initial content.
  emit tabs->currentChanged(tabs->currentIndex());

  QVBoxLayout *right = new QVBoxLayout;
  right->setSpacing(0);
  right->addWidget(label);
  right->addSpacing(12);
  right->addWidget(tabs, 0, TAB_BAR_ALIGNMENT);
  right->addWidget(browser);

  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->addLayout(left);
  layout->addSpacing(8);
  layout->addLayout(right);
}

void AboutDialog::openSharedInstance()
{
  static QPointer<AboutDialog> dialog;
  if (dialog) {
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
    return;
  }

  dialog = new AboutDialog;
  dialog->show();
}
