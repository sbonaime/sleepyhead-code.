#ifndef NEWPROFILE_H
#define NEWPROFILE_H

#include <QDialog>

namespace Ui {
    class NewProfile;
}

class NewProfile : public QDialog
{
    Q_OBJECT

public:
    explicit NewProfile(QWidget *parent = 0);
    ~NewProfile();
    void skipWelcomeScreen();
private slots:
    void on_nextButton_clicked();

    void on_backButton_clicked();

    void on_cpapModeCombo_activated(int index);

    void on_agreeCheckbox_clicked(bool checked);

private:
    Ui::NewProfile *ui;
    bool m_editMode;
    int m_firstPage;
};

#endif // NEWPROFILE_H
