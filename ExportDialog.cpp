#include "ExportDialog.h"

#include "ui_ExportDialog.h"

struct ExportDialog::Internals
{
    Ui::ExportDialog Ui;
};

ExportDialog::ExportDialog(QWidget* const parent) :
    QDialog(parent),
    m_Internals(new ExportDialog::Internals)
{
    m_Internals->Ui.setupUi(this);

    connect(m_Internals->Ui.ButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_Internals->Ui.ButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_Internals->Ui.PlotsList, &QListWidget::itemChanged,
            this, [this](QListWidgetItem* item)
    {
        if (item->checkState() == Qt::Checked)
        {
            item->setBackgroundColor(QColor("#ffffb2"));
        }
        else
        {
            item->setBackgroundColor(QColor("#ffffff"));
        }
    });

    QStringList plotsList;
    plotsList << "Horizontal Plot" << "Vertical Plot" << "Waterfall Plot";
    m_Internals->Ui.PlotsList->addItems(plotsList);

    for (int i = 0; i < m_Internals->Ui.PlotsList->count(); ++i)
    {
        QListWidgetItem* item = m_Internals->Ui.PlotsList->item(i);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }
}

ExportDialog::~ExportDialog()
{
    delete m_Internals;
}

bool ExportDialog::getExportHorizontalCurve() const
{
    return m_Internals->Ui.PlotsList->item(0)->checkState() == Qt::Checked;
}

bool ExportDialog::getExportVerticalCurve() const
{
    return m_Internals->Ui.PlotsList->item(1)->checkState() == Qt::Checked;
}

bool ExportDialog::getExportWaterfallCurve() const
{
    return m_Internals->Ui.PlotsList->item(2)->checkState() == Qt::Checked;
}
