/*
 * Name: ROP_GroupOut.C
 * Version: 1.0
*/

#include "ROP_GroupOut.h"

#include <OP/OP_Director.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <PRM/PRM_Include.h>
#include <ROP/ROP_Templates.h>
#include <UT/UT_DSOVersion.h>

#include <GU/GU_Detail.h>
#include <iostream.h>


void
newDriverOperator(OP_OperatorTable *table)
{
    table->addOperator(
        new OP_Operator("group_out",
                        "Group Out",
                        ROP_Script::myConstructor,
                        ROP_Script::getTemplatePair(),
                        0,
                        1,
                        ROP_Script::getVariablePair(),
                        OP_FLAG_GENERATOR)
    );
}

static PRM_Name names[] =
{
    PRM_Name("sepparm1", "Separator"),
    PRM_Name("sop_path", "sop path"),
    PRM_Name("outputdir", "output"),
    PRM_Name("sepparm2", "Separator"),
};

static PRM_Default defaults[] =
{
    PRM_Default(0, "/tmp"),
};


static PRM_Template * getTemplates()
{
    static PRM_Template	*theTemplate = 0;

    if (theTemplate)
        return theTemplate;

    // Array large enough to hold our custom parms and all the
    // render script parms.
    theTemplate = new PRM_Template[17];

    // Separator between frame/take parms and the code parm.
    theTemplate[0] = PRM_Template(PRM_SEPARATOR, 1, &names[0]);
    
    theTemplate[1] = PRM_Template(PRM_STRING, PRM_TYPE_DYNAMIC_PATH, 1, &names[1]);
    theTemplate[2] = PRM_Template(PRM_GEOFILE, 1, &names[2],&defaults[0]);

    // Separator between the code parm and the render scripts.
    theTemplate[3] = PRM_Template(PRM_SEPARATOR, 1, &names[3]);

    theTemplate[4] = theRopTemplates[ROP_TPRERENDER_TPLATE];
    theTemplate[5] = theRopTemplates[ROP_PRERENDER_TPLATE];
    theTemplate[6] = theRopTemplates[ROP_LPRERENDER_TPLATE];

    theTemplate[7] = theRopTemplates[ROP_TPREFRAME_TPLATE];
    theTemplate[8] = theRopTemplates[ROP_PREFRAME_TPLATE];
    theTemplate[9] = theRopTemplates[ROP_LPREFRAME_TPLATE];

    theTemplate[10] = theRopTemplates[ROP_TPOSTFRAME_TPLATE];
    theTemplate[11] = theRopTemplates[ROP_POSTFRAME_TPLATE];
    theTemplate[12] = theRopTemplates[ROP_LPOSTFRAME_TPLATE];

    theTemplate[13] = theRopTemplates[ROP_TPOSTRENDER_TPLATE];
    theTemplate[14] = theRopTemplates[ROP_POSTRENDER_TPLATE];
    theTemplate[15] = theRopTemplates[ROP_LPOSTRENDER_TPLATE];

    theTemplate[16] = PRM_Template();

    return theTemplate;
}

OP_TemplatePair *
ROP_Script::getTemplatePair()
{
    static OP_TemplatePair *ropPair = 0;

    if (!ropPair)
    {
        OP_TemplatePair	*base;

        base = new OP_TemplatePair(getTemplates());
        ropPair = new OP_TemplatePair(ROP_Node::getROPbaseTemplate(), base);
    }

    return ropPair;
}

OP_VariablePair *
ROP_Script::getVariablePair()
{
    static OP_VariablePair *pair = 0;

    if (!pair)
        pair = new OP_VariablePair(ROP_Node::myVariableList);

    return pair;
}

OP_Node *
ROP_Script::myConstructor(OP_Network *net,
                          const char *name,
                          OP_Operator *op)
{
    return new ROP_Script(net, name, op);
}

ROP_Script::ROP_Script(OP_Network *net,
                       const char *name,
                       OP_Operator *entry):
    ROP_Node(net, name, entry) {}

int
ROP_Script::startRender(int /*nframes*/, fpreal tstart, fpreal tend)
{
    UT_String sopname;
    myEndTime = tend;
    if (error() < UT_ERROR_ABORT)
        executePreRenderScript(tstart);
    
    SOPNAME(sopname,tstart);
    
    mySOP = OPgetDirector()->findSOPNode(sopname);
    if (!mySOP) {
        addError(0, (const char *)getName());
        return 0;
    }

    return 1;
}

ROP_RENDER_CODE
ROP_Script::renderFrame(fpreal time, UT_Interrupt *)
{
    OP_Context context(time);
    // Execute the pre-frame script.
    executePreFrameScript(time);
    
    auto *gdp = (GU_Detail *)mySOP->getCookedGeo(context);
    if (!gdp) {
        addError(0,(const char *)getName());
        return ROP_ABORT_RENDER;
    }

    auto my_frame = context.getFrame();
    
    UT_String dirname; 
    OUTDIR(dirname ,my_frame);

    // Create a new GDB
    GU_Detail pgdp;
    GA_PrimitiveGroup const *grp;

    GA_FOR_ALL_PRIMGROUPS(gdp, grp) {
        // skip internal groups
        if (grp->getInternal())
            continue;

        // Merge in the prim group
        pgdp.merge(*gdp, grp);

        // Make the group name the file name.
        UT_WorkBuffer buf;
        buf.sprintf("%s/%s.%04d.geo", (const char *)dirname,
                                      (const char *)grp->getName(), 
                                      (int)my_frame);

        // Save the group and stash pointers.
        pgdp.save(buf.buffer(), NULL);
        pgdp.stashAll();
    }
    pgdp.clearAndDestroy();

    // If no problems have been encountered, execute the post-frame
    // script.
    if (error() < UT_ERROR_ABORT)
        executePostFrameScript(time);

    return ROP_CONTINUE_RENDER;
}

ROP_RENDER_CODE
ROP_Script::endRender()
{
    if (error() < UT_ERROR_ABORT)
        executePostRenderScript(myEndTime);
    return ROP_CONTINUE_RENDER;
}

