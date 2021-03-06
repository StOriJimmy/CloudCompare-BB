#ifndef BDR_IMAGE_EDITOR_PANEL_HEADER
#define BDR_IMAGE_EDITOR_PANEL_HEADER

#include "ui_bdrImageEditorPanel.h"
class MainWindow;
class bdr2Point5DimEditor;
class ccDBRoot;
class ccHObject;
class ccBBox;
class ccCameraSensor;
class bdrSketcher;
#include "ccBBox.h"

namespace Ui
{
	class bdrImageEditorPanelDlg;
}

class bdrImageEditorPanel : public QDialog
{
	Q_OBJECT

public:
	using ProjectedPair = std::pair<ccHObject*, ccHObject*>;

	//! Default constructor
	explicit bdrImageEditorPanel(bdr2Point5DimEditor* img, ccDBRoot* root, QWidget* parent = 0);
	
protected slots:
	void ZoomFit();
	void toogleImageList();
	void changeSelection();
	void displayImage();
	void selectImage();
	void previous();
	void next();
	void toogleDisplayAll();
	ccHObject * getTraceBlock(QString image_name);
	void startEditor();
	void stopEditor(bool);
	void showBestImage();
	
signals:
	void imageDisplayed();
public:
	void clearAll();
	void setItems(std::vector<ccHObject*> items, int defaultSelectedIndex);
	void setItems(std::vector<ccCameraSensor*> items, int defaultSelectedIndex);
	void display(bool display_all);
	double getBoxScale();

	ccBBox getObjViewBox() { return m_objViewBox; }
	void setObjViewBox(ccBBox box) { m_objViewBox = box; }
	CCVector3d getObjViewUpDir() { return m_objViewUpDir.norm2d() < 1e-6 ? CCVector3d(0, 1, 0) : m_objViewUpDir; }
	void setObjViewUpDir(CCVector3d up) { m_objViewUpDir = up; }
	CCVector3d getImageViewUpDir();

	bool isObjChecked();
	void updateCursorPos(const CCVector3d& P, bool b3d);
	bool isLinkToMainView();
	void linkToMainViewState(bool state);

	void addProjection(std::vector<ccHObject*> project_entities);
	std::vector<ProjectedPair> getProjectedObjects() { return m_projected_2D_3D; }

	void ZoomFitProjected();
	void startImageEditMode();

	void clearTempProjected();

protected:
	std::vector<ProjectedPair> m_projected_2D_3D;

private:
	Ui::bdrImageEditorPanelDlg *m_UI;
	bdr2Point5DimEditor* m_pbdrImshow;
	bdrSketcher* m_pSketcher;
	ccDBRoot* m_root;
	ccBBox m_objViewBox;
	CCVector3d m_objViewUpDir;
	int m_image_display_height;

	bool* m_storedLinkMainState;
};

#endif
