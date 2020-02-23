#include <maya/MIOStream.h>
#include <stdio.h>
#include <stdlib.h>
#include <maya/MFn.h>
#include <maya/MPxNode.h>
#include <maya/MPxManipContainer.h>
#include <maya/MPxSelectionContext.h>
#include <maya/MPxContextCommand.h>
#include <maya/MModelMessage.h>
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MItSelectionList.h>
#include <maya/MPoint.h>
#include <maya/MVector.h>
#include <maya/MDagPath.h>
#include <maya/MManipData.h>
#include <maya/MMatrix.h>
// Manipulators
#include <maya/MFnFreePointTriadManip.h>
#include <maya/MFnDistanceManip.h>

class IfkManip : public MPxManipContainer
{
public:
	IfkManip();
	~IfkManip() override;

	static void * creator();
	static MStatus initialize();
	MStatus createChildren() override;
	MStatus connectToDependNode(const MObject &node) override;
	void draw(M3dView &view,
		const MDagPath &path,
		M3dView::DisplayStyle style,
		M3dView::DisplayStatus status) override;
private:
	void updateManipLocations(const MObject &node);
public:
	MDagPath fDistanceManip;
	MDagPath fFreePointManip;
	static MTypeId id;
};
MTypeId IfkManip::id(0x8001d);
IfkManip::IfkManip()
{
	// The constructor must not call createChildren for user-defined
	// manipulators.
}
IfkManip::~IfkManip()
{
}
void *IfkManip::creator()
{
	return new IfkManip();
}
MStatus IfkManip::initialize()
{
	MStatus stat;
	stat = MPxManipContainer::initialize();
	return stat;
}
MStatus IfkManip::createChildren()
{
	MStatus stat = MStatus::kSuccess;
	fDistanceManip = addDistanceManip("distanceManip",
		"distance");

	return stat;
}
void IfkManip::updateManipLocations(const MObject &node)
//
// Description
//        setTranslation and setRotation to the parent's transformation.
//
{
	MFnDagNode dagNodeFn(node);
	MDagPath nodePath;
	dagNodeFn.getPath(nodePath);
	MFnFreePointTriadManip manipFn(fFreePointManip);
	MTransformationMatrix m(nodePath.exclusiveMatrix());
	double rot[3];
	MTransformationMatrix::RotationOrder rOrder;
	m.getRotation(rot, rOrder, MSpace::kWorld);
	manipFn.setRotation(rot, rOrder);
	MVector trans = m.getTranslation(MSpace::kWorld);
	manipFn.setTranslation(trans, MSpace::kWorld);
}

MStatus IfkManip::connectToDependNode(const MObject &node)
{
	MStatus stat;
	//
	// This routine connects the distance manip to the scaleY plug on the node
	// and connects the translate plug to the position plug on the freePoint
	// manipulator.
	//
	MFnDependencyNode nodeFn(node);
	MPlug tPlug = nodeFn.findPlug("translate", &stat);
	MFnDistanceManip distanceManipFn(fDistanceManip);
	MFnFreePointTriadManip freePointManipFn(fFreePointManip);
	freePointManipFn.connectToPointPlug(tPlug);
	updateManipLocations(node);
	finishAddingManips();
	MPxManipContainer::connectToDependNode(node);
	return stat;
}
void IfkManip::draw(M3dView & view,
	const MDagPath & path,
	M3dView::DisplayStyle style,
	M3dView::DisplayStatus status)
{
	//
	// Demonstrate how drawing can be overriden for manip containers - draw the
	// string "Stretch Me!" at the origin.
	//
	MPxManipContainer::draw(view, path, style, status);
	view.beginGL();
	MPoint textPos(0, 0, 0);
	char str[100];
	sprintf_s(str, "Ik translateManip");
	MString distanceText(str);
	view.drawText(distanceText, textPos, M3dView::kLeft);
	view.endGL();
}
//
// MoveManipContext
//
// This class is a simple context for supporting a move manipulator.
//
class IfkManipContext : public MPxSelectionContext
{
public:
	IfkManipContext();
	void    toolOnSetup(MEvent &event) override;
	void    toolOffCleanup() override;
	// Callback issued when selection list changes
	static void updateManipulators(void * data);
private:
	MCallbackId id1;
};
IfkManipContext::IfkManipContext()
{
	MString str("Plugin move Manipulator");
	setTitleString(str);
}
void IfkManipContext::toolOnSetup(MEvent &)
{
	MString str("Move the FK chain in a ik way");
	setHelpString(str);
	updateManipulators(this);
	MStatus status;
	id1 = MModelMessage::addCallback(MModelMessage::kActiveListModified,
		updateManipulators,
		this, &status);
	if (!status) {
		MGlobal::displayError("Model addCallback failed");
	}
}
void IfkManipContext::toolOffCleanup()
{
	MStatus status;
	status = MModelMessage::removeCallback(id1);
	if (!status) {
		MGlobal::displayError("Model remove callback failed");
	}
	MPxContext::toolOffCleanup();
}
void IfkManipContext::updateManipulators(void * data)
{
	MStatus stat = MStatus::kSuccess;

	IfkManipContext * ctxPtr = (IfkManipContext *)data;
	ctxPtr->deleteManipulators();
	MSelectionList list;
	stat = MGlobal::getActiveSelectionList(list);
	MItSelectionList iter(list, MFn::kInvalid, &stat);
	if (MS::kSuccess == stat) {
		for (; !iter.isDone(); iter.next()) {
			// Make sure the selection list item is a depend node and has the
			// required plugs before manipulating it.
			//
			MObject dependNode;
			iter.getDependNode(dependNode);
			if (dependNode.isNull() || !dependNode.hasFn(MFn::kDependencyNode))
			{
				MGlobal::displayWarning("Object in selection list is not "
					"a depend node.");
				continue;
			}
			MFnDependencyNode dependNodeFn(dependNode);
			MPlug rPlug = dependNodeFn.findPlug("translate", &stat);
			MPlug sPlug = dependNodeFn.findPlug("scaleY", &stat);
			if (rPlug.isNull() || sPlug.isNull()) {
				MGlobal::displayWarning("Object cannot be manipulated: " +
					dependNodeFn.name());
				continue;
			}
			// Add manipulator to the selected object
			//
			MString manipName("ifkManip");
			MObject manipObject;
			IfkManip* manipulator =
				(IfkManip *)IfkManip::newManipulator(manipName, manipObject);
			if (NULL != manipulator) {
				// Add the manipulator
				//
				ctxPtr->addManipulator(manipObject);
				// Connect the manipulator to the object in the selection list.
				//
				if (!manipulator->connectToDependNode(dependNode))
				{
					MGlobal::displayWarning("Error connecting manipulator to"
						" object: " + dependNodeFn.name());
				}
			}
		}
	}
}
//
// moveManipContext
//
// This is the command that will be used to create instances
// of our context.
//
class IfkManipContextCmd : public MPxContextCommand
{
public:
	IfkManipContextCmd() {};
	MPxContext * makeObj() override;
public:
	static void* creator();
};
MPxContext *IfkManipContextCmd::makeObj()
{
	return new IfkManipContext();
}
void *IfkManipContextCmd::creator()
{
	return new IfkManipContextCmd;
}
//
// The following routines are used to register/unregister
// the context and manipulator
//
MStatus initializePlugin(MObject obj)
{
	MStatus status;
	MFnPlugin plugin(obj, PLUGIN_COMPANY, "3.0", "Any");
	status = plugin.registerContextCommand("ifkContext",
		&IfkManipContextCmd::creator);
	if (!status) {
		MGlobal::displayError("Error registering moveManipContext command");
		return status;
	}
	status = plugin.registerNode("moveManip", IfkManip::id,
		&IfkManip::creator, &IfkManip::initialize,
		MPxNode::kManipContainer);
	if (!status) {
		MGlobal::displayError("Error registering moveManip node");
		return status;
	}
	return status;
}
MStatus uninitializePlugin(MObject obj)
{
	MStatus status;
	MFnPlugin plugin(obj);
	status = plugin.deregisterContextCommand("ifkContext");
	if (!status) {
		MGlobal::displayError("Error deregistering moveManipContext command");
		return status;
	}
	status = plugin.deregisterNode(IfkManip::id);
	if (!status) {
		MGlobal::displayError("Error deregistering moveManip node");
		return status;
	}
	return status;
}