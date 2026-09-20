#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringList>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void on_actionadd_triggered();

    void on_editAttributes_triggered();

    void on_searchButton_clicked();

    void on_actiondelete_triggered();

    void on_actionnew_triggered();

    void on_actionOFE_triggered();

    void on_actionsave_triggered();

    void on_actionsave_as_triggered();

    void on_actionopen_triggered();

private:
    Ui::MainWindow *ui;
    QStringList attributes;
    QString m_currentFilePath;
    bool saveToFile(const QString &filepath);

};
#endif // MAINWINDOW_H
