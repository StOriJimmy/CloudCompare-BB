#ifndef BDR_TRACE_FOOTPRINT_HEADER
#define BDR_TRACE_FOOTPRINT_HEADER

//Local
#include "ccContourExtractor.h"
#include "ccOverlayDialog.h"
#include "StFootPrint.h"

//qCC_db
//#include <ccHObject.h>
#define BUTTON_STATE_CTRL_PUSHED (QApplication::keyboardModifiers() & Qt::ControlModifier)
#define BUTTON_STATE_SHIFT_PUSHED (QApplication::keyboardModifiers() & Qt::ShiftModifier)
#define BUTTON_STATE_ALT_PUSHED (QApplication::keyboardModifiers() & Qt::AltModifier)

class ccGenericPointCloud;
class ccPointCloud;
class ccGLWindow;
class ccPlane;
class QToolButton;

template <class SECTION>
class PickingVertex {

public:
	PickingVertex() :
		buttonState(Qt::NoButton)
		, nearestEntitiy(nullptr)
		, nearestVert(nullptr)
		, nearestVertIndex(-1)
		, selectedEntitiy(nullptr)
		, selectedVert(nullptr)
		, selectedVertIndex(-1)
	{}

	void reset() {
		picking_repo.clear();
		resetSelected();
		resetNearest();
	}

	void resetNearest() {
		nearestEntitiy = nullptr;
		nearestVert = nullptr;
		nearestVertIndex = -1;
	}

	void resetSelected() {
		selectedEntitiy = nullptr;
		selectedVert = nullptr;
		selectedVertIndex = -1;
	}
public:
	Qt::MouseButtons buttonState;	// button is down or just hover

	QList<SECTION> picking_repo;	// for picking

	//! mouse move
	ccHObject* nearestEntitiy;
	CCVector3* nearestVert;
	int nearestVertIndex;
	//! picking result
	ccHObject* selectedEntitiy;
	CCVector3* selectedVert;
	int selectedVertIndex;
};

namespace Ui
{
	class bdrSketcherDlg;
}

//! Section extraction tool
class bdrSketcher : public ccOverlayDialog
{
	Q_OBJECT

public:

	//! Default constructor
	explicit bdrSketcher(QWidget* parent);
	//! Destructor
	~bdrSketcher() override;

	//! Adds a cloud to the 'clouds' pool
	bool addCloud(ccGenericPointCloud* cloud, bool alreadyInDB = true);
	//! Adds a polyline to the 'sections' pool
	/** \warning: if this method returns true, the class takes the ownership of the cloud!
	**/
	bool addPolyline(ccPolyline* poly, bool alreadyInDB = true);
	
	//! Removes all registered entities (clouds & polylines)
	void removeAllEntities();
		
	//inherited from ccOverlayDialog
	bool linkWith(ccGLWindow* win) override;
	bool start() override;
	void stop(bool accepted) override;

	void setTraceViewMode(bool trace_image = true);
	void SetDestAndGround(ccHObject* dest, double ground);

	void setWorkingPlane();

	void importEntities3D(std::vector<std::pair<ccHObject*, ccHObject*>> entities);

	std::vector<ccHObject*> getSections();

protected:

	enum POLYLINE_TYPE
	{
		FOOTPRINT_NORMAL,	// polygon counterclockwise
		FOOTPRINT_HOLE,		// polygon clockwise
		POLYLINE_OPEN,		// polyline
	};

	enum SketchObjectMode {
		SO_POINT,
		SO_POLYLINE,
		SO_CIRCLE_CENTER,
		SO_CIRCLE_3POINT,
		SO_ARC_CENTER,
		SO_ARC_3POINT,
		SO_CURVE_BEZIER,
		SO_CURVE_BEZIER3,
		SO_CURVE_BSPLINE,
		SO_NPOLYGON,
		SO_RECTANGLE,
	};

	//! Imported entity
	template<class EntityType = ccHObject> struct SketcherObject
	{
		//! Default constructor
		SketcherObject()
			: entity(0)
			, originalDisplay(nullptr)
			, isInDB(false)
			, backupColorShown(false)
			, backupWidth(1)
			, isModified(false)
		{}

		//! Copy constructor
		SketcherObject(const SketcherObject& section)
			: entity(section.entity)
			, originalDisplay(section.originalDisplay)
			, isInDB(section.isInDB)
			, backupColorShown(section.backupColorShown)
			, backupWidth(section.backupWidth)
			, isModified(section.isModified)
		{
			backupColor = section.backupColor;
		}

		//! Constructor from an entity
		SketcherObject(EntityType* e, bool alreadyInDB)
			: entity(e)
			, originalDisplay(e->getDisplay())
			, isInDB(alreadyInDB)
		{
#if 0
			//specific case: polylines
			if (e->isA(CC_TYPES::POLY_LINE))
			{
				ccPolyline* poly = reinterpret_cast<ccPolyline*>(e);

				//backup color
				backupColor = poly->getColor();
				backupColorShown = poly->colorsShown();
				//backup thickness
				backupWidth = poly->getWidth();
			}
			if (e->isA(CC_TYPES::ST_FOOTPRINT))
			{
				StFootPrint* poly = reinterpret_cast<StFootPrint*>(e);
				//backup color
				backupColor = poly->getColor();
				backupColorShown = poly->colorsShown();
				//backup thickness
				backupWidth = poly->getWidth();

				if (!poly->isClosed()) {
					type = POLYLINE_OPEN;
				}
				else {
					//! hole state
					if (poly->isHole()) type = FOOTPRINT_HOLE;
					else type = FOOTPRINT_NORMAL;
				}
			}
#endif
		}

		bool operator ==(const SketcherObject& ie) { return entity == ie.entity; }

		EntityType* entity;
		ccHObject* projected_from;
		ccGenericGLDisplay* originalDisplay;
		bool isInDB;
		bool isModified;

		//backup info (for polylines only)
		ccColor::Rgb backupColor;
		bool backupColorShown;
		PointCoordinateType backupWidth;

		//! for footprint only
		POLYLINE_TYPE type;

		SketchObjectMode soMode;
	};

	//! Section
	using Section = SketcherObject<ccHObject>;

	void undo();
	bool reset(bool askForConfirmation = true);
	void apply();
	void cancel();

	void enableSketcherEditingMode(bool);
	
	void createSketchObject(SketchObjectMode mode);
	QToolButton* getCurrentSOButton();

	

	///< edit the selected object
	void enableSelectedSOEditingMode(bool);


	void addCurrentPointToPolyline();
	void closeFootprint();

	void doImportPolylinesFromDB();
	void setVertDimension(int);
	
	void exportFootprints();
	void exportSections();
	void exportFootprintInside();
	void exportFootprintOutside();

	//! To capture overridden shortcuts (pause button, etc.)
	void onShortcutTriggered(int);

	//////////////////////////////////////////////////////////////////////////

	//! echo glview signals
	void echoLeftButtonClicked(int x, int y);
	void echoRightButtonClicked(int x = 0, int y = 0); //arguments for compatibility with ccGlWindow::rightButtonClicked signal
	void changePickingCursor();

	struct SOPickingParams;
	void startCPUPointPicking(const SOPickingParams& params);

	void startCPURectPicking(const SOPickingParams & params, std::vector<PickingVertex<Section>>& selected);

	void echoMouseMoved(int x, int y, Qt::MouseButtons buttons);
	void echoItemPicked(ccHObject* entity, unsigned subEntityID, int x, int y, const CCVector3& P, const CCVector3d& uvw);
	void echoButtonReleased();
	void entitySelected(ccHObject*);
	void echoRectangleSelected(const int & x, const int & y, const int & w, const int & h);

	///< polyline
	void addPointToPolyline(int x, int y);
	void closePolyLine(int x = 0, int y = 0); //arguments for compatibility with ccGlWindow::rightButtonClicked signal
	void updatePolyLine(int x, int y, Qt::MouseButtons buttons);

	//////////////////////////////////////////////////////////////////////////

protected:

	//! Projects a 2D (screen) point to 3D
	//CCVector3 project2Dto3D(int x, int y) const;

	//! Cancels currently edited polyline
	void cancelCurrentPolyline();

	void setDefaultInteractionPickingMode();

	//! Deletes currently selected polyline
	void deleteSelectedPolyline();

	//! Adds a 'step' on the undo stack
	void addUndoStep();

	//! Creates (if necessary) and returns a group to store entities in the main DB
	ccHObject* getExportGroup(unsigned& defaultGroupID, const QString& defaultName);

	

	//! Releases a polyline
	/** The polyline is removed from display. Then it is
		deleted if the polyline is not already in DB.
	**/
	void releasePolyline(Section* section);

	//! Cloud
	using Cloud = SketcherObject<ccGenericPointCloud>;

	//! Type of the pool of active sections
	using SectionPool = QList<Section>;

	//! Type of the pool of clouds
	using CloudPool = QList<Cloud>;

	//! Process states
	enum ProcessStates
	{
		//...			= 1,
		//...			= 2,
		//...			= 4,
		//...			= 8,
		PS_EDITING			= 16,
		PS_PAUSED			= 32,
		PS_STARTED			= 64,
		PS_RUNNING			= 128,
	};

	//! Deselects the currently selected polyline
	void selectSketcherObject(Section* poly, bool autoRefreshDisplay = true);

	//! Updates the global clouds bounding-box
	void updateCloudsBox();

	void setPickingRepoButtonDown();
	void setPickingRepoHover();

private: //members
	Ui::bdrSketcherDlg	*m_UI;
	
	//! Pool of active sections
	SectionPool m_sections;

	//! Selected polyline (if any)
	Section* m_selectedSO;

	CCVector3* m_selectedVert;

	//! Pool of clouds
	CloudPool m_clouds;

	//! Current process state
	unsigned m_state;

	//! Last 'undo' count
	std::vector<size_t> m_undoCount;

	//! Currently edited polyline
	ccPolyline* m_editedPoly;
	//! Segmentation polyline vertices
	ccPointCloud* m_editedPolyVertices;

	//! Global clouds bounding-box
	ccBBox m_cloudsBox;

	double m_ground;
	ccHObject* m_dest_obj;

	bool m_trace_image;

	ccPlane* m_workingPlane;

	SketchObjectMode m_currentSOMode;
	
	PickingVertex<Section>* m_pickingVertex;
};

#endif //BDR_TRACE_FOOTPRINT_HEADER
