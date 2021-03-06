#include "bdrSettingBDSegDlg.h"

bdrSettingBDSegDlg::bdrSettingBDSegDlg(QWidget* parent)
	: m_UI(new Ui::bdrSettingBDSegDlg)
{
	m_UI->setupUi(this);

}

bdrSettingBDSegDlg::~bdrSettingBDSegDlg()
{

	delete m_UI;
}

QStringList bdrSettingBDSegDlg::getParameters()
{
	QStringList paras;
	//1 1st class
	//2 2nd class
	char str[32]; sprintf(str, "%s", m_UI->BDClassLineEdit->text().toStdString().c_str());
	char seps[] = " ";
	char* token = strtok(str, seps);
	std::vector<int> classes;
	while (token != NULL) {
		int class_number;
		sscanf(token, "%d", &class_number);
		classes.push_back(class_number);
		token = strtok(NULL, seps);
	}

	if (classes.size() == 1) {
		paras.append(QString::number(classes[0])); 
		paras.append(QString::number(classes[0]));
	}
	else if (classes.size() >= 2){
		paras.append(QString::number(classes[0]));
		paras.append(QString::number(classes[1]));
	}
	else {
		paras.append("3"); paras.append("3");
	}

	//3 max distance
	paras.append(QString::number(m_UI->maxDistanceDoubleSpinBox->value()));

	//4 min dist 2d
	paras.append(QString::number(m_UI->minDist2DDoubleSpinBox->value()));

	//5 iteration spacing
	paras.append(QString::number(m_UI->IterSpacingDoubleSpinBox->value()));

	//6 min dist 3d
	paras.append(QString::number(m_UI->minDist3DDoubleSpinBox->value()));

	//7 merge
	paras.append(QString::number(m_UI->fractionMergedoubleSpinBox->value()));

	//8 min points
	paras.append(QString::number(m_UI->minPtsSpinBox->value()));

	return paras;
}
