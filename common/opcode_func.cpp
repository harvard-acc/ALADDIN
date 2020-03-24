#include <string>
#include "opcode_func.h"

std::string opcode_name(uint8_t opcode) {
#define LLVM_IR_OPCODE_TO_NAME(Opcode)                                         \
  case LLVM_IR_##Opcode:                                                       \
    return std::string(#Opcode)

  switch (opcode) {
    LLVM_IR_OPCODE_TO_NAME(Move);
    LLVM_IR_OPCODE_TO_NAME(Ret);
    LLVM_IR_OPCODE_TO_NAME(Br);
    LLVM_IR_OPCODE_TO_NAME(Switch);
    LLVM_IR_OPCODE_TO_NAME(IndirectBr);
    LLVM_IR_OPCODE_TO_NAME(Invoke);
    LLVM_IR_OPCODE_TO_NAME(Resume);
    LLVM_IR_OPCODE_TO_NAME(Unreachable);
    LLVM_IR_OPCODE_TO_NAME(Add);
    LLVM_IR_OPCODE_TO_NAME(FAdd);
    LLVM_IR_OPCODE_TO_NAME(Sub);
    LLVM_IR_OPCODE_TO_NAME(FSub);
    LLVM_IR_OPCODE_TO_NAME(Mul);
    LLVM_IR_OPCODE_TO_NAME(FMul);
    LLVM_IR_OPCODE_TO_NAME(UDiv);
    LLVM_IR_OPCODE_TO_NAME(SDiv);
    LLVM_IR_OPCODE_TO_NAME(FDiv);
    LLVM_IR_OPCODE_TO_NAME(URem);
    LLVM_IR_OPCODE_TO_NAME(SRem);
    LLVM_IR_OPCODE_TO_NAME(FRem);
    LLVM_IR_OPCODE_TO_NAME(Shl);
    LLVM_IR_OPCODE_TO_NAME(LShr);
    LLVM_IR_OPCODE_TO_NAME(AShr);
    LLVM_IR_OPCODE_TO_NAME(And);
    LLVM_IR_OPCODE_TO_NAME(Or);
    LLVM_IR_OPCODE_TO_NAME(Xor);
    LLVM_IR_OPCODE_TO_NAME(Alloca);
    LLVM_IR_OPCODE_TO_NAME(Load);
    LLVM_IR_OPCODE_TO_NAME(Store);
    LLVM_IR_OPCODE_TO_NAME(GetElementPtr);
    LLVM_IR_OPCODE_TO_NAME(Fence);
    LLVM_IR_OPCODE_TO_NAME(AtomicCmpXchg);
    LLVM_IR_OPCODE_TO_NAME(AtomicRMW);
    LLVM_IR_OPCODE_TO_NAME(Trunc);
    LLVM_IR_OPCODE_TO_NAME(ZExt);
    LLVM_IR_OPCODE_TO_NAME(SExt);
    LLVM_IR_OPCODE_TO_NAME(FPToUI);
    LLVM_IR_OPCODE_TO_NAME(FPToSI);
    LLVM_IR_OPCODE_TO_NAME(UIToFP);
    LLVM_IR_OPCODE_TO_NAME(SIToFP);
    LLVM_IR_OPCODE_TO_NAME(FPTrunc);
    LLVM_IR_OPCODE_TO_NAME(FPExt);
    LLVM_IR_OPCODE_TO_NAME(PtrToInt);
    LLVM_IR_OPCODE_TO_NAME(IntToPtr);
    LLVM_IR_OPCODE_TO_NAME(BitCast);
    LLVM_IR_OPCODE_TO_NAME(AddrSpaceCast);
    LLVM_IR_OPCODE_TO_NAME(ICmp);
    LLVM_IR_OPCODE_TO_NAME(FCmp);
    LLVM_IR_OPCODE_TO_NAME(PHI);
    LLVM_IR_OPCODE_TO_NAME(Call);
    LLVM_IR_OPCODE_TO_NAME(Select);
    LLVM_IR_OPCODE_TO_NAME(VAArg);
    LLVM_IR_OPCODE_TO_NAME(ExtractElement);
    LLVM_IR_OPCODE_TO_NAME(InsertElement);
    LLVM_IR_OPCODE_TO_NAME(ShuffleVector);
    LLVM_IR_OPCODE_TO_NAME(ExtractValue);
    LLVM_IR_OPCODE_TO_NAME(InsertValue);
    LLVM_IR_OPCODE_TO_NAME(LandingPad);
    LLVM_IR_OPCODE_TO_NAME(SetReadyBits);
    LLVM_IR_OPCODE_TO_NAME(EntryDecl);
    LLVM_IR_OPCODE_TO_NAME(DMAFence);
    LLVM_IR_OPCODE_TO_NAME(DMAStore);
    LLVM_IR_OPCODE_TO_NAME(DMALoad);
    LLVM_IR_OPCODE_TO_NAME(HostStore);
    LLVM_IR_OPCODE_TO_NAME(HostLoad);
    LLVM_IR_OPCODE_TO_NAME(IndexAdd);
    LLVM_IR_OPCODE_TO_NAME(SilentStore);
    LLVM_IR_OPCODE_TO_NAME(SpecialMathOp);
    LLVM_IR_OPCODE_TO_NAME(Intrinsic);
    LLVM_IR_OPCODE_TO_NAME(SetSamplingFactor);
    default:
      return "UNKNOWN_OPCODE";
  }

#undef LLVM_IR_OPCODE_TO_NAME
}
