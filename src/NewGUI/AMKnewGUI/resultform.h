#ifndef RESULTFORM_H
#define RESULTFORM_H

#include <QDialog>

namespace Ui {
class ResultForm;
}

class ResultForm : public QDialog
{
    Q_OBJECT

public:
    explicit ResultForm(QString results, bool success, QWidget *parent = nullptr);
    ~ResultForm();

private slots:
    void on_pushButton_clicked();

private:
    Ui::ResultForm *ui;
};

#endif // RESULTFORM_H
