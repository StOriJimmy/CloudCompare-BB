#include "bdrLabelAnnotationPanel.h"

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

bdrLabelAnnotationPanel::bdrLabelAnnotationPanel(QWidget* parent)
	: ccOverlayDialog(parent)
	, m_UI(new Ui::bdrLabelAnnotationPanel )
	, m_somethingHasChanged(false)
	, m_state(0)
	, m_segmentationPoly(0)
	, m_polyVertices(0)
	, m_rectangularSelection(false)
	, m_deleteHiddenParts(false)
	, m_destination(nullptr)
{
	// Set QDialog background as transparent (DGM: doesn't work over an OpenGL context)
	//setAttribute(Qt::WA_NoSystemBackground);

	m_UI->setupUi(this);

// 	connect(inButton,							&QToolButton::clicked,		this,	&bdrLabelAnnotationPanel::segmentIn);
// 	connect(outButton,							&QToolButton::clicked,		this,	&bdrLabelAnnotationPanel::segmentOut);
// 	connect(razButton,							&QToolButton::clicked,		this,	&bdrLabelAnnotationPanel::reset);
// 	connect(validButton,						&QToolButton::clicked,		this,	&bdrLabelAnnotationPanel::apply);
// 	connect(validAndDeleteButton,				&QToolButton::clicked,		this,	&bdrLabelAnnotationPanel::applyAndDelete);
// 	connect(cancelButton,						&QToolButton::clicked,		this,	&bdrLabelAnnotationPanel::cancel);
// 	connect(pauseButton,						&QToolButton::toggled,		this,	&bdrLabelAnnotationPanel::pauseSegmentationMode);
// 
// 	//selection modes
// 	connect(actionSetPolylineSelection,			&QAction::triggered,	this,	&bdrLabelAnnotationPanel::doSetPolylineSelection);
// 	connect(actionSetRectangularSelection,		&QAction::triggered,	this,	&bdrLabelAnnotationPanel::doSetRectangularSelection);
// 	//import/export options
// 	connect(actionUseExistingPolyline,			&QAction::triggered,	this,	&bdrLabelAnnotationPanel::doActionUseExistingPolyline);
// 	connect(actionExportSegmentationPolyline,	&QAction::triggered,	this,	&bdrLabelAnnotationPanel::doExportSegmentationPolyline);
// 
// 	connect(createBuildingToolButton,			SIGNAL(clicked()),		this,	SLOT(createBuilding()));
// 
// 	//add shortcuts
// 	addOverridenShortcut(Qt::Key_Space);  //space bar for the "pause" button
// 	addOverridenShortcut(Qt::Key_Escape); //escape key for the "cancel" button
// 	addOverridenShortcut(Qt::Key_Return); //return key for the "apply" button
// 	addOverridenShortcut(Qt::Key_Delete); //delete key for the "apply and delete" button
// 	addOverridenShortcut(Qt::Key_Tab);    //tab key to switch between rectangular and polygonal selection modes
// 	addOverridenShortcut(Qt::Key_I);      //'I' key for the "segment in" button
// 	addOverridenShortcut(Qt::Key_O);      //'O' key for the "segment out" button
// 	connect(this, &ccOverlayDialog::shortcutTriggered, this, &bdrLabelAnnotationPanel::onShortcutTriggered);
// 
// 	QMenu* selectionModeMenu = new QMenu(this);
// 	selectionModeMenu->addAction(actionSetPolylineSelection);
// 	selectionModeMenu->addAction(actionSetRectangularSelection);
// 	selectionModelButton->setDefaultAction(actionSetPolylineSelection);
// 	selectionModelButton->setMenu(selectionModeMenu);
// 
// 	QMenu* importExportMenu = new QMenu(this);
// 	importExportMenu->addAction(actionUseExistingPolyline);
// 	importExportMenu->addAction(actionExportSegmentationPolyline);
// 	loadSaveToolButton->setMenu(importExportMenu);
// 
// 	m_polyVertices = new ccPointCloud("vertices");
// 	m_segmentationPoly = new ccPolyline(m_polyVertices);
// 	m_segmentationPoly->setForeground(true);
// 	m_segmentationPoly->setColor(ccColor::green);
// 	m_segmentationPoly->showColors(true);
// 	m_segmentationPoly->set2DMode(true);
// 	allowPolylineExport(false);
}

void bdrLabelAnnotationPanel::allowPolylineExport(bool state)
{
// 	if (state)
// 	{
// 		actionExportSegmentationPolyline->setEnabled(true);
// 	}
// 	else
// 	{
// 		loadSaveToolButton->setDefaultAction(actionUseExistingPolyline);
// 		actionExportSegmentationPolyline->setEnabled(false);
// 	}
}

bdrLabelAnnotationPanel::~bdrLabelAnnotationPanel()
{
	if (m_segmentationPoly)
		delete m_segmentationPoly;
	m_segmentationPoly = 0;

	if (m_polyVertices)
		delete m_polyVertices;
	m_polyVertices = 0;
}

void bdrLabelAnnotationPanel::onShortcutTriggered(int key)
{
 	switch(key)
	{
	case Qt::Key_Space:
		m_UI->pauseButton->toggle();
		return;

	case Qt::Key_I:
		//inButton->click();
		return;

	case Qt::Key_O:
		//outButton->click();
		return;

	case Qt::Key_Return:
		//validButton->click();
		return;
	case Qt::Key_Delete:
		//validAndDeleteButton->click();
		return;
	case Qt::Key_Escape:
		//cancelButton->click();
		return;

	case Qt::Key_Tab:
		if (m_rectangularSelection)
			doSetPolylineSelection();
		else
			doSetRectangularSelection();
		return;

	default:
		//nothing to do
		break;
	}
}

bool bdrLabelAnnotationPanel::linkWith(ccGLWindow* win)
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
		connect(m_associatedWin, &ccGLWindow::leftButtonClicked,	this, &bdrLabelAnnotationPanel::addPointToPolyline);
		connect(m_associatedWin, &ccGLWindow::rightButtonClicked,	this, &bdrLabelAnnotationPanel::closePolyLine);
		connect(m_associatedWin, &ccGLWindow::mouseMoved,			this, &bdrLabelAnnotationPanel::updatePolyLine);
		connect(m_associatedWin, &ccGLWindow::buttonReleased,		this, &bdrLabelAnnotationPanel::closeRectangle);

		if (m_segmentationPoly)
		{
			m_segmentationPoly->setDisplay(m_associatedWin);
		}
	}

	return true;
}

bool bdrLabelAnnotationPanel::start()
{
	assert(m_polyVertices && m_segmentationPoly);

	if (!m_associatedWin)
	{
		ccLog::Warning("[Graphical Segmentation Tool] No associated window!");
		return false;
	}

	m_segmentationPoly->clear();
	m_polyVertices->clear();
	allowPolylineExport(false);

	//the user must not close this window!
	m_associatedWin->setUnclosable(true);
	m_associatedWin->addToOwnDB(m_segmentationPoly);
	m_associatedWin->setPickingMode(ccGLWindow::NO_PICKING);
	pauseSegmentationMode(false);

	m_somethingHasChanged = false;

	reset();

	return ccOverlayDialog::start();
}

void bdrLabelAnnotationPanel::removeAllEntities(bool unallocateVisibilityArrays)
{
	if (unallocateVisibilityArrays)
	{
		for (QSet<ccHObject*>::const_iterator p = m_toSegment.constBegin(); p != m_toSegment.constEnd(); ++p)
		{
			ccHObjectCaster::ToGenericPointCloud(*p)->unallocateVisibilityArray();
		}
	}

	m_toSegment.clear();
}

void bdrLabelAnnotationPanel::stop(bool accepted)
{
	assert(m_segmentationPoly);

	if (m_associatedWin)
	{
		m_associatedWin->displayNewMessage("Segmentation [OFF]",
											ccGLWindow::UPPER_CENTER_MESSAGE,
											false,
											2,
											ccGLWindow::MANUAL_SEGMENTATION_MESSAGE);

		m_associatedWin->setInteractionMode(ccGLWindow::TRANSFORM_CAMERA());
		m_associatedWin->setPickingMode(ccGLWindow::DEFAULT_PICKING);
		m_associatedWin->setUnclosable(false);
		m_associatedWin->removeFromOwnDB(m_segmentationPoly);
	}

	ccOverlayDialog::stop(accepted);
}

void bdrLabelAnnotationPanel::reset()
{
	if (m_somethingHasChanged)
	{
		for (QSet<ccHObject*>::const_iterator p = m_toSegment.constBegin(); p != m_toSegment.constEnd(); ++p)
		{
			if (m_segment_mode <= SEGMENT_PLANE_SPLIT) {
				ccHObjectCaster::ToGenericPointCloud(*p)->resetVisibilityArray();
			}
		}

		if (m_associatedWin)
			m_associatedWin->redraw(false);
		m_somethingHasChanged = false;
	}

// 	razButton->setEnabled(false);
// 	validButton->setEnabled(false);
// 	validAndDeleteButton->setEnabled(false);
// 	loadSaveToolButton->setDefaultAction(actionUseExistingPolyline);
}

bool bdrLabelAnnotationPanel::addEntity(ccHObject* entity)
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
				ccLog::Warning(QString("[Graphical Segmentation Tool] Can't segment mesh vertices '%1' directly! Select its parent mesh instead!").arg(entity->getName()));
				return false;
			}
			//or the parent of its child mesh!
			ccHObject::Container meshes;
			if (cloud->filterChildren(meshes,false,CC_TYPES::MESH) != 0)
			{
				for (unsigned i=0; i<meshes.size(); ++i)
					if (ccHObjectCaster::ToGenericMesh(meshes[i])->getAssociatedCloud() == cloud)
					{
						ccLog::Warning(QString("[Graphical Segmentation Tool] Can't segment mesh vertices '%1' directly! Select its child mesh instead!").arg(entity->getName()));
						return false;
					}
			}
		}

		if (m_segment_mode == SEGMENT_CLASS_EDIT || m_segment_mode == SEGMENT_BUILD_EIDT) {
		}
		else {
			cloud->resetVisibilityArray();
		}
		
		m_toSegment.insert(cloud);

		//automatically add cloud's children
		for (unsigned i=0; i<entity->getChildrenNumber(); ++i)
			result |= addEntity(entity->getChild(i));
	}
	else if (entity->isKindOf(CC_TYPES::MESH))
	{
		if (entity->isKindOf(CC_TYPES::PRIMITIVE))
		{
			ccLog::Warning("[bdrLabelAnnotationPanel] Can't segment primitives yet! Sorry...");
			return false;
		}
		if (entity->isKindOf(CC_TYPES::SUB_MESH))
		{
			ccLog::Warning("[bdrLabelAnnotationPanel] Can't segment sub-meshes! Select the parent mesh...");
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

unsigned bdrLabelAnnotationPanel::getNumberOfValidEntities() const
{
	return static_cast<unsigned>(m_toSegment.size());
}

void bdrLabelAnnotationPanel::updatePolyLine(int x, int y, Qt::MouseButtons buttons)
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
				allowPolylineExport(false);
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

void bdrLabelAnnotationPanel::addPointToPolyline(int x, int y)
{
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
			allowPolylineExport(false);
			return;
		}
		//we add the same point twice (the last point will be used for display only)
		m_polyVertices->addPoint(P);
		m_polyVertices->addPoint(P);
		m_segmentationPoly->clear();
		if (!m_segmentationPoly->addPointIndex(0, 2))
		{
			ccLog::Error("Out of memory!");
			allowPolylineExport(false);
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
				allowPolylineExport(false);
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

void bdrLabelAnnotationPanel::closeRectangle()
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
		allowPolylineExport(false);
	}
	else
	{
		allowPolylineExport(true);
	}

	//stop
	m_state &= (~RUNNING);

	if (m_associatedWin)
		m_associatedWin->redraw(true, false);
}

void bdrLabelAnnotationPanel::closePolyLine(int, int)
{
	//only for polyline in RUNNING mode
	if ((m_state & POLYLINE) == 0 || (m_state & RUNNING) == 0)
		return;

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
	}

	//stop
	m_state &= (~RUNNING);

	//set the default import/export icon to 'export' mode
// 	loadSaveToolButton->setDefaultAction(actionExportSegmentationPolyline);
// 	allowPolylineExport(m_segmentationPoly->size() > 1);
// 	if (m_segment_mode == SEGMENT_PLANE_CREATE || m_segment_mode == SEGMENT_PLANE_SPLIT) {
// 		validButton->setEnabled(m_segmentationPoly->size() > 1);
// 		validAndDeleteButton->setEnabled(m_segmentationPoly->size() > 1);
// 	}	
// 	if (m_associatedWin)
// 	{
// 		m_associatedWin->redraw(true, false);
// 	}
}

void bdrLabelAnnotationPanel::segmentIn()
{
	segment(true);
}

void bdrLabelAnnotationPanel::segmentOut()
{
	segment(false);
}

void bdrLabelAnnotationPanel::segment(bool keepPointsInside)
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
		ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(*p);
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
// 	validButton->setEnabled(true);
// 	validAndDeleteButton->setEnabled(true);
// 	razButton->setEnabled(true);
	pauseSegmentationMode(true);
}

void bdrLabelAnnotationPanel::pauseSegmentationMode(bool state)
{
	assert(m_polyVertices && m_segmentationPoly);

	if (!m_associatedWin)
		return;

	if (state/*=activate pause mode*/)
	{
		m_state = PAUSED;
		if (m_polyVertices->size() != 0)
		{
			m_segmentationPoly->clear();
			m_polyVertices->clear();
			allowPolylineExport(false);
		}
		m_associatedWin->setInteractionMode(ccGLWindow::TRANSFORM_CAMERA());
		m_associatedWin->displayNewMessage("Segmentation [PAUSED]", ccGLWindow::UPPER_CENTER_MESSAGE, false, 3600, ccGLWindow::MANUAL_SEGMENTATION_MESSAGE);
		m_associatedWin->displayNewMessage("Unpause to segment again", ccGLWindow::UPPER_CENTER_MESSAGE, true, 3600, ccGLWindow::MANUAL_SEGMENTATION_MESSAGE);
	}
	else
	{
		m_state = STARTED;
		m_associatedWin->setInteractionMode(ccGLWindow::INTERACT_SEND_ALL_SIGNALS);
		if (m_rectangularSelection)
		{
			m_associatedWin->displayNewMessage("Segmentation [ON] (rectangular selection)", ccGLWindow::UPPER_CENTER_MESSAGE, false, 3600, ccGLWindow::MANUAL_SEGMENTATION_MESSAGE);
			m_associatedWin->displayNewMessage("Left click: set opposite corners", ccGLWindow::UPPER_CENTER_MESSAGE, true, 3600, ccGLWindow::MANUAL_SEGMENTATION_MESSAGE);
		}
		else
		{
			m_associatedWin->displayNewMessage("Segmentation [ON] (polygonal selection)", ccGLWindow::UPPER_CENTER_MESSAGE, false, 3600, ccGLWindow::MANUAL_SEGMENTATION_MESSAGE);
			m_associatedWin->displayNewMessage("Left click: add contour points / Right click: close", ccGLWindow::UPPER_CENTER_MESSAGE, true, 3600, ccGLWindow::MANUAL_SEGMENTATION_MESSAGE);
		}
	}

	//update mini-GUI
	m_UI->pauseButton->blockSignals(true);
	m_UI->pauseButton->setChecked(state);
	m_UI->pauseButton->blockSignals(false);

	m_associatedWin->redraw(!state);
}

void bdrLabelAnnotationPanel::doSetPolylineSelection()
{
	if (!m_rectangularSelection)
		return;

	//selectionModelButton->setDefaultAction(actionSetPolylineSelection);

	m_rectangularSelection = false;
	if (m_state != PAUSED)
	{
		pauseSegmentationMode(true);
		pauseSegmentationMode(false);
	}

	m_associatedWin->displayNewMessage(QString(),ccGLWindow::UPPER_CENTER_MESSAGE); //clear the area
	m_associatedWin->displayNewMessage("Segmentation [ON] (rectangular selection)",ccGLWindow::UPPER_CENTER_MESSAGE,false,3600,ccGLWindow::MANUAL_SEGMENTATION_MESSAGE);
	m_associatedWin->displayNewMessage("Right click: set opposite corners",ccGLWindow::UPPER_CENTER_MESSAGE,true,3600,ccGLWindow::MANUAL_SEGMENTATION_MESSAGE);
}

void bdrLabelAnnotationPanel::doSetRectangularSelection()
{
	if (m_rectangularSelection)
		return;

	//selectionModelButton->setDefaultAction(actionSetRectangularSelection);

	m_rectangularSelection=true;
	if (m_state != PAUSED)
	{
		pauseSegmentationMode(true);
		pauseSegmentationMode(false);
	}

	m_associatedWin->displayNewMessage(QString(),ccGLWindow::UPPER_CENTER_MESSAGE); //clear the area
	m_associatedWin->displayNewMessage("Segmentation [ON] (rectangular selection)",ccGLWindow::UPPER_CENTER_MESSAGE,false,3600,ccGLWindow::MANUAL_SEGMENTATION_MESSAGE);
	m_associatedWin->displayNewMessage("Right click: set opposite corners",ccGLWindow::UPPER_CENTER_MESSAGE,true,3600,ccGLWindow::MANUAL_SEGMENTATION_MESSAGE);
}

void bdrLabelAnnotationPanel::apply()
{
	if (m_segment_mode == SEGMENT_PLANE_CREATE || m_segment_mode == SEGMENT_PLANE_SPLIT) {
		segmentIn();
	}
	m_deleteHiddenParts = false;
	stop(true);
}

void bdrLabelAnnotationPanel::applyAndDelete()
{
	if (m_segment_mode == SEGMENT_PLANE_CREATE || m_segment_mode == SEGMENT_PLANE_SPLIT) {
		segmentOut();
	}
	m_deleteHiddenParts = true;
	stop(true);
}

void bdrLabelAnnotationPanel::cancel()
{
	reset();
	m_deleteHiddenParts = false;
	stop(false);
}

void bdrLabelAnnotationPanel::setSegmentMode(SegmentMode mode)
{
	m_segment_mode = mode;

// 	switch (mode)
// 	{
// 	case bdrLabelAnnotationPanel::SEGMENT_GENERAL:
// 		inButton->setVisible(true);
// 		outButton->setVisible(true);
// 		validAndDeleteButton->setToolTip("Confirm and delete hidden points");
// 		validButton->setToolTip("Confirm segmentation");
// 		createBuildingToolButton->setVisible(false);
// 		break;
// 	case bdrLabelAnnotationPanel::SEGMENT_PLANE_CREATE:
// 	case bdrLabelAnnotationPanel::SEGMENT_PLANE_SPLIT:
// 		inButton->setVisible(false);
// 		outButton->setVisible(false);
// 		validAndDeleteButton->setToolTip("Confirm and delete points inside the polygon");
// 		validButton->setToolTip("Create a new plane by points inside the polygon");
// 		createBuildingToolButton->setVisible(false);
// 		break;
// 	case bdrLabelAnnotationPanel::SEGMENT_CLASS_EDIT:
// 		inButton->setVisible(false);
// 		outButton->setVisible(false);
// 		break;
// 	case bdrLabelAnnotationPanel::SEGMENT_BUILD_EIDT:
// 		inButton->setVisible(false);
// 		outButton->setVisible(false);
// 		createBuildingToolButton->setVisible(true);
// 		validAndDeleteButton->setVisible(false);
// 		break;
// 	default:
// 		break;
// 	}
}

void bdrLabelAnnotationPanel::createBuilding()
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

	if (!m_destination) {
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
		ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(*p);
		assert(cloud);

		ccGenericPointCloud::VisibilityTableType& visibilityArray = cloud->getTheVisibilityArray();

		unsigned cloudSize = cloud->size();

		//! create a new point cloud
		int number = GetMaxNumberExcludeChildPrefix(m_destination, BDDB_BUILDING_PREFIX);
		ccPointCloud* new_building = new ccPointCloud(BuildingNameByNumber(number + 1));
		new_building->setGlobalScale(cloud->getGlobalScale());
		new_building->setGlobalShift(cloud->getGlobalShift());

		//we project each point and we check if it falls inside the segmentation polyline
// #if defined(_OPENMP)
// #pragma omp parallel for
// #endif
		for (int i = 0; i < static_cast<int>(cloudSize); ++i)
		{
			if (visibilityArray.size() == cloudSize && visibilityArray[i] != POINT_VISIBLE)	{
				continue;
			}
			const CCVector3* P3D = cloud->getPoint(i);

			CCVector3d Q2D;
			bool pointInFrustrum = camera.project(*P3D, Q2D, true);

			CCVector2 P2D(static_cast<PointCoordinateType>(Q2D.x - half_w),
				static_cast<PointCoordinateType>(Q2D.y - half_h));

			bool pointInside = pointInFrustrum && CCLib::ManualSegmentationTools::isPointInsidePoly(P2D, m_segmentationPoly);

			if (!pointInside) { continue; }
			
			new_building->addPoint(*P3D);
		}
				
		new_building->setDisplay(m_destination->getDisplay());
		new_building->setRGBColor(ccColor::Generator::Random());
		new_building->showColors(true);
		m_destination->addChild(new_building);
		MainWindow::TheInstance()->addToDB(new_building, m_destination->getDBSourceType());
	}

	m_somethingHasChanged = true;
// 	validButton->setEnabled(true);
// 	validAndDeleteButton->setEnabled(true);
// 	razButton->setEnabled(true);
	pauseSegmentationMode(true);
}