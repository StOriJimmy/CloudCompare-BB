#include "bdr3DGeometryEditPanel.h"

//Local
#include "mainwindow.h"
#include "ccItemSelectionDlg.h"

//CCLib
#include <ManualSegmentationTools.h>
#include <SquareMatrix.h>

//qCC_db
#include <ccLog.h>
#include <ccPolyline.h>
#include <ccGenericPointCloud.h>
#include <ccPointCloud.h>
#include <ccMesh.h>
#include <ccHObjectCaster.h>
#include <cc2DViewportObject.h>
#include <ccScalarField.h>
#include <ccColorScalesManager.h>
#include "ccPickingHub.h"
#include "bdrPlaneEditorDlg.h"

#include "StBlock.h"
#include "ccBox.h"
#include "ccSphere.h"
#include "ccCone.h"
#include "ccCylinder.h"
#include "ccTorus.h"
#include "ccDish.h"
#include "ccPlane.h"

//qCC_gl
#include <ccGLWindow.h>

//Qt
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>

#include "stocker_parser.h"

//System
#include <assert.h>

static CC_TYPES::DB_SOURCE s_dbSource;

bdr3DGeometryEditPanel::bdr3DGeometryEditPanel(QWidget* parent, ccPickingHub* pickingHub)
	: ccOverlayDialog(parent, Qt::Tool | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowStaysOnTopHint)
	, m_UI(new Ui::bdr3DGeometryEditPanel )
	, m_pickingHub(pickingHub)
	, m_somethingHasChanged(false)
	, m_state(0)
	, m_segmentationPoly(0)
	, m_polyVertices(0)
	, m_rectangularSelection(false)
	, m_deleteHiddenParts(false)
	, m_destination(nullptr)
	, m_current_editor(GEO_END)
	, m_refPlane(nullptr)
	, m_toolPlane(nullptr)
{
	m_UI->setupUi(this);

	m_UI->blockToolButton->installEventFilter(this);
	m_UI->boxToolButton->installEventFilter(this);
	m_UI->sphereToolButton->installEventFilter(this);
	m_UI->coneToolButton->installEventFilter(this);
	m_UI->cylinderToolButton->installEventFilter(this);
	m_UI->parapetToolButton->installEventFilter(this);
	m_UI->toroidCylinderToolButton->installEventFilter(this);
	m_UI->torusToolButton->installEventFilter(this);
	m_UI->dishToolButton->installEventFilter(this);
	m_UI->planeToolButton->installEventFilter(this);
	m_UI->polygonToolButton->installEventFilter(this);
	m_UI->polylineToolButton->installEventFilter(this);
	m_UI->wallToolButton->installEventFilter(this);

	connect(m_UI->editToolButton,				&QToolButton::toggled, this, &bdr3DGeometryEditPanel::startEditingMode);
	connect(m_UI->blockToolButton,				&QToolButton::clicked, this, &bdr3DGeometryEditPanel::doBlock);
	connect(m_UI->boxToolButton,				&QToolButton::clicked, this, &bdr3DGeometryEditPanel::doBox);
	connect(m_UI->sphereToolButton,				&QToolButton::clicked, this, &bdr3DGeometryEditPanel::doSphere);
	connect(m_UI->coneToolButton,				&QToolButton::clicked, this, &bdr3DGeometryEditPanel::doCone);
	connect(m_UI->cylinderToolButton,			&QToolButton::clicked, this, &bdr3DGeometryEditPanel::doCylinder);
	connect(m_UI->parapetToolButton,			&QToolButton::clicked, this, &bdr3DGeometryEditPanel::doParapet);
	connect(m_UI->toroidCylinderToolButton,		&QToolButton::clicked, this, &bdr3DGeometryEditPanel::doToroidCylinder); 
	connect(m_UI->torusToolButton,				&QToolButton::clicked, this, &bdr3DGeometryEditPanel::doTorus);
	connect(m_UI->dishToolButton,				&QToolButton::clicked, this, &bdr3DGeometryEditPanel::doDish);
	connect(m_UI->planeToolButton,				&QToolButton::clicked, this, &bdr3DGeometryEditPanel::doPlane);
	connect(m_UI->polylineToolButton,			&QToolButton::clicked, this, &bdr3DGeometryEditPanel::doPolyline);
	connect(m_UI->wallToolButton,				&QToolButton::clicked, this, &bdr3DGeometryEditPanel::doWall);
	
	connect(m_UI->razButton,					&QToolButton::clicked, this, &bdr3DGeometryEditPanel::reset);
	connect(m_UI->exitButton,					&QToolButton::clicked, this, &bdr3DGeometryEditPanel::exit);

	connect(m_UI->freeMeshToolButton,			&QToolButton::clicked, this, &bdr3DGeometryEditPanel::makeFreeMesh);

	m_UI->geometryTabWidget->tabBar()->hide();
	
 	//add shortcuts
 	addOverridenShortcut(Qt::Key_Space);  //space bar for the "pause" button
 	addOverridenShortcut(Qt::Key_Escape); //escape key for the "cancel" button
 	addOverridenShortcut(Qt::Key_Return); //return key for the "apply" button
 	addOverridenShortcut(Qt::Key_Delete); //delete key for the "apply and delete" button
 	addOverridenShortcut(Qt::Key_Tab);    //tab key to switch between EDITING mode
 	addOverridenShortcut(Qt::Key_C);      // CONFIRM
 	addOverridenShortcut(Qt::Key_I);      // IMAGE VIEW
	addOverridenShortcut(Qt::Key_E);	  // EIDITING
 	connect(this,								&ccOverlayDialog::shortcutTriggered, this, &bdr3DGeometryEditPanel::onShortcutTriggered);

	echoUIchange();

	createPlaneEditInterface();

//  QMenu* selectionModeMenu = new QMenu(this);
//  selectionModeMenu->addAction(m_UI->action2DSelection);
//  selectionModeMenu->addAction(m_UI->action3DSelection);
// 	m_UI->selectionModeButton->setDefaultAction(m_UI->action3DSelection);
// 	m_UI->selectionModeButton->setMenu(selectionModeMenu);
 
// 	QMenu* importExportMenu = new QMenu(this);
// 	importExportMenu->addAction(actionUseExistingPolyline);
// 	importExportMenu->addAction(actionExportSegmentationPolyline);
// 	loadSaveToolButton->setMenu(importExportMenu);
// 
 	m_polyVertices = new ccPointCloud("vertices");
 	m_segmentationPoly = new ccPolyline(m_polyVertices);
 	m_segmentationPoly->setForeground(true);
 	m_segmentationPoly->setColor(ccColor::green);
 	m_segmentationPoly->showColors(true);
 	m_segmentationPoly->set2DMode(true);
 	allowExecutePolyline(false);

	m_selection_mode = SELECT_3D;
}

bdr3DGeometryEditPanel::~bdr3DGeometryEditPanel()
{
	if (m_segmentationPoly)
		delete m_segmentationPoly;
	m_segmentationPoly = 0;

	if (m_polyVertices)
		delete m_polyVertices;
	m_polyVertices = 0;
}

void bdr3DGeometryEditPanel::echoUIchange()
{
//block
	connect(m_UI->blockTopDoubleSpinBox,	SIGNAL(valueChanged(double)), this, SLOT(echoBlockTopHeight(double)));
	connect(m_UI->blockBottomDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(echoBlockBottomHeight(double)));
}

void bdr3DGeometryEditPanel::createPlaneEditInterface()
{
	m_refPlane = new bdrPlaneEditorDlg(m_pickingHub, this);
	m_UI->refPlaneHorizontalLayout->addWidget(m_refPlane);

	m_toolPlane = new bdrPlaneEditorDlg(m_pickingHub, this);
	m_UI->toolPlaneHorizontalLayout->addWidget(m_toolPlane);
}

bool bdr3DGeometryEditPanel::eventFilter(QObject * obj, QEvent * e)
{
	if (e->type() == QEvent::Close) {
		exit();
	}
	else if (e->type() == QEvent::HoverEnter) {
		if (obj == m_UI->blockToolButton)				{ m_UI->geometryTabWidget->setCurrentIndex(GEO_BLOCK); }
		else if (obj == m_UI->boxToolButton)			{ m_UI->geometryTabWidget->setCurrentIndex(GEO_BOX); }
		else if (obj == m_UI->sphereToolButton)			{ m_UI->geometryTabWidget->setCurrentIndex(GEO_SPHERE); }
		else if (obj == m_UI->coneToolButton)			{ m_UI->geometryTabWidget->setCurrentIndex(GEO_CONE); }
		else if (obj == m_UI->cylinderToolButton)		{ m_UI->geometryTabWidget->setCurrentIndex(GEO_CYLINDER); }
		else if (obj == m_UI->parapetToolButton)		{ m_UI->geometryTabWidget->setCurrentIndex(GEO_PARAPET); }
		else if (obj == m_UI->toroidCylinderToolButton) { m_UI->geometryTabWidget->setCurrentIndex(GEO_TORCYLINDER); }
		else if (obj == m_UI->torusToolButton)			{ m_UI->geometryTabWidget->setCurrentIndex(GEO_TORUS); }
		else if (obj == m_UI->dishToolButton)			{ m_UI->geometryTabWidget->setCurrentIndex(GEO_DISH); }
		else if (obj == m_UI->planeToolButton)			{ m_UI->geometryTabWidget->setCurrentIndex(GEO_PLANE); }
		else if (obj == m_UI->polygonToolButton)		{ m_UI->geometryTabWidget->setCurrentIndex(GEO_POLYGON); }
		else if (obj == m_UI->polylineToolButton)		{ m_UI->geometryTabWidget->setCurrentIndex(GEO_POLYLINE); }
		else if (obj == m_UI->wallToolButton)			{ m_UI->geometryTabWidget->setCurrentIndex(GEO_WALL); }
	}
	else if (e->type() == QEvent::Leave) {
		if (m_current_editor < GEO_END) {
			m_UI->geometryTabWidget->setCurrentIndex(m_current_editor);
		}
	}
	return ccOverlayDialog::eventFilter(obj, e);
}

QToolButton* bdr3DGeometryEditPanel::getGeoToolBottun(GEOMETRY3D g)
{
	switch (g)
	{
	case GEO_BLOCK:
		return m_UI->blockToolButton;
	case GEO_BOX:
		return m_UI->boxToolButton;
	case GEO_SPHERE:
		return m_UI->sphereToolButton;
	case GEO_CONE:
		return m_UI->coneToolButton;
	case GEO_CYLINDER:
		return m_UI->cylinderToolButton;
	case GEO_PARAPET:
		return m_UI->parapetToolButton;
	case GEO_TORCYLINDER:
		return m_UI->toroidCylinderToolButton;
	case GEO_TORUS:
		return m_UI->torusToolButton;
	case GEO_DISH:
		return m_UI->dishToolButton;
	case GEO_PLANE:
		return m_UI->planeToolButton;
	case GEO_POLYGON:
		return m_UI->polygonToolButton;
	case GEO_POLYLINE:
		return m_UI->polylineToolButton;
	case GEO_WALL:			
		return m_UI->wallToolButton;
	default:
		break;
	}
	return nullptr;
}

void bdr3DGeometryEditPanel::allowExecutePolyline(bool state)
{
 	if (state) {
 	}
 	else {
		
 	}
}

void bdr3DGeometryEditPanel::allowStateChange(bool state)
{

}

void bdr3DGeometryEditPanel::updateWithActive(ccHObject * obj)
{
	m_UI->geometryTabWidget->blockSignals(true);
	if (obj->isA(CC_TYPES::ST_BLOCK)) {
		StBlock* block = ccHObjectCaster::ToStBlock(obj);
		if (block) {
			m_UI->blockTopDoubleSpinBox->setValue(block->getTopHeight());
			m_UI->blockBottomDoubleSpinBox->setValue(block->getBottomHeight());
			m_UI->geometryTabWidget->setCurrentIndex(GEO_BLOCK);
		}
	}
	else if (obj->isA(CC_TYPES::BOX)) {
		ccBox* box = ccHObjectCaster::ToBox(obj);
		if (box) {
			CCVector3 dim = box->getDimensions();
			m_UI->boxDxDoubleSpinBox->setValue(dim.x);
			m_UI->boxDxDoubleSpinBox->setValue(dim.y);
			m_UI->boxDxDoubleSpinBox->setValue(dim.z);

			m_UI->geometryTabWidget->setCurrentIndex(GEO_BOX);
		}
	}
	else if (obj->isA(CC_TYPES::SPHERE)) {
		ccSphere* sphere = ccHObjectCaster::ToSphere(obj);
		if (sphere)	{
			m_UI->sphereRadiusDoubleSpinBox->setValue(sphere->getRadius());
			m_UI->sphereDimensionSpinBox->setValue(sphere->getDrawingPrecision());
			
			m_UI->geometryTabWidget->setCurrentIndex(GEO_SPHERE);
		}
	}
	else if (obj->isA(CC_TYPES::CONE)) {
		ccCone* cone = ccHObjectCaster::ToCone(obj);
		if (cone) {
			m_UI->coneTopRadiusDoubleSpinBox->setValue(cone->getTopRadius());
			m_UI->coneBottomRadiusDoubleSpinBox->setValue(cone->getBottomRadius());
			m_UI->coneHeightDoubleSpinBox->setValue(cone->getHeight());
			PointCoordinateType snoutx, snouty;
			cone->getSnout(snoutx, snouty);
			m_UI->coneXOffsetDoubleSpinBox->setValue(snoutx);
			m_UI->coneYOffsetDoubleSpinBox->setValue(snouty);
			if (cone->hasMetaData(GEO_ROUND_FLAG) && cone->getMetaData(GEO_ROUND_FLAG).toBool()) {
				m_UI->conePrecisionRadioButton->setChecked(true);
				m_UI->conePrecisionSpinBox->setValue(cone->getDrawingPrecision());
			}
			else {
				m_UI->coneEdgeDimRadioButton->setChecked(true);
				m_UI->coneEdgeDimSpinBox->setValue(cone->getDrawingPrecision());
			}
			m_UI->geometryTabWidget->setCurrentIndex(GEO_CONE);
		}
	}
	else if (obj->isA(CC_TYPES::CYLINDER)) {
		ccCylinder* cylinder = ccHObjectCaster::ToCylinder(obj);
		if (cylinder) {
			m_UI->cylinderRadiusDoubleSpinBox->setValue(cylinder->getTopRadius());
			m_UI->cylinderHeightDoubleSpinBox->setValue(cylinder->getHeight());
			if (cylinder->hasMetaData(GEO_ROUND_FLAG) && cylinder->getMetaData(GEO_ROUND_FLAG).toBool()) {
				m_UI->cylinderPrecisionRadioButton->setChecked(true);
				m_UI->cylinderPrecisionSpinBox->setValue(cylinder->getDrawingPrecision());
			}
			else {
				m_UI->cylinderEdgeDimRadioButton->setChecked(true);
				m_UI->cylinderEdgeDimSpinBox->setValue(cylinder->getDrawingPrecision());
			}
			m_UI->geometryTabWidget->setCurrentIndex(GEO_CYLINDER);
		}
	}
	else if (obj->isA(CC_TYPES::TORUS)) {
		ccTorus* torus = ccHObjectCaster::ToTorus(obj);
		if (torus) {
			if (torus->isCylinder()) {
				m_UI->toroidCylinderInsideRadiusDoubleSpinBox->setValue(torus->getInsideRadius());
				m_UI->toroidCylinderOutsideRadiusDoubleSpinBox->setValue(torus->getOutsideRadius());
				m_UI->toroidCylinderAngleDoubleSpinBox->setValue(torus->getAngle());
				m_UI->toroidCylinderRectSectionHeightDoubleSpinBox->setValue(torus->getCylinderHeight());
				m_UI->geometryTabWidget->setCurrentIndex(GEO_TORCYLINDER);
			}
			else {
				m_UI->torusInsideRadiusDoubleSpinBox->setValue(torus->getInsideRadius());
				m_UI->torusOutsideRadiusDoubleSpinBox->setValue(torus->getOutsideRadius());
				m_UI->torusAngleDoubleSpinBox->setValue(torus->getAngle());
				m_UI->geometryTabWidget->setCurrentIndex(GEO_TORUS);
			}
		}
	}
	else if (obj->isA(CC_TYPES::DISH)) {
		ccDish* dish = ccHObjectCaster::ToDish(obj);
		if (dish) {
			PointCoordinateType r1, r2;
			dish->getRadius(r1, r2);
			m_UI->dishRadiusDoubleSpinBox->setValue(r1);
			m_UI->dishRadius2DoubleSpinBox->setValue(r2);
			m_UI->dishHeightDoubleSpinBox->setValue(dish->getHeight());
			m_UI->geometryTabWidget->setCurrentIndex(GEO_DISH);
		}
	}
	else if (obj->isA(CC_TYPES::PLANE)) {
		ccPlane* plane = ccHObjectCaster::ToPlane(obj);
		if (plane) {
			m_toolPlane->initWithPlane(plane);
			m_UI->geometryTabWidget->setCurrentIndex(GEO_PLANE);
		}
	}
	else if (obj->isA(CC_TYPES::POLY_LINE)) {

		m_UI->geometryTabWidget->setCurrentIndex(GEO_POLYLINE);
	}
	m_UI->geometryTabWidget->blockSignals(false);
}

void bdr3DGeometryEditPanel::onShortcutTriggered(int key)
{
 	switch(key)
	{
	case Qt::Key_Space:
		m_UI->editToolButton->toggle();
		return;

	case Qt::Key_I:
		MainWindow::TheInstance()->showBestImage(false);
		return;

	case Qt::Key_O:
		//outButton->click();
		return;

	case Qt::Key_E:
		m_UI->editToolButton->toggle();
		return;

	case Qt::Key_Return:
		//validButton->click();
		return;
	case Qt::Key_Delete:
		//validAndDeleteButton->click();
		return;
	case Qt::Key_Escape:
		return;

	case Qt::Key_Tab:
		
		return;

	default:
		//nothing to do
		break;
	}
}

bool bdr3DGeometryEditPanel::linkWith(ccGLWindow* win)
{
	assert(m_segmentationPoly);

	ccGLWindow* oldWin = m_associatedWin;

	if (!ccOverlayDialog::linkWith(win))
	{
		return false;
	}

	if (oldWin)
	{
		oldWin->disconnect(this);
		if (m_segmentationPoly)
		{
			m_segmentationPoly->setDisplay(0);
		}
	}
	
	if (m_associatedWin)
	{
		connect(m_associatedWin, &ccGLWindow::leftButtonClicked,	this, &bdr3DGeometryEditPanel::addPointToPolyline);
		connect(m_associatedWin, &ccGLWindow::rightButtonClicked,	this, &bdr3DGeometryEditPanel::closePolyLine);
		connect(m_associatedWin, &ccGLWindow::mouseMoved,			this, &bdr3DGeometryEditPanel::updatePolyLine);
		connect(m_associatedWin, &ccGLWindow::buttonReleased,		this, &bdr3DGeometryEditPanel::closeRectangle);
		connect(m_associatedWin, &ccGLWindow::entitySelectionChanged, this, &bdr3DGeometryEditPanel::echoSelectChange);

		if (m_segmentationPoly)
		{
			m_segmentationPoly->setDisplay(m_associatedWin);
		}
	}

	return true;
}

bool bdr3DGeometryEditPanel::start()
{
	assert(m_polyVertices && m_segmentationPoly);

	if (!m_associatedWin)
	{
		return false;
	}

	m_segmentationPoly->clear();
	m_polyVertices->clear();
	allowExecutePolyline(false);

	//the user must not close this window!
	m_associatedWin->setUnclosable(true);
	m_associatedWin->addToOwnDB(m_segmentationPoly);
	m_associatedWin->setPickingMode(ccGLWindow::NO_PICKING);
	
	startEditingMode(false);

	m_somethingHasChanged = false;

	reset();

	return ccOverlayDialog::start();
}

void bdr3DGeometryEditPanel::removeAllEntities(bool unallocateVisibilityArrays)
{
	for (QSet<ccHObject*>::const_iterator p = m_toSegment.constBegin(); p != m_toSegment.constEnd(); ++p)
	{
		
	}
	
	m_toSegment.clear();
}

void bdr3DGeometryEditPanel::setActiveItem(std::vector<ccHObject*> active)
{
	if (m_state & RUNNING) {
		return;
	}
	m_actives = active;

	if (m_actives.size() == 1 && m_actives.front()) {
		updateWithActive(m_actives.front());
	}
}

void bdr3DGeometryEditPanel::stop(bool accepted)
{
	assert(m_segmentationPoly);

	if (m_associatedWin) {
		m_associatedWin->setInteractionMode(ccGLWindow::TRANSFORM_CAMERA());
		m_associatedWin->setPickingMode(ccGLWindow::DEFAULT_PICKING);
		m_associatedWin->setPerspectiveState(true, true);
		m_associatedWin->lockRotationAxis(false, CCVector3d(0, 0, 1));
		m_associatedWin->setUnclosable(false);
		m_associatedWin->removeFromOwnDB(m_segmentationPoly);
	}

	ccOverlayDialog::stop(accepted);
}

void bdr3DGeometryEditPanel::reset()
{
	if (m_somethingHasChanged)
	{
		for (QSet<ccHObject*>::const_iterator p = m_toSegment.constBegin(); p != m_toSegment.constEnd(); ++p)
		{
			
		}

		if (m_associatedWin)
			m_associatedWin->redraw(false);
		m_somethingHasChanged = false;
	}

 	m_UI->razButton->setEnabled(false);
// 	validButton->setEnabled(false);
// 	validAndDeleteButton->setEnabled(false);
// 	loadSaveToolButton->setDefaultAction(actionUseExistingPolyline);
}

bool bdr3DGeometryEditPanel::addEntity(ccHObject* entity)
{
	//FIXME
	/*if (entity->isLocked())
		ccLog::Warning(QString("Can't use entity [%1] cause it's locked!").arg(entity->getName()));
	else */
	if (!entity->isDisplayedIn(m_associatedWin))
	{
		ccLog::Warning(QString("[Graphical Segmentation Tool] Entity [%1] is not visible in the active 3D view!").arg(entity->getName()));
	}

	bool result = false;
	if (entity->isKindOf(CC_TYPES::POINT_CLOUD))
	{
		ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(entity);
		//detect if this cloud is in fact a vertex set for at least one mesh
		{
			//either the cloud is the child of its parent mesh
			if (cloud->getParent() && cloud->getParent()->isKindOf(CC_TYPES::MESH) && ccHObjectCaster::ToGenericMesh(cloud->getParent())->getAssociatedCloud() == cloud)
			{
				//ccLog::Warning(QString("[Graphical Segmentation Tool] Can't segment mesh vertices '%1' directly! Select its parent mesh instead!").arg(entity->getName()));
				return false;
			}
			//or the parent of its child mesh!
			ccHObject::Container meshes;
			if (cloud->filterChildren(meshes,false,CC_TYPES::MESH) != 0)
			{
				for (unsigned i=0; i<meshes.size(); ++i)
					if (ccHObjectCaster::ToGenericMesh(meshes[i])->getAssociatedCloud() == cloud)
					{
						//ccLog::Warning(QString("[Graphical Segmentation Tool] Can't segment mesh vertices '%1' directly! Select its child mesh instead!").arg(entity->getName()));
						return false;
					}
			}
		}

		{
			//cloud->hasScalarFields()
			//cloud->geScalarValueColor()
			
			ccPointCloud* cloudObj = ccHObjectCaster::ToPointCloud(cloud);
			if (cloudObj) {
				int class_index = cloudObj->getScalarFieldIndexByName("classification");
				if (class_index < 0) {
					class_index = cloudObj->addScalarField("classification");
					if (class_index < 0) return false;
				}
				
				cloudObj->setCurrentScalarField(class_index);
				cloudObj->setCurrentDisplayedScalarField(class_index);

				ccScalarField* sf = static_cast<ccScalarField*>(cloudObj->getCurrentInScalarField());
				
				if (!sf || sf != cloudObj->getCurrentDisplayedScalarField()) {
					return false;
				}

				sf->setMax(LAS_LABEL::LABEL_END - 1);
				sf->setMin(0);				
				sf->computeMinAndMax(false, false);
				sf->setColorScale(ccColorScalesManager::GetDefaultScale(ccColorScalesManager::CLASSIFICATION));

				cloud->showColors(false);
				cloud->showSF(true);
			}
		}
		

		cloud->setLocked(true);
		m_toSegment.insert(cloud);

		//automatically add cloud's children
		for (unsigned i=0; i<entity->getChildrenNumber(); ++i)
			result |= addEntity(entity->getChild(i));
	}
	else if (0)//(entity->isKindOf(CC_TYPES::MESH))
	{
		if (entity->isKindOf(CC_TYPES::PRIMITIVE))
		{
			ccLog::Warning("[bdr3DGeometryEditPanel] Can't segment primitives yet! Sorry...");
			return false;
		}
		if (entity->isKindOf(CC_TYPES::SUB_MESH))
		{
			ccLog::Warning("[bdr3DGeometryEditPanel] Can't segment sub-meshes! Select the parent mesh...");
			return false;
		}
		else
		{
			ccGenericMesh* mesh = ccHObjectCaster::ToGenericMesh(entity);

			//first, we must check that there's no mesh and at least one of its sub-mesh mixed in the current selection!
			for (QSet<ccHObject*>::const_iterator p = m_toSegment.constBegin(); p != m_toSegment.constEnd(); ++p)
			{
				if ((*p)->isKindOf(CC_TYPES::MESH))
				{
					ccGenericMesh* otherMesh = ccHObjectCaster::ToGenericMesh(*p);
					if (otherMesh->getAssociatedCloud() == mesh->getAssociatedCloud())
					{
						if ((otherMesh->isA(CC_TYPES::SUB_MESH) && mesh->isA(CC_TYPES::MESH))
							|| (otherMesh->isA(CC_TYPES::MESH) && mesh->isA(CC_TYPES::SUB_MESH)))
						{
							ccLog::Warning("[Graphical Segmentation Tool] Can't mix sub-meshes with their parent mesh!");
							return false;
						}
					}
				}
			}

			mesh->getAssociatedCloud()->resetVisibilityArray();
			m_toSegment.insert(mesh);
			result = true;
		}
	}
	else if (entity->isA(CC_TYPES::HIERARCHY_OBJECT))
	{
		//automatically add entity's children
		for (unsigned i=0;i<entity->getChildrenNumber();++i)
			result |= addEntity(entity->getChild(i));
	}

	if (result && getNumberOfValidEntities() == 1) {
		s_dbSource = entity->getDBSourceType();
	}

	return result;
}

unsigned bdr3DGeometryEditPanel::getNumberOfValidEntities() const
{
	return static_cast<unsigned>(m_toSegment.size());
}

void bdr3DGeometryEditPanel::updatePolyLine(int x, int y, Qt::MouseButtons buttons)
{
	//process not started yet?
	if ((m_state & RUNNING) == 0)
	{
		return;
	}
	if (!m_associatedWin)
	{
		assert(false);
		return;
	}

	assert(m_polyVertices);
	assert(m_segmentationPoly);

	unsigned vertCount = m_polyVertices->size();

	//new point (expressed relatively to the screen center)
	QPointF pos2D = m_associatedWin->toCenteredGLCoordinates(x, y);
	CCVector3 P(static_cast<PointCoordinateType>(pos2D.x()),
				static_cast<PointCoordinateType>(pos2D.y()),
				0);

	if (m_state & RECTANGLE)
	{
		//we need 4 points for the rectangle!
		if (vertCount != 4)
			m_polyVertices->resize(4);

		const CCVector3* A = m_polyVertices->getPointPersistentPtr(0);
		CCVector3* B = const_cast<CCVector3*>(m_polyVertices->getPointPersistentPtr(1));
		CCVector3* C = const_cast<CCVector3*>(m_polyVertices->getPointPersistentPtr(2));
		CCVector3* D = const_cast<CCVector3*>(m_polyVertices->getPointPersistentPtr(3));
		*B = CCVector3(A->x,P.y,0);
		*C = P;
		*D = CCVector3(P.x,A->y,0);

		if (vertCount != 4)
		{
			m_segmentationPoly->clear();
			if (!m_segmentationPoly->addPointIndex(0,4))
			{
				ccLog::Error("Out of memory!");
				allowExecutePolyline(false);
				return;
			}
			m_segmentationPoly->setClosed(true);
		}
	}
	else if (m_state & POLYLINE)
	{
		if (vertCount < 2)
			return;
		//we replace last point by the current one
		CCVector3* lastP = const_cast<CCVector3*>(m_polyVertices->getPointPersistentPtr(vertCount-1));
		*lastP = P;
	}

	m_associatedWin->redraw(true, false);
}

void bdr3DGeometryEditPanel::echoSelectChange(ccHObject* obj)
{
	if (m_state & RUNNING) {
		return;
	}
	if (!(QApplication::keyboardModifiers() & Qt::ControlModifier)) {
		m_actives.clear();
	}
	m_actives.push_back(obj);
	setActiveItem(m_actives);
}

void bdr3DGeometryEditPanel::addPointToPolyline(int x, int y)
{
	return;

	if ((m_state & STARTED) == 0)
	{
		return;
	}
	if (!m_associatedWin)
	{
		assert(false);
		return;
	}

	assert(m_polyVertices);
	assert(m_segmentationPoly);
	unsigned vertCount = m_polyVertices->size();

	//particular case: we close the rectangular selection by a 2nd click
	if (m_rectangularSelection && vertCount == 4 && (m_state & RUNNING))
		return;

	//new point
	QPointF pos2D = m_associatedWin->toCenteredGLCoordinates(x, y);
	CCVector3 P(static_cast<PointCoordinateType>(pos2D.x()),
				static_cast<PointCoordinateType>(pos2D.y()),
				0);

	//CTRL key pressed at the same time?
	bool ctrlKeyPressed = m_rectangularSelection || ((QApplication::keyboardModifiers() & Qt::ControlModifier) == Qt::ControlModifier);

	//start new polyline?
	if (((m_state & RUNNING) == 0) || vertCount == 0 || ctrlKeyPressed)
	{
		//reset state
		m_state = (ctrlKeyPressed ? RECTANGLE : POLYLINE);
		m_state |= (STARTED | RUNNING);
		//reset polyline
		m_polyVertices->clear();
		if (!m_polyVertices->reserve(2))
		{
			ccLog::Error("Out of memory!");
			allowExecutePolyline(false);
			return;
		}
		//we add the same point twice (the last point will be used for display only)
		m_polyVertices->addPoint(P);
		m_polyVertices->addPoint(P);
		m_segmentationPoly->clear();
		if (!m_segmentationPoly->addPointIndex(0, 2))
		{
			ccLog::Error("Out of memory!");
			allowExecutePolyline(false);
			return;
		}
	}
	else //next points in "polyline mode" only
	{
		//we were already in 'polyline' mode?
		if (m_state & POLYLINE)
		{
			if (!m_polyVertices->reserve(vertCount+1))
			{
				ccLog::Error("Out of memory!");
				allowExecutePolyline(false);
				return;
			}

			//we replace last point by the current one
			CCVector3* lastP = const_cast<CCVector3*>(m_polyVertices->getPointPersistentPtr(vertCount-1));
			*lastP = P;
			//and add a new (equivalent) one
			m_polyVertices->addPoint(P);
			if (!m_segmentationPoly->addPointIndex(vertCount))
			{
				ccLog::Error("Out of memory!");
				return;
			}
			m_segmentationPoly->setClosed(true);
		}
		else //we must change mode
		{
			assert(false); //we shouldn't fall here?!
			m_state &= (~RUNNING);
			addPointToPolyline(x,y);
			return;
		}
	}

	m_associatedWin->redraw(true, false);
}

void bdr3DGeometryEditPanel::closeRectangle()
{
	//only for rectangle selection in RUNNING mode
	if ((m_state & RECTANGLE) == 0 || (m_state & RUNNING) == 0)
		return;

	assert(m_segmentationPoly);
	unsigned vertCount = m_segmentationPoly->size();
	if (vertCount < 4)
	{
		//first point only? we keep the real time update mechanism
		if (m_rectangularSelection)
			return;
		m_segmentationPoly->clear();
		m_polyVertices->clear();
		allowExecutePolyline(false);
	}
	else
	{
		allowExecutePolyline(true);
	}

	//stop
	m_state &= (~RUNNING);

	if (m_associatedWin)
		m_associatedWin->redraw(true, false);
}

void bdr3DGeometryEditPanel::closePolyLine(int, int)
{
	//only for polyline in RUNNING mode
	if ((m_state & POLYLINE) == 0 || (m_state & RUNNING) == 0)
		return;

	if ((QApplication::keyboardModifiers() & Qt::ShiftModifier)) {
		return;
	}

	assert(m_segmentationPoly);
	unsigned vertCount = m_segmentationPoly->size();
	if (vertCount < 4)
	{
		m_segmentationPoly->clear();
		m_polyVertices->clear();
	}
	else
	{
		//remove last point!
		m_segmentationPoly->resize(vertCount-1); //can't fail --> smaller
		m_segmentationPoly->setClosed(true);
		allowExecutePolyline(true);
	}

	//stop
	m_state &= (~RUNNING);
 	
 	if (m_associatedWin)
 	{
 		m_associatedWin->redraw(true, false);
 	}
}

void bdr3DGeometryEditPanel::segment(bool keepPointsInside)
{
	if (!m_associatedWin)
		return;

	if (!m_segmentationPoly)
	{
		ccLog::Error("No polyline defined!");
		return;
	}

	if (!m_segmentationPoly->isClosed())
	{
		ccLog::Error("Define and/or close the segmentation polygon first! (right click to close)");
		return;
	}

	//viewing parameters
	ccGLCameraParameters camera;
	m_associatedWin->getGLCameraParameters(camera);
	const double half_w = camera.viewport[2] / 2.0;
	const double half_h = camera.viewport[3] / 2.0;

	//for each selected entity
	for (QSet<ccHObject*>::const_iterator p = m_toSegment.constBegin(); p != m_toSegment.constEnd(); ++p)
	{
		ccPointCloud* cloud = ccHObjectCaster::ToPointCloud(*p);
		if (!cloud) continue;
		assert(cloud);

		ccGenericPointCloud::VisibilityTableType& visibilityArray = cloud->getTheVisibilityArray();
		assert(!visibilityArray.empty());

		unsigned cloudSize = cloud->size();

		//we project each point and we check if it falls inside the segmentation polyline
#if defined(_OPENMP)
#pragma omp parallel for
#endif
		for (int i = 0; i < static_cast<int>(cloudSize); ++i)
		{
			
			if (visibilityArray[i] == POINT_VISIBLE)
			{
				const CCVector3* P3D = cloud->getPoint(i);

				CCVector3d Q2D;
				bool pointInFrustrum = camera.project(*P3D, Q2D, true);

				CCVector2 P2D(	static_cast<PointCoordinateType>(Q2D.x-half_w),
								static_cast<PointCoordinateType>(Q2D.y-half_h) );
				
				bool pointInside = pointInFrustrum && CCLib::ManualSegmentationTools::isPointInsidePoly(P2D, m_segmentationPoly);

				visibilityArray[i] = (keepPointsInside != pointInside ? POINT_HIDDEN : POINT_VISIBLE);

			}
		}
	}

	m_somethingHasChanged = true;
 	m_UI->razButton->setEnabled(true);
	startEditingMode(false);
}

void bdr3DGeometryEditPanel::startEditingMode(bool state)
{
	assert(m_polyVertices && m_segmentationPoly);

	if (!m_associatedWin)
		return;

	if (!state/*=activate start mode*/) {
		m_state = PAUSED;
		
		if (m_polyVertices->size() != 0)
		{
			m_segmentationPoly->clear();
			m_polyVertices->clear();
		}
		allowExecutePolyline(false);
		allowStateChange(true);
		m_associatedWin->setInteractionMode(ccGLWindow::TRANSFORM_CAMERA());
		m_associatedWin->setPickingMode(ccGLWindow::DEFAULT_PICKING);
		MainWindow::TheInstance()->dispToStatus(QString("paused, press space to continue labeling"));

		m_UI->editToolButton->setIcon(QIcon(QStringLiteral(":/CC/Stocker/images/stocker/play.png")));
	}
	else {
		m_state = STARTED;
		allowStateChange(false);
		m_associatedWin->setInteractionMode(ccGLWindow::INTERACT_SHIFT_PAN | ccGLWindow::INTERACT_SEND_ALL_SIGNALS);
		if (m_selection_mode == SELECT_2D) {
			MainWindow::TheInstance()->dispToStatus(QString("labeling (2D), left click to add contour points, right click to close"));
		}
		else if (m_selection_mode == SELECT_3D) {
			MainWindow::TheInstance()->dispToStatus(QString("labeling (3D), left click to add contour points, right click to close"));
		}
		m_UI->editToolButton->setIcon(QIcon(QStringLiteral(":/CC/Stocker/images/stocker/pause.png")));
	}

	//update mini-GUI
	m_UI->editToolButton->blockSignals(true);
	m_UI->editToolButton->setChecked(state);
	m_UI->editToolButton->blockSignals(false);

	m_associatedWin->redraw(!state);
}

void bdr3DGeometryEditPanel::pauseAll()
{
	startEditingMode(false);
	m_state = PAUSED;
	QToolButton* ct = getGeoToolBottun(m_current_editor);
	if (ct) ct->setChecked(false);
	m_current_editor = GEO_END;
}

void bdr3DGeometryEditPanel::startGeoTool(GEOMETRY3D g)
{
	bool same = (m_current_editor == g);
	if (m_current_editor < GEO_END) {
		pauseAll();
	}
	if (!same) {
		m_current_editor = g;
	}
}

void bdr3DGeometryEditPanel::doBlock()
{
	startGeoTool(GEO_BLOCK);
}

void bdr3DGeometryEditPanel::doBox()
{
	startGeoTool(GEO_BOX);
}

void bdr3DGeometryEditPanel::doSphere()
{
	startGeoTool(GEO_SPHERE);
}

void bdr3DGeometryEditPanel::doCone()
{
	startGeoTool(GEO_CONE);
}

void bdr3DGeometryEditPanel::doCylinder()
{
	startGeoTool(GEO_CYLINDER);
}

void bdr3DGeometryEditPanel::doParapet()
{
	startGeoTool(GEO_PARAPET);
}

void bdr3DGeometryEditPanel::doToroidCylinder()
{
	startGeoTool(GEO_TORCYLINDER);
}

void bdr3DGeometryEditPanel::doTorus()
{
	startGeoTool(GEO_TORUS);
}

void bdr3DGeometryEditPanel::doDish()
{
	startGeoTool(GEO_DISH);
}

void bdr3DGeometryEditPanel::doPlane()
{
	startGeoTool(GEO_PLANE);
}

void bdr3DGeometryEditPanel::doPolyline()
{
	startGeoTool(GEO_BOX);
}

void bdr3DGeometryEditPanel::doWall()
{
	startGeoTool(GEO_WALL);
}

void bdr3DGeometryEditPanel::startEdit()
{
	if (m_actives.size() == 1) {
		ccHObject* active = m_actives.front();
		if (active)	{

		}
	}
	else {

	}
}

void bdr3DGeometryEditPanel::makeFreeMesh()
{
}

void bdr3DGeometryEditPanel::setLabel()
{
	if (!m_associatedWin)
		return;

	if (!m_segmentationPoly)
	{
		ccLog::Error("No polyline defined!");
		return;
	}

	if (!m_segmentationPoly->isClosed())
	{
		ccLog::Error("Define and/or close the segmentation polygon first! (right click to close)");
		return;
	}

	//viewing parameters
	ccGLCameraParameters camera;
	m_associatedWin->getGLCameraParameters(camera);
	const double half_w = camera.viewport[2] / 2.0;
	const double half_h = camera.viewport[3] / 2.0;

	int label_index = m_UI->typeComboBox->currentIndex();
	
	//for each selected entity
	for (QSet<ccHObject*>::const_iterator p = m_toSegment.constBegin(); p != m_toSegment.constEnd(); ++p)
	{
		ccPointCloud* cloud = ccHObjectCaster::ToPointCloud(*p);
		if (!cloud) continue;
		assert(cloud);

// 		ccGenericPointCloud::VisibilityTableType& visibilityArray = cloud->getTheVisibilityArray();
// 		assert(!visibilityArray.empty());

		unsigned cloudSize = cloud->size();

		//we project each point and we check if it falls inside the segmentation polyline
#if defined(_OPENMP)
#pragma omp parallel for
#endif
		for (int i = 0; i < static_cast<int>(cloudSize); ++i)
		{

			//if (visibilityArray[i] == POINT_VISIBLE)
			{
				const CCVector3* P3D = cloud->getPoint(i);

				CCVector3d Q2D;
				bool pointInFrustrum = camera.project(*P3D, Q2D, true);

				CCVector2 P2D(static_cast<PointCoordinateType>(Q2D.x - half_w),
					static_cast<PointCoordinateType>(Q2D.y - half_h));

				bool pointInside = pointInFrustrum && CCLib::ManualSegmentationTools::isPointInsidePoly(P2D, m_segmentationPoly);

				//visibilityArray[i] = (keepPointsInside != pointInside ? POINT_HIDDEN : POINT_VISIBLE);

				if (pointInside) {
					cloud->setPointScalarValue(i, label_index);
				}

			}
		}
		cloud->notifyGeometryUpdate();
		cloud->prepareDisplayForRefresh();
	}

	m_somethingHasChanged = true;
	m_UI->razButton->setEnabled(true);
	startEditingMode(false);
}

void bdr3DGeometryEditPanel::createEntity()
{
	if (!m_associatedWin)
		return;

	if (!m_segmentationPoly)
	{
		ccLog::Error("No polyline defined!");
		return;
	}

	if (!m_segmentationPoly->isClosed())
	{
		ccLog::Error("Define and/or close the segmentation polygon first! (right click to close)");
		return;
	}

// 	if (!m_destination) {
// 		return;
// 	}

	//viewing parameters
	ccGLCameraParameters camera;
	m_associatedWin->getGLCameraParameters(camera);
	const double half_w = camera.viewport[2] / 2.0;
	const double half_h = camera.viewport[3] / 2.0;

	//for each selected entity
	for (QSet<ccHObject*>::const_iterator p = m_toSegment.constBegin(); p != m_toSegment.constEnd(); ++p)
	{
		ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(*p);
		assert(cloud);

//		ccGenericPointCloud::VisibilityTableType& visibilityArray = cloud->getTheVisibilityArray();

		unsigned cloudSize = cloud->size();
		ccHObject* destination = nullptr;
		if (m_destination) destination = m_destination;
		else {
			destination = GetRootBDBase(*p);
		}
		if (!destination) continue;
		m_changed_baseobj.insert(destination);

		//! create a new point cloud
		
		ccPointCloud* new_ent = new ccPointCloud();
		new_ent->setGlobalScale(cloud->getGlobalScale());
		new_ent->setGlobalShift(cloud->getGlobalShift());

		//we project each point and we check if it falls inside the segmentation polyline
// #if defined(_OPENMP)
// #pragma omp parallel for
// #endif
		for (int i = 0; i < static_cast<int>(cloudSize); ++i)
		{
// 			if (visibilityArray.size() == cloudSize && visibilityArray[i] != POINT_VISIBLE) {
// 				continue;
// 			}
			const CCVector3* P3D = cloud->getPoint(i);

			CCVector3d Q2D;
			bool pointInFrustrum = camera.project(*P3D, Q2D, true);

			CCVector2 P2D(static_cast<PointCoordinateType>(Q2D.x - half_w),
				static_cast<PointCoordinateType>(Q2D.y - half_h));

			bool pointInside = pointInFrustrum && CCLib::ManualSegmentationTools::isPointInsidePoly(P2D, m_segmentationPoly);

			if (!pointInside) { continue; }

			new_ent->addPoint(*P3D);
		}
		new_ent->setRGBColor(ccColor::Generator::Random());
		new_ent->showColors(true);
		new_ent->setPointSize(2);
		
		bool created = false;
		switch (m_UI->typeComboBox->currentIndex())
		{
		case LAS_LABEL::Building:
		{
			int number = GetMaxNumberExcludeChildPrefix(destination, BDDB_BUILDING_PREFIX);
			new_ent->setName(BuildingNameByNumber(number + 1));

			if (new_ent->size() > 15) {
				ccHObject* new_building_folder = new ccHObject(new_ent->getName() + BDDB_ORIGIN_CLOUD_SUFFIX);
				new_building_folder->addChild(new_ent);
				StBuilding* new_building_obj = new StBuilding(new_ent->getName());
				new_building_obj->addChild(new_building_folder);
				new_building_obj->setDisplay_recursive(destination->getDisplay());
				new_building_obj->setDBSourceType_recursive(destination->getDBSourceType());

				destination->addChild(new_building_obj);
				MainWindow::TheInstance()->addToDB(new_building_obj, destination->getDBSourceType());
				created = true;
			}
			
			break;
		}
		default:
			break;
		}
		
		if (!created && new_ent) {
			delete new_ent;
			new_ent = nullptr;
			continue;
		}
		m_somethingHasChanged = true;
	}

	m_UI->razButton->setEnabled(true);
	startEditingMode(false);
}

void bdr3DGeometryEditPanel::exit()
{
	if (m_somethingHasChanged) {
		// TODO: ask for reset
		if (0) {
			reset();
			m_deleteHiddenParts = false;
			stop(false);
			return;
		}
	}
	stop(true);
}

//////////////////////////////////////////////////////////////////////////

void bdr3DGeometryEditPanel::echoBlockTopHeight(double v)
{
	for (ccHObject* active : m_actives) {
		StBlock* block = ccHObjectCaster::ToStBlock(active);
		block->setTopHeight(v);
	}
	m_associatedWin->redraw();
}

void bdr3DGeometryEditPanel::echoBlockBottomHeight(double v)
{
	for (ccHObject* active : m_actives) {
		StBlock* block = ccHObjectCaster::ToStBlock(active);
		block->setBottomHeight(v);
	}
	m_associatedWin->redraw();
}

//////////////////////////////////////////////////////////////////////////