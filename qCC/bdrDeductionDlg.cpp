#include "bdrDeductionDlg.h"

//local
#include "mainwindow.h"

#include <QFileDialog>
#include <QToolButton>
#include <QPushButton>

bdrDeductionDlg::bdrDeductionDlg(QWidget* parent)
	: QDialog(parent, Qt::Tool)
	, Ui::BDRDeductionDlg()
{
	setupUi(this);

// 	connect(PointcloudFilePathToolButton, &QAbstractButton::clicked, this, &bdrSettingLoD2Dlg::browsePointcloudFilename);
// 	connect(OutputDirFilePathToolButton, &QAbstractButton::clicked, this, &bdrSettingLoD2Dlg::browseOutputDirPath);
// 	connect(ConfigureFilePathToolButton, &QAbstractButton::clicked, this, &bdrSettingLoD2Dlg::browseConfigureFilename);
// 	connect(buttonBox, SIGNAL(accepted()), this, SLOT(saveSettings()));
}
