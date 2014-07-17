//===----------------------------------------------------------------------===//
// SlotTracker Class: Enumerate slot numbers for unnamed values
//===----------------------------------------------------------------------===//
/// This class provides computation of slot numbers for LLVM Assembly writing.
///

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/Constants.h"
#include "llvm/DebugInfo.h"
#include "llvm/IR/DerivedTypes.h"
//#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/Assembly/PrintModulePass.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/TypeFinder.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Dwarf.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/MathExtras.h"
#include <algorithm>
#include <cctype>
using namespace llvm;


class SlotTracker {
public:
    /// ValueMap - A mapping of Values to slot numbers.
    typedef DenseMap<const Value*, unsigned> ValueMap;
    
private:
    /// TheModule - The module for which we are holding slot numbers.
    const Module* TheModule;
    
    /// TheFunction - The function for which we are holding slot numbers.
    const Function* TheFunction;
    bool FunctionProcessed;
    
    /// mMap - The slot map for the module level data.
    ValueMap mMap;
    unsigned mNext;
    
    /// fMap - The slot map for the function level data.
    ValueMap fMap;
    unsigned fNext;
    
    /// mdnMap - Map for MDNodes.
    DenseMap<const MDNode*, unsigned> mdnMap;
    unsigned mdnNext;
    
    /// asMap - The slot map for attribute sets.
    DenseMap<AttributeSet, unsigned> asMap;
    unsigned asNext;
public:
    /// Construct from a module
    explicit SlotTracker(const Module *M);
    /// Construct from a function, starting out in incorp state.
    explicit SlotTracker(const Function *F);
    
    /// Return the slot number of the specified value in it's type
    /// plane.  If something is not in the SlotTracker, return -1.
    int getLocalSlot(const Value *V);
    int getGlobalSlot(const GlobalValue *V);
    int getMetadataSlot(const MDNode *N);
    int getAttributeGroupSlot(AttributeSet AS);
    
    /// If you'd like to deal with a function instead of just a module, use
    /// this method to get its data into the SlotTracker.
    void incorporateFunction(const Function *F) {
        TheFunction = F;
        FunctionProcessed = false;
    }
    
    /// After calling incorporateFunction, use this method to remove the
    /// most recently incorporated function from the SlotTracker. This
    /// will reset the state of the machine back to just the module contents.
    void purgeFunction();
    
    /// MDNode map iterators.
    typedef DenseMap<const MDNode*, unsigned>::iterator mdn_iterator;
    mdn_iterator mdn_begin() { return mdnMap.begin(); }
    mdn_iterator mdn_end() { return mdnMap.end(); }
    unsigned mdn_size() const { return mdnMap.size(); }
    bool mdn_empty() const { return mdnMap.empty(); }
    
    /// AttributeSet map iterators.
    typedef DenseMap<AttributeSet, unsigned>::iterator as_iterator;
    as_iterator as_begin()   { return asMap.begin(); }
    as_iterator as_end()     { return asMap.end(); }
    unsigned as_size() const { return asMap.size(); }
    bool as_empty() const    { return asMap.empty(); }
    
    /// This function does the actual initialization.
    inline void initialize();
    
    // Implementation Details
private:
    /// CreateModuleSlot - Insert the specified GlobalValue* into the slot table.
    void CreateModuleSlot(const GlobalValue *V);
    
    /// CreateMetadataSlot - Insert the specified MDNode* into the slot table.
    void CreateMetadataSlot(const MDNode *N);
    
    /// CreateFunctionSlot - Insert the specified Value* into the slot table.
    void CreateFunctionSlot(const Value *V);
    
    /// \brief Insert the specified AttributeSet into the slot table.
    void CreateAttributeSetSlot(AttributeSet AS);
    
    /// Add all of the module level global variables (and their initializers)
    /// and function declarations, but not the contents of those functions.
    void processModule();
    
    /// Add all of the functions arguments, basic blocks, and instructions.
    void processFunction();
    
    SlotTracker(const SlotTracker &) LLVM_DELETED_FUNCTION;
    void operator=(const SlotTracker &) LLVM_DELETED_FUNCTION;
};

SlotTracker *createSlotTracker(const Module *M) {
    return new SlotTracker(M);
}

static SlotTracker *createSlotTracker(const Value *V) {
    if (const Argument *FA = dyn_cast<Argument>(V))
        return new SlotTracker(FA->getParent());
    
    if (const Instruction *I = dyn_cast<Instruction>(V))
        if (I->getParent())
            return new SlotTracker(I->getParent()->getParent());
    
    if (const BasicBlock *BB = dyn_cast<BasicBlock>(V))
        return new SlotTracker(BB->getParent());
    
    if (const GlobalVariable *GV = dyn_cast<GlobalVariable>(V))
        return new SlotTracker(GV->getParent());
    
    if (const GlobalAlias *GA = dyn_cast<GlobalAlias>(V))
        return new SlotTracker(GA->getParent());
    
    if (const Function *Func = dyn_cast<Function>(V))
        return new SlotTracker(Func);
    
    if (const MDNode *MD = dyn_cast<MDNode>(V)) {
        if (!MD->isFunctionLocal())
            return new SlotTracker(MD->getFunction());
        
        return new SlotTracker((Function *)0);
    }
    
    return 0;
}

#if 0
#define ST_DEBUG(X) dbgs() << X
#else
#define ST_DEBUG(X)
#endif

// Module level constructor. Causes the contents of the Module (sans functions)
// to be added to the slot table.
SlotTracker::SlotTracker(const Module *M)
: TheModule(M), TheFunction(0), FunctionProcessed(false),
mNext(0), fNext(0),  mdnNext(0), asNext(0) {
}

// Function level constructor. Causes the contents of the Module and the one
// function provided to be added to the slot table.
SlotTracker::SlotTracker(const Function *F)
: TheModule(F ? F->getParent() : 0), TheFunction(F), FunctionProcessed(false),
mNext(0), fNext(0), mdnNext(0), asNext(0) {
}

inline void SlotTracker::initialize() {
    if (TheModule) {
        processModule();
        TheModule = 0; ///< Prevent re-processing next time we're called.
    }
    
    if (TheFunction && !FunctionProcessed)
        processFunction();
}

// Iterate through all the global variables, functions, and global
// variable initializers and create slots for them.
void SlotTracker::processModule() {
    ST_DEBUG("begin processModule!\n");
    
    // Add all of the unnamed global variables to the value table.
    for (Module::const_global_iterator I = TheModule->global_begin(),
         E = TheModule->global_end(); I != E; ++I) {
        if (!I->hasName())
            CreateModuleSlot(I);
    }
    
    // Add metadata used by named metadata.
    for (Module::const_named_metadata_iterator
         I = TheModule->named_metadata_begin(),
         E = TheModule->named_metadata_end(); I != E; ++I) {
        const NamedMDNode *NMD = I;
        for (unsigned i = 0, e = NMD->getNumOperands(); i != e; ++i)
            CreateMetadataSlot(NMD->getOperand(i));
    }
    
    for (Module::const_iterator I = TheModule->begin(), E = TheModule->end();
         I != E; ++I) {
        if (!I->hasName())
            // Add all the unnamed functions to the table.
            CreateModuleSlot(I);
        
        // Add all the function attributes to the table.
        // FIXME: Add attributes of other objects?
        AttributeSet FnAttrs = I->getAttributes().getFnAttributes();
        if (FnAttrs.hasAttributes(AttributeSet::FunctionIndex))
            CreateAttributeSetSlot(FnAttrs);
    }
    
    ST_DEBUG("end processModule!\n");
}

// Process the arguments, basic blocks, and instructions  of a function.
void SlotTracker::processFunction() {
    ST_DEBUG("begin processFunction!\n");
    fNext = 0;
    
    // Add all the function arguments with no names.
    for(Function::const_arg_iterator AI = TheFunction->arg_begin(),
        AE = TheFunction->arg_end(); AI != AE; ++AI)
        if (!AI->hasName())
            CreateFunctionSlot(AI);
    
    ST_DEBUG("Inserting Instructions:\n");
    
    SmallVector<std::pair<unsigned, MDNode*>, 4> MDForInst;
    
    // Add all of the basic blocks and instructions with no names.
    for (Function::const_iterator BB = TheFunction->begin(),
         E = TheFunction->end(); BB != E; ++BB) {
        if (!BB->hasName())
            CreateFunctionSlot(BB);
        
        for (BasicBlock::const_iterator I = BB->begin(), E = BB->end(); I != E;
             ++I) {
            if (!I->getType()->isVoidTy() && !I->hasName())
                CreateFunctionSlot(I);
            
            // Intrinsics can directly use metadata.  We allow direct calls to any
            // llvm.foo function here, because the target may not be linked into the
            // optimizer.
            if (const CallInst *CI = dyn_cast<CallInst>(I)) {
                if (Function *F = CI->getCalledFunction())
                    if (F->isIntrinsic())
                        for (unsigned i = 0, e = I->getNumOperands(); i != e; ++i)
                            if (MDNode *N = dyn_cast_or_null<MDNode>(I->getOperand(i)))
                                CreateMetadataSlot(N);
                
                // Add all the call attributes to the table.
                AttributeSet Attrs = CI->getAttributes().getFnAttributes();
                if (Attrs.hasAttributes(AttributeSet::FunctionIndex))
                    CreateAttributeSetSlot(Attrs);
            } else if (const InvokeInst *II = dyn_cast<InvokeInst>(I)) {
                // Add all the call attributes to the table.
                AttributeSet Attrs = II->getAttributes().getFnAttributes();
                if (Attrs.hasAttributes(AttributeSet::FunctionIndex))
                    CreateAttributeSetSlot(Attrs);
            }
            
            // Process metadata attached with this instruction.
            I->getAllMetadata(MDForInst);
            for (unsigned i = 0, e = MDForInst.size(); i != e; ++i)
                CreateMetadataSlot(MDForInst[i].second);
            MDForInst.clear();
        }
    }
    
    FunctionProcessed = true;
    
    ST_DEBUG("end processFunction!\n");
}

/// Clean up after incorporating a function. This is the only way to get out of
/// the function incorporation state that affects get*Slot/Create*Slot. Function
/// incorporation state is indicated by TheFunction != 0.
void SlotTracker::purgeFunction() {
    ST_DEBUG("begin purgeFunction!\n");
    fMap.clear(); // Simply discard the function level map
    TheFunction = 0;
    FunctionProcessed = false;
    ST_DEBUG("end purgeFunction!\n");
}

/// getGlobalSlot - Get the slot number of a global value.
int SlotTracker::getGlobalSlot(const GlobalValue *V) {
    // Check for uninitialized state and do lazy initialization.
    initialize();
    
    // Find the value in the module map
    ValueMap::iterator MI = mMap.find(V);
    return MI == mMap.end() ? -1 : (int)MI->second;
}

/// getMetadataSlot - Get the slot number of a MDNode.
int SlotTracker::getMetadataSlot(const MDNode *N) {
    // Check for uninitialized state and do lazy initialization.
    initialize();
    
    // Find the MDNode in the module map
    mdn_iterator MI = mdnMap.find(N);
    return MI == mdnMap.end() ? -1 : (int)MI->second;
}


/// getLocalSlot - Get the slot number for a value that is local to a function.
int SlotTracker::getLocalSlot(const Value *V) {
    assert(!isa<Constant>(V) && "Can't get a constant or global slot with this!");
    
    // Check for uninitialized state and do lazy initialization.
    initialize();
    
    ValueMap::iterator FI = fMap.find(V);
    return FI == fMap.end() ? -1 : (int)FI->second;
}

int SlotTracker::getAttributeGroupSlot(AttributeSet AS) {
    // Check for uninitialized state and do lazy initialization.
    initialize();
    
    // Find the AttributeSet in the module map.
    as_iterator AI = asMap.find(AS);
    return AI == asMap.end() ? -1 : (int)AI->second;
}

/// CreateModuleSlot - Insert the specified GlobalValue* into the slot table.
void SlotTracker::CreateModuleSlot(const GlobalValue *V) {
    assert(V && "Can't insert a null Value into SlotTracker!");
    assert(!V->getType()->isVoidTy() && "Doesn't need a slot!");
    assert(!V->hasName() && "Doesn't need a slot!");
    
    unsigned DestSlot = mNext++;
    mMap[V] = DestSlot;
    
    ST_DEBUG("  Inserting value [" << V->getType() << "] = " << V << " slot=" <<
             DestSlot << " [");
    // G = Global, F = Function, A = Alias, o = other
    ST_DEBUG((isa<GlobalVariable>(V) ? 'G' :
              (isa<Function>(V) ? 'F' :
               (isa<GlobalAlias>(V) ? 'A' : 'o'))) << "]\n");
}

/// CreateSlot - Create a new slot for the specified value if it has no name.
void SlotTracker::CreateFunctionSlot(const Value *V) {
    assert(!V->getType()->isVoidTy() && !V->hasName() && "Doesn't need a slot!");
    
    unsigned DestSlot = fNext++;
    fMap[V] = DestSlot;
    
    // G = Global, F = Function, o = other
    ST_DEBUG("  Inserting value [" << V->getType() << "] = " << V << " slot=" <<
             DestSlot << " [o]\n");
}

/// CreateModuleSlot - Insert the specified MDNode* into the slot table.
void SlotTracker::CreateMetadataSlot(const MDNode *N) {
    assert(N && "Can't insert a null Value into SlotTracker!");
    
    // Don't insert if N is a function-local metadata, these are always printed
    // inline.
    if (!N->isFunctionLocal()) {
        mdn_iterator I = mdnMap.find(N);
        if (I != mdnMap.end())
            return;
        
        unsigned DestSlot = mdnNext++;
        mdnMap[N] = DestSlot;
    }
    
    // Recursively add any MDNodes referenced by operands.
    for (unsigned i = 0, e = N->getNumOperands(); i != e; ++i)
        if (const MDNode *Op = dyn_cast_or_null<MDNode>(N->getOperand(i)))
            CreateMetadataSlot(Op);
}

void SlotTracker::CreateAttributeSetSlot(AttributeSet AS) {
    assert(AS.hasAttributes(AttributeSet::FunctionIndex) &&
           "Doesn't need a slot!");
    
    as_iterator I = asMap.find(AS);
    if (I != asMap.end())
        return;
    
    unsigned DestSlot = asNext++;
    asMap[AS] = DestSlot;
}
