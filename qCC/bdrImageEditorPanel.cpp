#include "bdrImageEditorPanel.h"

//local
#include "mainwindow.h"
#include "bdr2.5DimEditor.h"
#include "ccDBRoot.h"
#include "ccHObject.h"
#include "ccCameraSensor.h"
#include "ccImage.h"
#include "ccGLWindow.h"
#include "ccPointCloud.h"
#include "ccHObjectCaster.h"
#include "ccBBox.h"
#include "bdrTraceFootprintDlg.h"
#include "vcg/space/point3.h"

#include <iostream>
#include <QFileDialog>
#include <QToolButton>
#include <QPushButton>

bdrImageEditorPanel::bdrImageEditorPanel(bdr2Point5DimEditor* img, ccDBRoot* root, QWidget* parent)
	: m_pbdrImshow(img)
	, m_root(root)
	, QDialog(parent, Qt::Tool)
	, Ui::bdrImageEditorPanelDlg()
{
	setupUi(this);
	verticalLayout->setContentsMargins(0, 0, 0, 0);
	verticalLayout->setSpacing(0);

	m_pbdrTraceFP = new bdrTraceFootprint(parent);
	m_pbdrTraceFP->setFixedHeight(23);
	m_pbdrTraceFP->hide();
	connect(m_pbdrTraceFP, &ccOverlayDialog::processFinished, this, &bdrImageEditorPanel::stopEditor);
	verticalLayoutTraceFP->addWidget(m_pbdrTraceFP);

	setMinimumHeight(23);
	setMaximumHeight(155);
	m_image_display_height = 80;

	connect(ZoomFitToolButton,		&QAbstractButton::clicked, this, &bdrImageEditorPanel::ZoomFit);
	connect(toggleListToolButton, &QAbstractButton::clicked, this, &bdrImageEditorPanel::toogleImageList);
	connect(displayAllToolButton, &QAbstractButton::clicked, this, &bdrImageEditorPanel::display);
	connect(PreviousToolButton, &QAbstractButton::clicked, this, &bdrImageEditorPanel::previous);
	connect(NextToolButton, &QAbstractButton::clicked, this, &bdrImageEditorPanel::next);
	connect(polyEditToolButton, &QAbstractButton::clicked, this, &bdrImageEditorPanel::startEditor);

	//imageListWidget
	imageListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	connect(imageListWidget->selectionModel(), &QItemSelectionModel::selectionChanged, this, &bdrImageEditorPanel::changeSelection);
	// double click list
	connect(imageListWidget, &QAbstractItemView::doubleClicked, this, &bdrImageEditorPanel::displayImage);
	
	connect(m_root, &ccDBRoot::selectionChanged, this, &bdrImageEditorPanel::selectImage);

}

void bdrImageEditorPanel::ZoomFit()
{
	if (m_pbdrImshow) {
		m_pbdrImshow->ZoomFit();
	}
}

void bdrImageEditorPanel::toogleImageList()
{
	if (toggleListToolButton->isChecked()) {
		if (m_pbdrTraceFP->isHidden()) {
			setFixedHeight(155);
		}
		else {
			setFixedHeight(178);
		}
	}
	else {
		if (m_pbdrTraceFP->isHidden()) {
			setFixedHeight(23);
		}
		else {
			setFixedHeight(46);
		}
	}
}

void bdrImageEditorPanel::changeSelection()
{
	QList<QListWidgetItem*> list = imageListWidget->selectedItems();
	if (!list.empty()) {
		QListWidgetItem* sel_item = list.front();
		sel_item->text();
		ccHObject::Container children;
		m_root->getRootEntity()->filterChildrenByName(children, true, sel_item->text(), true);
		m_root->unselectAllEntities();
		if (!children.empty()) {
			m_root->selectEntity(children.front());
		}
		imageListWidget->scrollToItem(sel_item);
	}
	else {
		//m_root->unselectAllEntities();
	}
	//m_root->selectEntity()
}

void bdrImageEditorPanel::displayImage()
{	
	//m_root->getSelectedEntities()
	emit imageDisplayed();
}

void bdrImageEditorPanel::selectImage()
{
	ccHObject::Container sels;
	m_root->getSelectedEntities(sels, CC_TYPES::CAMERA_SENSOR);
	if (sels.empty()) {
		return;
	}
	for (size_t i = 0; i < imageListWidget->count(); i++) {
		QListWidgetItem* item = imageListWidget->item(i);
		if (sels.back()->getName() == item->text())	{
			item->setSelected(true);
			break;
		}
	}
}

void bdrImageEditorPanel::previous()
{
	if (imageListWidget->count() < 2) {
		return;
	}
	ccImage* cur_img = m_pbdrImshow->getImage();
	if (!cur_img) { return; }

	ccHObject* cam = cur_img->getAssociatedSensor();
	if (!cam) return;

	m_root->selectEntity(cam);
	
	QList<QListWidgetItem*> list = imageListWidget->selectedItems();
	if (list.empty()) { return; }

	int cur_index = imageListWidget->row(list.front());
	imageListWidget->item((cur_index - 1) % imageListWidget->count())->setSelected(true);
	displayImage();
}

void bdrImageEditorPanel::next()
{
	if (imageListWidget->count() < 2) {
		return;
	}
	ccImage* cur_img = m_pbdrImshow->getImage();
	if (!cur_img) { return; }

	ccHObject* cam = cur_img->getAssociatedSensor();
	if (!cam) return;

	m_root->selectEntity(cam);

	QList<QListWidgetItem*> list = imageListWidget->selectedItems();
	if (list.empty()) { return; }

	int cur_index = imageListWidget->row(list.front());
	imageListWidget->setItemSelected(imageListWidget->item((cur_index + 1) % imageListWidget->count()), true);
	displayImage();
}

void bdrImageEditorPanel::toogleDisplayAll()
{
	display(!displayAllToolButton->isChecked());
}

void bdrImageEditorPanel::startEditor()
{
	if (!m_pbdrTraceFP) {
		return;
	}
	polyEditToolButton->setChecked(true);
	//m_pbdrTraceFP->setExtractMode();
	m_pbdrTraceFP->linkWith(m_pbdrImshow->getGLWindow());
	if (!m_pbdrTraceFP->start()) {
		stopEditor(false);
	}
}

void bdrImageEditorPanel::stopEditor(bool state)
{
	polyEditToolButton->setChecked(false);
	if (state) {
		
	}
	if (m_pbdrTraceFP) {
		m_pbdrTraceFP->removeAllEntities();
	}
}

void bdrImageEditorPanel::updateCursorPos(const CCVector3d & P)
{
	m_pbdrImshow->updateCursorPos(P);
}

void bdrImageEditorPanel::clearAll()
{
	imageListWidget->clear();
	m_pbdrImshow->clearAll();
}

void bdrImageEditorPanel::setItems(std::vector<ccHObject*> items, int defaultSelectedIndex)
{
	imageListWidget->clear();
	if (items.empty()) { return; }
	int max_width = -1;
	for (size_t i = 0; i < items.size(); i++) {
		ccCameraSensor* camObj = ccHObjectCaster::ToCameraSensor(items[i]);
		
		QListWidgetItem *item = new QListWidgetItem;
		QImage image = camObj->getImage();
		if (!image.isNull()) {
			int width = static_cast<int>((double)image.width() / (double)image.height() * (double)m_image_display_height);
			item->setSizeHint(QSize(width, m_image_display_height + 20));
			item->setIcon(QIcon(QPixmap::fromImage(image)));
			if (max_width < width) {
				max_width = width;
			}
		}
		item->setText(camObj->getName());
		imageListWidget->addItem(item);
		
	}
	if (max_width <= 0) {
		return;
	}
	imageListWidget->setIconSize(QSize(max_width, m_image_display_height));
	if (defaultSelectedIndex >= 0 && defaultSelectedIndex < items.size()) {
		imageListWidget->setItemSelected(imageListWidget->item(defaultSelectedIndex), true);
	}
}

void bdrImageEditorPanel::setItems(std::vector<ccCameraSensor*> items, int defaultSelectedIndex)
{
	imageListWidget->clear();
	if (items.empty()) { return; }
	int max_width = -1;
	for (size_t i = 0; i < items.size(); i++) {
		ccCameraSensor* camObj = items[i];

		QListWidgetItem *item = new QListWidgetItem;
		QImage image = camObj->getImage();
		if (!image.isNull()) {
			int width = static_cast<int>((double)image.width() / (double)image.height() * (double)m_image_display_height);
			item->setSizeHint(QSize(width, m_image_display_height + 20));
			item->setIcon(QIcon(QPixmap::fromImage(image)));
			if (max_width < width) {
				max_width = width;
			}
		}
		item->setText(camObj->getName());
		imageListWidget->addItem(item);

	}
	if (max_width <= 0) {
		return;
	}
	imageListWidget->setIconSize(QSize(max_width, m_image_display_height));
	if (defaultSelectedIndex >= 0 && defaultSelectedIndex < items.size()) {
		imageListWidget->setItemSelected(imageListWidget->item(defaultSelectedIndex), true);
	}
}

void bdrImageEditorPanel::display(bool display_all)
{
	displayAllToolButton->setChecked(display_all);
	//! sort by area
	ccHObject::Container children =	GetEnabledObjFromGroup(m_root->getRootEntity(), CC_TYPES::CAMERA_SENSOR, true, true);
	std::vector<ccCameraSensor*> items;
	for (ccHObject* obj : children)	{
		ccCameraSensor* camObj = ccHObjectCaster::ToCameraSensor(obj);
		assert(camObj); if (!camObj) { return; }
		if (!camObj->isBranchEnabled())	{
			continue;
		}
		if (!display_all && camObj->getDisplayOrder() < 0) {
			continue;
		}
		else {
			items.push_back(camObj);
		}
	}
	if (!display_all) {
		if (!items.empty()) {
			std::sort(items.begin(), items.end(), [](ccCameraSensor* _l, ccCameraSensor* _r) {
				return _l->getDisplayOrder() < _r->getDisplayOrder();
			});
		}
	}
	setItems(items, 0);
}

double bdrImageEditorPanel::getBoxScale()
{
	return BoxScaleDoubleSpinBox->value();
}

ccBBox bdrImageEditorPanel::getObjBox()
{
	return m_obj_box;
}

void bdrImageEditorPanel::setObjBox(ccBBox box)
{
	m_obj_box = box;
}

bool bdrImageEditorPanel::isObjChecked()
{
	return m_obj_box.isValid() && CheckObjToolButton->isChecked();	
}


