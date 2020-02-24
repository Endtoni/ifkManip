#include "ifkManip.h"
#include <maya/MFnPlugin.h>
#include <maya/MMatrix.h>

MTypeId IfkManip::id(0x8001d);
IfkManip::IfkManip(){}
IfkManip::~IfkManip(){}
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
	fFreePointManip = addFreePointTriadManip("pointManip", "freePoint");
	return stat;
}
void IfkManip::updateManipLocations(const MObject &node)
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
	MFnDependencyNode nodeFn(node);
	MPlug tPlug = nodeFn.findPlug("translate", &stat);
	MFnFreePointTriadManip freePointManipFn(fFreePointManip);
	freePointManipFn.connectToPointPlug(tPlug);
	updateManipLocations(node);
	finishAddingManips();
	MPxManipContainer::connectToDependNode(node);
	return stat;
}
void IfkManip::preDrawUI(const M3dView &view)
{}

void IfkManip::drawUI(MHWRender::MUIDrawManager& drawManager,
	const MHWRender::MFrameContext& frameContext) const
{}

void IfkManip::draw(M3dView & view,
	const MDagPath & path,
	M3dView::DisplayStyle style,
	M3dView::DisplayStatus status)
{	
	MPxManipContainer::draw(view, path, style, status);
	if(fNodePath.isValid()){
		view.beginGL();
		MMatrix mat = fNodePath.inclusiveMatrix();
		MTransformationMatrix tMat(mat);
		MVector vec = tMat.getTranslation(MSpace::kWorld);
		MPoint textPos(vec);
		char str[100];

		sprintf_s(str, "update text pose: %f %f %f", textPos.x, textPos.x, textPos.x);
		MString distanceText(str);
		view.drawText(distanceText, textPos, M3dView::kLeft);
		view.endGL();
	}


}

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
	MGlobal::displayInfo("Update manip");
	IfkManipContext * ctxPtr = (IfkManipContext *)data;
	ctxPtr->deleteManipulators();
	MSelectionList list;
	stat = MGlobal::getActiveSelectionList(list);

	unsigned int length = list.length();


	MItSelectionList iter(list, MFn::kInvalid, &stat);
	MObject dependNode;
	MDagPath dagPath;
	if (MS::kSuccess == stat) {
		for (; !iter.isDone(); iter.next()) {
			// Make sure the selection list item is a depend node and has the
			// required plugs before manipulating it.
			//
			iter.getDependNode(dependNode);
			if (dependNode.isNull() || !dependNode.hasFn(MFn::kDagNode))
			{
				MGlobal::displayWarning("Object in selection list is not "
					"a dag node.");
				continue;
			}
			iter.getDagPath(dagPath);
		}
	}
	// Add manipulator to the selected object
//
	MFnDependencyNode dependNodeFn(dependNode);
	MString manipName(MANIPNODENAME);
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
		manipulator->fNodePath = dagPath;
	}
}

MPxContext *IfkManipContextCmd::makeObj()
{
	return new IfkManipContext();
}

void *IfkManipContextCmd::creator()
{
	return new IfkManipContextCmd;
}

MStatus initializePlugin(MObject obj)
{
	MStatus status;
	MFnPlugin plugin(obj, "TYE", "1.0", "Any");
	status = plugin.registerContextCommand(TOOLNAME,
		&IfkManipContextCmd::creator);
	if (!status) {
		MGlobal::displayError("Error registering ifkManipContext command");
		return status;
	}
	status = plugin.registerNode(MANIPNODENAME, IfkManip::id,
		&IfkManip::creator, &IfkManip::initialize,
		MPxNode::kManipContainer);
	if (!status) {
		MGlobal::displayError("Error registering ifkManpulator node");
		return status;
	}
	return status;
}
MStatus uninitializePlugin(MObject obj)
{
	MStatus status;
	MFnPlugin plugin(obj);
	status = plugin.deregisterContextCommand(TOOLNAME);
	if (!status) {
		MGlobal::displayError("Error deregistering ifkManipContext command");
		return status;
	}
	status = plugin.deregisterNode(IfkManip::id);
	if (!status) {
		MGlobal::displayError("Error deregistering ifkManpulator node");
		return status;
	}
	return status;
}
