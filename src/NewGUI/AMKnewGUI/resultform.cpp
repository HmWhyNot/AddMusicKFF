#include "resultform.h"
#include "ui_resultform.h"

ResultForm::ResultForm(QString results, bool success, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ResultForm)
{
    ui->setupUi(this);

    ui->textBrowser->setText(results);

    if ( success )
    {
        ui->label->setText(R"(This is the output from AddmusicK. It appears that there were no errors, so your ROM should be all set!

When you are finished, just close this window.)");
    }
    else
    {
        ui->label->setText(R"(This is the output from AddmusicK. There was an error somewhere. Review this information to find the problem, and please see the readme if you need help!

When you are finished, just close this window.)");
    }

    ui->textBrowser->setFocus();
}

ResultForm::~ResultForm()
{
    delete ui;
}

void ResultForm::on_pushButton_clicked()
{
    close();
}

