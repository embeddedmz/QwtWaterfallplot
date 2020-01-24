#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QDialog>

class ExportDialog : public QDialog
{
    Q_OBJECT
    typedef QDialog Superclass;

public:
    explicit ExportDialog(QWidget* const parent = nullptr);
    ~ExportDialog() override;

    bool getExportWaterfallCurve() const;
    bool getExportHorizontalCurve() const;
    bool getExportVerticalCurve() const;

private:
    Q_DISABLE_COPY(ExportDialog)

    struct Internals;
    Internals* m_Internals;
};

#endif // EXPORTDIALOG_H
