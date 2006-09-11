/* This file is part of Strigi Desktop Search
 *
 * Copyright (C) 2006 Jos van den Oever <jos@vandenoever.info>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#include "dlgpreferences.h"
#include "simplesearchgui.h"
#include "searchtabs.h"
#include "socketclient.h"
#include <QtGui/QListWidget>
#include <QtGui/QListWidgetItem>
#include <QtGui/QLineEdit>
#include <QtGui/QVBoxLayout>
#include <QtGui/QStackedWidget>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QFileDialog>
#include <QtGui/QComboBox>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QAction>
#include <QtCore/QString>
#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>
#include <string>
#include <map>
#include <vector>
using namespace std;

SimpleSearchGui::SimpleSearchGui (QWidget * parent, Qt::WFlags flags)
    : QMainWindow (parent, flags)
{
    mainview = new QStackedWidget();

    QWidget* statuswidget = new QWidget();
    QVBoxLayout *statuslayout = new QVBoxLayout;
    statusview = new QLabel();
    statusview->setAlignment(Qt::AlignTop);
    statusview->setMargin(25);
    indexing = false;
    running = false;
    starting = true;
    indexeddirs = new QListWidget();
    adddir = new QPushButton("add directory");
    removedir = new QPushButton("remove directory");
    toggleindexing = new QPushButton("start indexing");
    toggledaemon = new QPushButton("stop daemon");
    statuslayout->addWidget(statusview);
    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addWidget(toggledaemon);
    hlayout->addWidget(toggleindexing);
    statuslayout->addLayout(hlayout);
    vector<string> backends = ClientInterface::getBackEnds();
    if (backends.size() > 1) {
        backendsList = new QComboBox();
        for (uint i = 0; i< backends.size(); ++i) {
            QString backend = backends[i].c_str();
            backendsList->insertItem(i, backend);
        }
        statuslayout->addWidget(backendsList);
    } else {
        backendsList = 0;
    }
    statuslayout->addWidget(indexeddirs);
    hlayout = new QHBoxLayout;
    hlayout->addWidget(adddir);
    hlayout->addWidget(removedir);
    statuslayout->addLayout(hlayout);
    statuswidget->setLayout(statuslayout);

    tabs = new SearchTabs();
    tabs->addTab("kde", "kde");
    tabs->addTab("msg", "mimetype:message/*");
    tabs->addTab("mail", "mimetype:text/x-mail");
    tabs->addTab("audio", "mimetype:audio/*");
    tabs->addTab("other", "-kde -mimetype:mesage/* -mimetype:text/x-mail -mimetype:audio/*");
    tabs->addTab("all", "");
    mainview->addWidget(tabs);
    mainview->addWidget(statuswidget);
    mainview->setCurrentIndex(1);

    queryfield = new QLineEdit();
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(mainview);
    layout->addWidget(queryfield);
   
    centralview = new QWidget();
    centralview->setLayout(layout);
    setCentralWidget(centralview);
   
    createActions();
    createMenus();
   
    connect(queryfield, SIGNAL(textChanged(const QString&)),
        this, SLOT(query(const QString&)));
    connect(toggleindexing, SIGNAL(clicked()),
        this, SLOT(toggleIndexing()));
    connect(toggledaemon, SIGNAL(clicked()),
        this, SLOT(toggleDaemon()));
    connect(adddir, SIGNAL(clicked()),
        this, SLOT(addDirectory()));
    connect(removedir, SIGNAL(clicked()),
        this, SLOT(removeDirectory()));
    queryfield->setFocus(Qt::ActiveWindowFocusReason);

    socketfile = getenv("HOME");
    socketfile += "/.strigi/socket";

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateStatus()));
    timer->start(2000);
    updateStatus();
}
void
SimpleSearchGui::createMenus() {
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(fileExitAct);
   
    editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(editPreferenceAct);
}
void
SimpleSearchGui::createActions() {
    fileExitAct = new QAction(tr("&Exit"), this);
    fileExitAct->setShortcut(tr("Ctrl+Q"));
    fileExitAct->setStatusTip(tr("Quit the program"));
    connect(fileExitAct, SIGNAL(triggered()), this, SLOT(close()));
   
    editPreferenceAct = new QAction(tr("Edit Preferences"), this);
    editPreferenceAct->setStatusTip(tr("Edit program preferences"));
    connect(editPreferenceAct, SIGNAL(triggered()), this, SLOT(editPreferences()));
}
void
SimpleSearchGui::query(const QString& item) {
    if (item.length()) {
        tabs->setQuery(item);
    }
    QString query = item.trimmed();
    if (query.length() == 0) {
        mainview->setCurrentIndex(1);
    } else {
        mainview->setCurrentIndex(0);
    }
}
void
SimpleSearchGui::updateStatus() {
    static bool first = true;
    static bool attemptedstart = false;
    if (!first && !statusview->isVisible()) return;
    first = false;
    SocketClient client;
    client.setSocketName(socketfile.c_str());
    map<string,string> s = client.getStatus();
    if (s.size() == 0) {
        running = false;
        /*if (!attemptedstart) {
            s["Status"] = "Starting daemon";
            startDaemon();
            attemptedstart = true;
        } else {*/
            s["Status"] = "Daemon is not running";
        //}
    } else {
        attemptedstart = true;
        starting = false;
        running = true;
        if (indexeddirs->count() == 0) {
            updateDirectories();
        }
    }
    toggleindexing->setEnabled(running);
    adddir->setEnabled(running);
    removedir->setEnabled(running);
    queryfield->setEnabled(running);
    toggledaemon->setText((running)?"stop daemon":"start daemon");
    if (backendsList) {
        backendsList->setEnabled(!running);
    }
    bool idxng = s["Status"] == "indexing";
    if (idxng != indexing) {
        indexing = idxng;
        toggleindexing->setText((indexing)?"stop indexing":"start indexing");
    }
   
    map<string,string>::const_iterator i;
    QString text;
    for (i = s.begin(); i != s.end(); ++i) {
        text += i->first.c_str();
        text += ":\t";
        text += i->second.c_str();
        text += '\n';
    }
    statusview->setText(text);
}
void
SimpleSearchGui::startDaemon() {
    starting = true;
    // try to start the daemon
    QFileInfo exe = QCoreApplication::applicationDirPath()
    + "/../../daemon/strigidaemon";
    QStringList args;
    if (backendsList) {
        args += backendsList->currentText();
    }
    if (exe.exists()) {
    // start not installed version
    QProcess::startDetached(exe.absoluteFilePath(), args);
    } else {
        exe = QCoreApplication::applicationDirPath()+"/strigidaemon";
        if (exe.exists()) {
            QProcess::startDetached(exe.absoluteFilePath(), args);
        } else {
        // start installed version
        QProcess::startDetached("strigidaemon");
        }
    }
}
void
SimpleSearchGui::toggleDaemon() {
    if (running) {
        SocketClient client;
        client.setSocketName(socketfile.c_str());
        client.stopDaemon();
        indexeddirs->clear();
    } else {
        startDaemon();
    }
}
void
SimpleSearchGui::toggleIndexing() {
    SocketClient client;
    client.setSocketName(socketfile.c_str());
    if (indexing) {
        client.stopIndexing();
    } else {
        client.startIndexing();
    }
}
void
SimpleSearchGui::addDirectory() {
    // open file dialog
    QString dir = QFileDialog::getExistingDirectory (this);
    if (dir.size() <= 0) return;
    for (int i=0; i<indexeddirs->count(); ++i) {
        QString text = indexeddirs->item(i)->text();
        if (dir.startsWith(text)) {
            return;
        }
        if (text.startsWith(dir)) {
            indexeddirs->takeItem(i);
            i = 0;
        }
    }
    indexeddirs->addItem(dir);
    setDirectories();
}
void
SimpleSearchGui::removeDirectory() {
    int i = indexeddirs->currentRow();
    if (i == -1) return;
    indexeddirs->takeItem(i);
    setDirectories();
}
void
SimpleSearchGui::setDirectories() {
    set<string> s;
    for (int i=0; i<indexeddirs->count(); ++i) {
        QString text = indexeddirs->item(i)->text();
        s.insert((const char*)text.toUtf8());
    }
    SocketClient client;
    client.setSocketName(socketfile.c_str());
    client.setIndexedDirectories(s);
    updateDirectories();
}
void
SimpleSearchGui::updateDirectories() {
    indexeddirs->clear();
    SocketClient client;
    client.setSocketName(socketfile.c_str());
    set<string> s = client.getIndexedDirectories();
    indexeddirs->clear();
    set<string>::const_iterator i;
    for (i = s.begin(); i != s.end(); ++i) {
        QString dir(QString::fromUtf8(i->c_str()));
        indexeddirs->addItem(dir);
    }
}
void
SimpleSearchGui::editPreferences() {
    SocketClient client;
    client.setSocketName(socketfile.c_str());
    multimap<int,string> rules = client.getFilteringRules();
   
    DlgPreferences* dlg = new DlgPreferences (running, &rules);
    dlg->exec();
   
    delete dlg;
    
    //update filtering rules
    client.setFilteringRules( rules);
}
