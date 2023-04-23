#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QDebug>
#include <QKeyEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QListWidget>
#include <QProcess>
#include <QDesktopServices>
#include <QThread>
#include <QSound>
#include <QSoundEffect>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    //void init(bool init);
    void init();

private slots:
    void on_runButton_clicked();

    void on_changeGlobalSongButton_clicked();

    void on_changeLocalSongButton_clicked();

    void on_clearSongButton_clicked();

    void on_removeSongButton_clicked();

    void on_addSongButton_clicked();

    void on_playSongButton_clicked();

    void on_upButton_clicked();

    void on_downButton_clicked();

    void on_reloadButton_clicked();

    void on_localSongList_itemSelectionChanged();

    void on_globalSongList_itemSelectionChanged();

    void AmkOutputDataHandler();

    void AmkErrorDataHandler();

private:
    Ui::MainWindow *ui;

    const Qt::Key SecretCodeKeys[10];
    int secretKeyCount = 0;
    QString romPath;
    QString listFolderPath;
    QSoundEffect secret;

    void keyPressEvent(QKeyEvent *event);
    void runAMK();
    QString writeList(QListWidget *list);
    void playSecret();
    void playSPC(int localSongIndex);
    void parseListFiles();
    void parseSongs(QFile *file, QString labelName, QListWidget *list);
    QString toRelativePath(QString path);
    QString toListItem(QString path, int index);
    void refreshListBox(QListWidget *box, int offset = 0, int startIndex = 0);
};
#endif // MAINWINDOW_H
