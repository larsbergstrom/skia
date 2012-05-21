/*
 * Copyright 2012 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GrProgramStageFactory_DEFINED
#define GrProgramStageFactory_DEFINED

#include "GrTypes.h"
#include "SkTemplates.h"

/** Given a GrCustomStage of a particular type, creates the corresponding
    graphics-backend-specific GrProgramStage. Also tracks equivalence
    of shaders generated via a key.
 */

class GrCustomStage;
class GrGLProgramStage;

class GrProgramStageFactory : public GrNoncopyable {
public:
    typedef uint16_t StageKey;
    enum {
        kProgramStageKeyBits = 10,
    };

    virtual StageKey stageKey(const GrCustomStage* stage) const = 0;
    virtual GrGLProgramStage* createGLInstance(
        const GrCustomStage* stage) const = 0;

    bool operator ==(const GrProgramStageFactory& b) const {
        return fStageClassID == b.fStageClassID;
    }
    bool operator !=(const GrProgramStageFactory& b) const {
        return !(*this == b);
    }

protected:
    enum {
        kIllegalStageClassID = 0,
    };

    GrProgramStageFactory() {
        fStageClassID = kIllegalStageClassID;
    }

    static StageKey GenID() {
        // fCurrStageClassID has been initialized to kIllegalStageClassID. The
        // atomic inc returns the old value not the incremented value. So we add
        // 1 to the returned value.
        int32_t id = sk_atomic_inc(&fCurrStageClassID) + 1;
        GrAssert(id < (1 << (8 * sizeof(StageKey) - kProgramStageKeyBits)));
        return id;
    }

    StageKey fStageClassID;

private:
    static int32_t fCurrStageClassID;
};

template <typename StageClass>
class GrTProgramStageFactory : public GrProgramStageFactory {

public:
    typedef typename StageClass::GLProgramStage GLProgramStage; 
    
    /** Returns an value that idenitifes the shader code generated by
        a GrCustomStage. This enables caching of generated shaders. Part of the
        id identifies the GrCustomShader subclass. The remainder is based
        on the aspects of the GrCustomStage object's configuration that affect
        code generation. */
    virtual StageKey stageKey(const GrCustomStage* stage) const SK_OVERRIDE {
        GrAssert(kIllegalStageClassID != fStageClassID);
        StageKey stageID = GLProgramStage::GenKey(stage);
#if GR_DEBUG
        static const StageKey kIllegalIDMask =
            ~((1 << kProgramStageKeyBits) - 1);
        GrAssert(!(kIllegalIDMask & stageID));
#endif
        return fStageClassID | stageID;
    }

    /** Returns a new instance of the appropriate *GL* implementation class
     for the given GrCustomStage; caller is responsible for deleting
     the object. */
    virtual GLProgramStage* createGLInstance(
                        const GrCustomStage* stage) const SK_OVERRIDE {
        return new GLProgramStage(stage);
    }

    static const GrProgramStageFactory& getInstance() {
        static SkAlignedSTStorage<1, GrTProgramStageFactory> gInstanceMem;
        static const GrTProgramStageFactory* gInstance;
        if (!gInstance) {
            gInstance = new (gInstanceMem.get()) GrTProgramStageFactory();
        }
        return *gInstance;
    }

protected:
    GrTProgramStageFactory() {
        fStageClassID = GenID() << kProgramStageKeyBits;
    }
};

#endif
