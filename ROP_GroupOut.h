/*
 * Name: ROP_GroupOut.h
 * Version: 1.0
*/

#ifndef __ROP_GroupOut_h__
#define __ROP_GroupOut_h__

#include <ROP/ROP_Node.h>
#include <SOP/SOP_Node.h>

class OP_TemplatePair;
class OP_VariablePair;

class ROP_Script: public ROP_Node
{
public:
    static OP_Node              *myConstructor(OP_Network *,
                                              const char *,
                                              OP_Operator *);
    static OP_TemplatePair      *getTemplatePair();
    static OP_VariablePair      *getVariablePair();

protected:

    ROP_Script(OP_Network *, const char *, OP_Operator *);
    virtual ~ROP_Script() {};
    virtual int                 startRender(int nframes, fpreal s, fpreal e);
    virtual ROP_RENDER_CODE     renderFrame(fpreal time, UT_Interrupt *boss);
    virtual ROP_RENDER_CODE     endRender();

private:
    void        SOPNAME(UT_String &str, fpreal t){ evalString(str, "sop_path", 0, t); }
    void        OUTDIR(UT_String &str, fpreal t) { evalString(str, "outputdir", 0, t); }
    fpreal      myEndTime;
    SOP_Node   *mySOP;

};

#endif

