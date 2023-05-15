#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "resultform.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , SecretCodeKeys{Qt::Key_Up,
                 Qt::Key_Up,
                 Qt::Key_Down,
                 Qt::Key_Down,
                 Qt::Key_Left,
                 Qt::Key_Right,
                 Qt::Key_Left,
                 Qt::Key_Right,
                 Qt::Key_B,
                 Qt::Key_A}
{
    romPath = "";
    listFolderPath = "";

    secret.setSource(QUrl("qrc:/Secret.wav"));
    secret.setVolume(0.5);

    ui->setupUi(this);
    this->setFocus();

    QString path = QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
    path.truncate(path.lastIndexOf("/" + QCoreApplication::applicationName() + ".app"));
#endif
    QDir::setCurrent(path);

    QTimer::singleShot(0, this, [this](){
        emit init();
        ui->resultsText->hide();
        ui->autoplayCheckbox->hide();
        parseListFiles();
    });

//    parseListFiles();
}

MainWindow::~MainWindow()
{
    delete ui;
}


/////////////////////
// Inout functions
//

void MainWindow::on_runButton_clicked()
{
    runAMK();
}


void MainWindow::on_changeGlobalSongButton_clicked()
{
    int index = ui->globalSongList->currentRow();
    if ( index == -1 )
        return;
    QString path = QFileDialog::getOpenFileName(this, tr("Choose a song..."), listFolderPath + "music/", tr("Text Files (*.txt)"));
    if ( path == "" )
        return;
    path = toListItem(toRelativePath(path), index + 1);
    ui->globalSongList->item(index)->setText(path);
}


void MainWindow::on_changeLocalSongButton_clicked()
{
    int index = ui->localSongList->currentRow();
    if ( index == -1 )
        return;
    QString path = QFileDialog::getOpenFileName(this, tr("Choose a song..."), listFolderPath + "music/", tr("Text Files (*.txt)"));
    if ( path == "" )
        return;
    QStringList directories = path.split("/");
    if ( !directories.contains("music") )
    {
        QMessageBox::warning(this, " ", "This music file was not in the music folder (or in a folder in the music folder, etc.)");
        return;
    }
    path = toListItem(toRelativePath(path), index + ui->globalSongList->count() + 1);
    ui->localSongList->item(index)->setText(path);
}


void MainWindow::on_clearSongButton_clicked()
{
    int index = ui->localSongList->currentRow();
    if ( index == -1 )
        return;
    ui->localSongList->item(index)->setText(toListItem("", index + ui->globalSongList->count() + 1));
}


void MainWindow::on_removeSongButton_clicked()
{
    int index = ui->localSongList->currentRow();
    if ( index == -1 )
        return;
    ui->localSongList->takeItem(index);

    refreshListBox(ui->localSongList, ui->globalSongList->count(), index);
}


void MainWindow::on_addSongButton_clicked()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Choose a song..."), listFolderPath + "music/", tr("Text Files (*.txt)"));
    if ( path == "")
        return;
    int index = ui->localSongList->currentRow();
    if ( index == -1 )
        index = ui->localSongList->count() - 1;
    path = toListItem(toRelativePath(path), index + ui->globalSongList->count() + 2);
    if ( index != -1 )
        ui->localSongList->insertItem(index + 1, path);
    else
        ui->localSongList->addItem(path);
//    ui->localSongList->setCurrentRow(ui->localSongList->currentRow() + 1);
    ui->localSongList->setCurrentRow(index + 1);

    refreshListBox(ui->localSongList, ui->globalSongList->count(), index);
}


void MainWindow::on_playSongButton_clicked()
{
    int index = ui->localSongList->currentRow();
    if ( index == -1 )
        return;
    playSPC(index);
}


void MainWindow::on_upButton_clicked()
{
    int index = ui->localSongList->currentRow();
    if ( index == -1 )
        return;
    QString higherValueToReplaceWith = ui->localSongList->item(index - 1)->text().mid(4);
    QString lowerValueToReplaceWith = ui->localSongList->item(index)->text().mid(4);
    ui->localSongList->item(index - 1)->setText(toListItem(lowerValueToReplaceWith, index + ui->globalSongList->count()));
    ui->localSongList->item(index)->setText(toListItem(higherValueToReplaceWith, index + ui->globalSongList->count() + 1));

    ui->localSongList->setCurrentRow(ui->localSongList->currentRow() - 1);
}


void MainWindow::on_downButton_clicked()
{
    int index = ui->localSongList->currentRow();
    if ( index == -1 )
        return;
    QString higherValueToReplaceWith = ui->localSongList->item(index)->text().mid(4);
    QString lowerValueToReplaceWith = ui->localSongList->item(index + 1)->text().mid(4);
    ui->localSongList->item(index)->setText(toListItem(lowerValueToReplaceWith, index + ui->globalSongList->count() + 1));
    ui->localSongList->item(index + 1)->setText(toListItem(higherValueToReplaceWith, index + ui->globalSongList->count() + 2));

    ui->localSongList->setCurrentRow(ui->localSongList->currentRow() + 1);
}


void MainWindow::on_reloadButton_clicked()
{
    ui->localSongList->clear();
    ui->globalSongList->clear();
    parseListFiles();
}


void MainWindow::on_localSongList_itemSelectionChanged()
{


    ui->removeSongButton->setEnabled(ui->localSongList->currentIndex().isValid());
    ui->playSongButton->setEnabled(ui->localSongList->currentIndex().isValid());
    ui->clearSongButton->setEnabled(ui->localSongList->currentIndex().isValid());
    ui->changeLocalSongButton->setEnabled(ui->localSongList->currentIndex().isValid());

    ui->upButton->setEnabled(ui->localSongList->currentIndex().isValid() && ui->localSongList->currentRow() > 0);

    ui->downButton->setEnabled(ui->localSongList->currentIndex().isValid() && ui->localSongList->currentRow() < ui->localSongList->count() - 1);

//    ui->globalSongLabel->setText(QString::number(ui->globalSongList->height()));
//    ui->addSongButton->setEnabled(ui->localSongList->count() == 0 || ui->localSongList->currentRow() != 0);
}


void MainWindow::on_globalSongList_itemSelectionChanged()
{
    ui->changeGlobalSongButton->setEnabled(ui->globalSongList->currentIndex().isValid());
}


void MainWindow::keyPressEvent(QKeyEvent *event)
{

    // Ctrl+R to Run
    if ( (event->key() == Qt::Key_R) && QApplication::keyboardModifiers() & Qt::ControlModifier)
    {
        runAMK();
    }


    // Secret key combo
    if ( event->key() == SecretCodeKeys[secretKeyCount])
    {
        secretKeyCount++;
    }
    else if ( ((event->key() & 0xFFFFFFFE) == Qt::Key_Return) && secretKeyCount == sizeof(SecretCodeKeys) / sizeof(SecretCodeKeys[0]) )
    {
        secretKeyCount = 0;
        playSecret();
    }
    else
    {
        secretKeyCount = 0;
    }
}


/////////////////////
// Logic functions


QString amkOutput = QString();
QString amkError = QString();

void MainWindow::runAMK()
{
    if ( ui->porterModeCheckBox->isChecked() && !ui->localSongList->currentIndex().isValid() )
    {
        QMessageBox::warning(this, " ", "No song selected to compile.");
        return;
    }

    try {
        QString writeStr = "Globals:\n" + writeList(ui->globalSongList) + "Locals:\n" + writeList(ui->localSongList);
        QFile file(listFolderPath + "Addmusic_list.txt");
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        file.write(writeStr.toUtf8());
        file.close();

//        QProcess *amk = new QProcess(this);
        QProcess amk(this);
        QStringList arguments;
        amk.setProgram(listFolderPath + "AddmusicK");
        amk.setWorkingDirectory(listFolderPath);

        if ( !ui->porterModeCheckBox->isChecked() )
        {
            if ( romPath == "" )
            {
                romPath = QFileDialog::getOpenFileName(this, tr("Select your ROM..."), listFolderPath, tr("ROM Files (*.smc *.sfc)"));
                if ( romPath == "" )
                    return;
            }
            arguments +=  romPath;
        }
        else
        {
            arguments += "-norom";
            arguments += ui->localSongList->currentItem()->text().mid(4);
        }

        if ( ui->verboseCheckBox->isChecked() )
            arguments += "-v";

        arguments += "-noblock";

        ui->waitText->setText("Please wait a moment...");
        this->repaint();
//        this->update();

        amkError = "";
        amkOutput = "";

        amk.setArguments(arguments);

//        connect(amk, SIGNAL(started()), this, SLOT(AmkStarted()));
//        connect(amk, SIGNAL(finished(int)), this, SLOT(AmkFinished()));

        connect(&amk, &QProcess::readyReadStandardOutput, this, &MainWindow::AmkOutputDataHandler);
        connect(&amk, &QProcess::readyReadStandardError, this, &MainWindow::AmkErrorDataHandler);
        amk.start();
        if ( !amk.waitForFinished() )
        {
            //
        }

        ui->waitText->setText("");

        if ( !ui->porterModeCheckBox->isChecked() )
        {
            ResultForm results(amk.exitCode() == 0 ? amkOutput : amkError, amk.exitCode() == 0, this);
            results.setWindowFlags(Qt::Dialog);
            results.exec();
        }
        else
        {
            if ( amk.exitCode() == 0 )
            {
                QString searchStr("Completed in ");
                try {
                    int index = amkOutput.indexOf(searchStr) + searchStr.length();
                    int index2 = amkOutput.indexOf(' ', index);
                    double timeTaken = 0;
                    if ( index != -1 )
                    {
                        QString timeStr = amkOutput.mid(index, index2 - index);
                        timeTaken = timeStr.toDouble();
                        ui->resultsText->setText("Song compiled successfully (" + QString::number(timeTaken) + (timeTaken == 1 ? " second)" : " seconds)"));
                    }
                } catch (...) {
                    ui->resultsText->setText("Song compiled successfully");
                }

                playSPC(ui->localSongList->currentRow());
            }
            else
            {
                ui->resultsText->setText("There were errors while compiling:\n\n" + amkError);
            }
        }

    } catch (...) {
        QMessageBox::critical(this, " ", "Could not start AddmusicK.  The error was: ");
        return;
    }
}

void MainWindow::AmkOutputDataHandler()
{
    amkOutput += ((QProcess*) sender())->readAllStandardOutput() + "\n";
//    qDebug() << "Out:\n" << amkOutput;
}

void MainWindow::AmkErrorDataHandler()
{
    amkError += ((QProcess*) sender())->readAllStandardError() + "\n";
//    qDebug() << "Err:\n" << amkError;
}

QString MainWindow::writeList(QListWidget *list)
{
    QString ret = "";
    for ( int i = 0 ; i < list->count() ; i++)
    {
        QString fileName = list->item(i)->text().mid(4);
        QString index = list->item(i)->text().mid(0, 2);
        if ( fileName != "" )
            ret += index + "  " + fileName + + "\n";
    }
    return ret;
}

void MainWindow::playSecret()
{
    secret.play();
//    this->setWindowOpacity(1 - (0.5 * (this->windowOpacity() - 0.5) / 0.5));
}

void MainWindow::playSPC(int localSongIndex)
{
    if ( !ui->autoplayCheckbox->checkState() )
        return;
    QString spcFilePath = ui->localSongList->item(localSongIndex)->text().mid(4);

    if ( spcFilePath.lastIndexOf("/") >=0 )
        spcFilePath = spcFilePath.mid(spcFilePath.lastIndexOf("/") + 1);
    if ( spcFilePath.lastIndexOf(".") >=0 )
        spcFilePath = spcFilePath.mid(0, spcFilePath.lastIndexOf(".")) + ".spc";

    spcFilePath = listFolderPath + "SPCs/" + spcFilePath;

    if ( !QFile::exists(spcFilePath) )
    {
        QMessageBox::warning(this, " ", "The SPC for this song has not yet been generated. Click on the Run button first, then try again.");
        return;
    }

    try {
        if ( !QDesktopServices::openUrl(QUrl::fromLocalFile(spcFilePath)) )
        {
            QMessageBox::critical(this, "Error", "Problem opening file: " + spcFilePath);
        }
    } catch (...) {
        QMessageBox::warning(this, " ", "Could not play the SPC file. Please make sure that you have a program set up that can play SPC files, and that SPC files are set to play with that program automatically when opened.");
    }
}


void MainWindow::parseListFiles()
{
    try {
        QFile file;
        QString filePath = QDir::currentPath() + "/Addmusic_list.txt";
        if ( !listFolderPath.isEmpty() )
        {
            filePath = listFolderPath + "/Addmusic_list.txt";
        }
        else if ( !QFile::exists(filePath) )
        {
            filePath = QFileDialog::getOpenFileName(this, tr("Locate Addmusic_list.txt..."), "", tr("Text Files (*.txt)"));
        }
        if ( filePath == "" )
        {
            QMessageBox::warning(this, " ", "Quitting");
            close();
        }

        file.setFileName(filePath);
        file.open(QIODevice::ReadOnly | QIODevice::Text);

        if ( filePath.lastIndexOf("/") >= 0 )
        {
            listFolderPath = filePath.left(filePath.lastIndexOf("/") + 1);
        }

        parseSongs(&file, "Globals", ui->globalSongList);
        parseSongs(&file, "Locals", ui->localSongList);
//        int listHeight = (ui->globalSongList->sizeHintForRow(0) * 9) + (ui->globalSongList->frameWidth() * 2);
//        ui->globalSongList->setFixedHeight(listHeight);
        ui->globalSongList->setFixedHeight((ui->globalSongList->sizeHintForRow(0) * 9) + (ui->globalSongList->frameWidth() * 2));
//        ui->globalSongList->setMaximumHeight(listHeight);
//        ui->globalSongList->setMinimumHeight(listHeight);

    } catch (...) {
        QMessageBox::critical(this, " ", "Quitting due to an error.\r\nHere is the message:\r\n\r\n----------------------------------------\r\n\r\n");
        close();
    }
}

void MainWindow::parseSongs(QFile *file, QString labelName, QListWidget *list)
{
    bool foundList = false;

    while ( !file->atEnd() )
    {
        QString line = file->readLine().trimmed();
        if ( line.startsWith(labelName + ":") )
        {
            foundList = true;
            continue;
        }
        else if (line.contains(":"))
        {
            foundList = false;
        }

        if (foundList && line.length() > 2)
        {
            bool ok;
            int index = line.leftRef(2).toInt(&ok, 16);
            QString path = line.midRef(4).toString();
            list->insertItem(index, toListItem(path, index));
        }
    }

    file->seek(0);
}

QString MainWindow::toRelativePath(QString path)
{
    if ( QDir::isAbsolutePath(listFolderPath) )
        return path.mid((listFolderPath + "music/").length());
//        return path.remove(listFolderPath + "music/");
    else
        return "music/" + path;
}

QString MainWindow::toListItem(QString path, int index)
{
    return QStringLiteral("%1").arg(index, 2, 16, QLatin1Char('0')).toUpper() + ":\t" + path;
}

void MainWindow::refreshListBox(QListWidget *box, int offset, int startIndex)
{
    ui->globalSongLabel->setText("hi");
    for (int i = startIndex; i < ui->localSongList->count(); i++)
    {
        QString path = ui->localSongList->item(i)->text().mid(4);
        ui->localSongList->item(i)->setText(toListItem(path, i + 1 + offset));
        ui->localSongLabel->setText("hi");
    }
}

