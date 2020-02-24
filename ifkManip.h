#ifndef __IFKMANIP__
#define __IFKMANIP__

#include <maya/MIOStream.h>
#include <stdio.h>
#include <stdlib.h>
#include <maya/MFnFreePointTriadManip.h>
#include <maya/MPoint.h>
#include <maya/MVector.h>
#include <maya/MManipData.h>
#include <maya/MItSelectionList.h>
#include <maya/MPlug.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPxManipContainer.h>
#include <maya/MGlobal.h>
#include <maya/MPxNode.h>
#include <maya/MDagPath.h>
#include <maya/MFn.h>
#include <maya/MPxSelectionContext.h>
#include <maya/MModelMessage.h>
#include <maya/MPxContextCommand.h>
#include <maya/MGlobal.h>

#define MANIPNODENAME "ifkManipulator"
#define TOOLNAME "ifkHandleTool"

class IfkManip : public MPxManipContainer
{
public:
	IfkManip();
	~IfkManip() override;

	static void * creator();
	static MStatus initialize();
	MStatus createChildren() override;
	MStatus connectToDependNode(const MObject &node) override;

	//Viewport 2.0 manipulator draw overrides
	void preDrawUI(const M3dView &view) override;
	void drawUI(MHWRender::MUIDrawManager& drawManager, 
		const MHWRender::MFrameContext& frameContext) const override;

	void draw(M3dView &view,
		const MDagPath &path,
		M3dView::DisplayStyle style,
		M3dView::DisplayStatus status) override;
	MVector nodeTranslation() const;

private:
	void updateManipLocations(const MObject &node);


public:
	MDagPath fFreePointManip;
	MDagPath fNodePath;

	//Value preparend for Viewport 2.0 draw
	MPoint fTextPosition;
	static MTypeId id;
};
class IfkManipContext : public MPxSelectionContext
{
public:
	IfkManipContext();
	void    toolOnSetup(MEvent &event) override;
	void    toolOffCleanup() override;

	static void updateManipulators(void * data);
private:
	MCallbackId id1;
};

class IfkManipContextCmd : public MPxContextCommand
{
public:
	IfkManipContextCmd() {};
	MPxContext * makeObj() override;
public:
	static void* creator();
};



#endif //!__IFKMANIP__