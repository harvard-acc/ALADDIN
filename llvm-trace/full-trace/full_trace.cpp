namespace std { class type_info; }
#include <vector>
#include <cmath>
#include "llvm/Pass.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Metadata.h"
#include "llvm/DebugInfo.h"
#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <string>
#include "SlotTracker.h"

#define NUM_OF_INTRINSICS 33
//#define FUNC 56
//#define TOP 20
#define INST_TYPES 60
//#define OPER 7
#define RESULT_LINE 19134
using namespace llvm;
using namespace std;

char list_of_intrinsics[NUM_OF_INTRINSICS][25] = {
	"llvm.memcoy",	// standard C lib
	"llvm.memmove",
	"llvm.memset",
	"llvm.sqrt",
	"llvm.powi",
	"llvm.sin",
	"llvm.cos",
	"llvm.pow",
	"llvm.exp",
	"llvm.exp2",
	"llvm.log",
	"llvm.log10",
	"llvm.log2",
	"llvm.fma",
	"llvm.fabs",
	"llvm.copysign",
	"llvm.floor",
	"llvm.ceil",
	"llvm.trunc",
	"llvm.rint",
	"llvm.nearbyint",
	"llvm.round",	
	"llvm.bswap",	//bit manipulation
	"llvm.ctpop",
	"llvm.ctlz",
	"llvm.cttz",
	"llvm.sadd.with.overflow",	//arithmetic with overflow
	"llvm.uadd.with.overflow",
	"llvm.ssub.with.overflow",
	"llvm.usub.with.overflow",
	"llvm.smul.with.overflow",
	"llvm.umul.with.overflow",
	"llvm.fmuladd"		//specialised arithmetic
 };


FILE *static_trace;
void close_static_trace_file()
{
  fclose(static_trace);
}
namespace {

  struct fullTrace : public BasicBlockPass {
    static char ID;
    
    // External trace_logger function
    Value *TL_log0, *TL_log_int, *TL_log_double, *TL_log_int_noreg, *TL_log_double_noreg; 
    
    fullTrace() : BasicBlockPass(ID) {}
    
    Module *curr_module;
    SlotTracker *st;
    Function *curr_function;
    
    char **functions;
    int num_of_functions;
    
    char ** str_split (char *a_str, const char a_delim, int *size)
    {
      int count = 0;
      char *tmp = a_str;
      char *last_comma = 0;
      char delim[2];
      delim[0] = a_delim;
      delim[1] = 0;

      while (*tmp)
      {
        if (a_delim == *tmp)
        {
          count++;
          last_comma = tmp;
        }
        tmp++;
      }
      count++;

      char **result;
      result = (char **) malloc (sizeof(char *) * count);
      if (result)
      {
        int idx = 0;
        char * token = strtok(a_str, delim);
        while (token)
        {
          assert(idx < count);
          *(result + idx) = strdup(token);
          idx++;
          token = strtok(0, delim);
        }
      }
      *size = count;
      return result;
    }
    
    virtual bool doInitialization(Module &M) 
    {
      static_trace = fopen("static_trace", "w");
      atexit(&close_static_trace_file);
      // Add external trace_logger function declaratio
      TL_log0 = M.getOrInsertFunction( "trace_logger_log0", Type::getVoidTy(M.getContext()), 
                Type::getInt64Ty(M.getContext()), 
                Type::getInt8PtrTy((M.getContext())),
                Type::getInt8PtrTy((M.getContext())),
                Type::getInt8PtrTy((M.getContext())),
                Type::getInt64Ty(M.getContext()), 
                NULL );
      
      TL_log_int = M.getOrInsertFunction( "trace_logger_log_int", Type::getVoidTy(M.getContext()),
                   Type::getInt64Ty(M.getContext()),
                   Type::getInt64Ty(M.getContext()),
                   Type::getInt64Ty(M.getContext()),
                   Type::getInt64Ty(M.getContext()),
                   Type::getInt8PtrTy((M.getContext())),
                   NULL );
      
      TL_log_double = M.getOrInsertFunction( "trace_logger_log_double", Type::getVoidTy(M.getContext()),
                      Type::getInt64Ty(M.getContext()),
                      Type::getInt64Ty(M.getContext()),
                      Type::getDoubleTy(M.getContext()),
                      Type::getInt64Ty(M.getContext()),
                      Type::getInt8PtrTy((M.getContext())),
                      NULL );

      TL_log_int_noreg = M.getOrInsertFunction( "trace_logger_log_int_noreg", Type::getVoidTy(M.getContext()),
                         Type::getInt64Ty(M.getContext()),
                         Type::getInt64Ty(M.getContext()),
                         Type::getInt64Ty(M.getContext()),
                         Type::getInt64Ty(M.getContext()),
                         NULL );
      
      TL_log_double_noreg = M.getOrInsertFunction( "trace_logger_log_double_noreg", Type::getVoidTy(M.getContext()),
                            Type::getInt64Ty(M.getContext()),
                            Type::getInt64Ty(M.getContext()),
                            Type::getDoubleTy(M.getContext()),
                            Type::getInt64Ty(M.getContext()),
                            NULL );
      char *func_string; 
      func_string = getenv("WORKLOAD");
      if (func_string == NULL)
      {
        cerr<<"Please set WORKLOAD as an environment variable!\n";
        return false;
      }
      fprintf(stderr, "workload = %s\n", func_string);
      functions = str_split(func_string, ',', &num_of_functions);

      st = createSlotTracker(&M);
      st->initialize();
      curr_module = &M;
      curr_function = NULL;
      return false;
    }
	
    int trace_or_not(char* func)
    {
      if (is_tracking_function(func))
        return 1;
      for(int i = 0; i < NUM_OF_INTRINSICS; i++)
        if(strstr(func, list_of_intrinsics[i]) == func)
          return i;
      return -1;
	  }

    bool is_tracking_function(string func)
    {
      for(int i=0;i<num_of_functions;i++)
        if(strcmp(*(functions + i), func.c_str()) == 0)
          return true;
      return false;
    }

    int getMemSize(Type *T)
    {
      int size = 0;
      if(T->isPointerTy())
        return getMemSize(T->getPointerElementType());
        //size = curr_module->getDataLayout()->getPointerTypeSizeInBits(T);
      else if(T->isFunctionTy())
        size = 0;	
      else if(T->isLabelTy())
        size = 0;
      else if(T->isStructTy())
      {
        StructType *S = dyn_cast<StructType>(T);
        for(unsigned i = 0; i != S->getNumElements(); i++)
        {
          Type *t = S->getElementType(i);
          size += getMemSize(t);	
        }
      }
      else if(T->isFloatingPointTy())
      {
        switch(T->getTypeID())
        {
          case llvm::Type::HalfTyID:        ///<  1: 16-bit floating point typ
            size = 16; break;
          case llvm::Type::FloatTyID:       ///<  2: 32-bit floating point type
            size = 4*8; break;
          case llvm::Type::DoubleTyID:      ///<  3: 64-bit floating point type
            size = 8*8; break;
          case llvm::Type::X86_FP80TyID:    ///<  4: 80-bit floating point type (X87)
            size = 10*8; break;
          case llvm::Type::FP128TyID:       ///<  5: 128-bit floating point type (112-bit mantissa)
            size = 16*8; break;
          case llvm::Type::PPC_FP128TyID:   ///<  6: 128-bit floating point type (two 64-bits, PowerPC)
            size = 16*8; break;
          default:
            fprintf(stderr, "!!Unknown floating point type size\n");
            assert(false && "Unknown floating point type size");
         }
      }
      else if(T->isIntegerTy())
        size = cast<IntegerType>(T)->getBitWidth();
      else if(T->isVectorTy())
        size = cast<VectorType>(T)->getBitWidth();
      else if(T->isArrayTy())
      {
        ArrayType *A = dyn_cast<ArrayType>(T);
        size = (int) A->getNumElements()* A->getElementType()->getPrimitiveSizeInBits();
      }
      else 
      {
        fprintf(stderr, "!!Unknown data type: %d\n", T->getTypeID());
        assert(false && "Unknown data type");
      }
                    
      return size;
    }

    //s - function ID or register ID or label name
    //opty - opcode or data type
    void print_line(BasicBlock::iterator itr, int line, int line_number, char *func_or_reg_id, 
              char *bbID, char *instID, int opty, 
		          int datasize=0, Value *value=NULL, bool is_reg=0)
    {
	
	    CallInst *tl_call;
      IRBuilder<> IRB(itr);

      Value *v_line, *v_opty, *v_value, *v_linenumber;
      v_line = ConstantInt::get(IRB.getInt64Ty(), line);
      v_opty = ConstantInt::get(IRB.getInt64Ty(), opty);
      v_linenumber = ConstantInt::get(IRB.getInt64Ty(), line_number);

      //print line 0	
      if(line==0)
      {
	      fprintf(static_trace, "\n%d,%s,%s,%s,%d\n", line, func_or_reg_id, bbID, instID, opty);
        Constant *v_func_id = ConstantDataArray::getString(curr_module->getContext(), func_or_reg_id, true);
        ArrayType* ArrayTy_0 = ArrayType::get(IntegerType::get(curr_module->getContext(), 8), (strlen(func_or_reg_id) + 1));
        GlobalVariable *gvar_array = new GlobalVariable(*curr_module, ArrayTy_0,
                                                        true, GlobalValue::PrivateLinkage, 0, ".str");
        gvar_array->setInitializer(v_func_id);
        std::vector<Constant*> indices;
        ConstantInt *zero = ConstantInt::get(curr_module->getContext(), APInt(32, StringRef("0"), 10));
        indices.push_back(zero);
        indices.push_back(zero);

        Constant * vv_func_id = ConstantExpr::getGetElementPtr(gvar_array, indices);

	      Constant *v_bb = ConstantDataArray::getString(curr_module->getContext(), bbID, true);
        ArrayTy_0 = ArrayType::get(IntegerType::get(curr_module->getContext(), 8), (strlen(bbID) + 1));
        gvar_array = new GlobalVariable(*curr_module, ArrayTy_0,
        	                          true, GlobalValue::PrivateLinkage, 0, ".str");
        gvar_array->setInitializer(v_bb);
        zero = ConstantInt::get(curr_module->getContext(), APInt(32, StringRef("0"), 10));
        Constant * vv_bb = ConstantExpr::getGetElementPtr(gvar_array, indices);

	      Constant *v_inst = ConstantDataArray::getString(curr_module->getContext(), instID, true);
        ArrayTy_0 = ArrayType::get(IntegerType::get(curr_module->getContext(), 8), (strlen(instID) + 1));
        gvar_array = new GlobalVariable(*curr_module, ArrayTy_0,
                                                true, GlobalValue::PrivateLinkage, 0, ".str");
        gvar_array->setInitializer(v_inst);
        zero = ConstantInt::get(curr_module->getContext(), APInt(32, StringRef("0"), 10));
        Constant * vv_inst = ConstantExpr::getGetElementPtr(gvar_array, indices);

        tl_call = IRB.CreateCall5(TL_log0, v_linenumber, vv_func_id, vv_bb, vv_inst, v_opty);
	    }
      //print line with reg
	    else
      {   
	      if (line == RESULT_LINE)
          fprintf(static_trace, "r,");
        else
          fprintf(static_trace, "%d,", line);
        
        if (func_or_reg_id == NULL)
          fprintf(static_trace, "%d,%d,%d\n", opty, datasize, is_reg);
        else
          fprintf(static_trace, "%d,%d,%d,%s\n", opty, datasize, is_reg, func_or_reg_id);
		    
		    Value *v_is_reg;
        v_is_reg = ConstantInt::get(IRB.getInt64Ty(), is_reg);
		    Value *v_size;
        v_size = ConstantInt::get(IRB.getInt64Ty(), datasize);
        
        if (func_or_reg_id != NULL)
        {
          Constant *v_reg_id = ConstantDataArray::getString(curr_module->getContext(), func_or_reg_id, true);
          ArrayType* ArrayTy_0 = ArrayType::get(IntegerType::get(curr_module->getContext(), 8), 
              (strlen(func_or_reg_id) + 1));
          GlobalVariable *gvar_array = new GlobalVariable(*curr_module, ArrayTy_0,
                                                      true, GlobalValue::PrivateLinkage, 0, ".str");
          gvar_array->setInitializer(v_reg_id);
          std::vector<Constant*> indices;
          ConstantInt *zero = ConstantInt::get(curr_module->getContext(), APInt(32, StringRef("0"), 10));
          indices.push_back(zero);
          indices.push_back(zero);
          Constant * vv_reg_id = ConstantExpr::getGetElementPtr(gvar_array, indices);
          
          if(value != NULL)
          {
            if(opty == llvm::Type::IntegerTyID)
            {
              v_value = IRB.CreateZExt(value, IRB.getInt64Ty());
              tl_call = IRB.CreateCall5(TL_log_int, v_line, v_size, v_value, v_is_reg, vv_reg_id);
            }
            else if(opty >= llvm::Type::HalfTyID &&opty<=llvm::Type::PPC_FP128TyID)
            {
              v_value = IRB.CreateFPExt(value, IRB.getDoubleTy());
              tl_call = IRB.CreateCall5(TL_log_double, v_line, v_size, v_value, v_is_reg, vv_reg_id);
            }
            // deal with functions individually
            else if(opty==llvm::Type::PointerTyID)
            {
              v_value = IRB.CreatePtrToInt(value, IRB.getInt64Ty());
              tl_call = IRB.CreateCall5(TL_log_int, v_line, v_size, v_value, v_is_reg, vv_reg_id);
            }
            else
              fprintf(stderr, "normal data else: %d, %s\n",opty, func_or_reg_id);             
          }
          else if (value == NULL &&  bbID != NULL && strcmp(bbID, "phi") == 0 )
          {
            v_value = ConstantInt::get(IRB.getInt64Ty(), 999);
            tl_call = IRB.CreateCall5(TL_log_int, v_line, v_size, v_value, v_is_reg, vv_reg_id);
          }
          else
          {
            v_value = ConstantInt::get(IRB.getInt64Ty(), 0);
            tl_call = IRB.CreateCall5(TL_log_int, v_line, v_size, v_value, v_is_reg, vv_reg_id);
          }
          //else
            //fprintf(stderr, "normal data else: %d, %s\n",opty, func_or_reg_id);             
        }
        else
        {
          if(value != NULL)
          {
            if(opty == llvm::Type::IntegerTyID)
            {
              v_value = IRB.CreateZExt(value, IRB.getInt64Ty());
              tl_call = IRB.CreateCall4(TL_log_int_noreg, v_line, v_size, v_value, v_is_reg);
            }
            else if(opty >= llvm::Type::HalfTyID &&opty<=llvm::Type::PPC_FP128TyID)
            {
              v_value = IRB.CreateFPExt(value, IRB.getDoubleTy());
              tl_call = IRB.CreateCall4(TL_log_double_noreg, v_line, v_size, v_value, v_is_reg);
            }
            // deal with functions individually
            else if(opty==llvm::Type::PointerTyID)
            {
              v_value = IRB.CreatePtrToInt(value, IRB.getInt64Ty());
              tl_call = IRB.CreateCall4(TL_log_int_noreg, v_line, v_size, v_value, v_is_reg);
            }
            else
              fprintf(stderr, "normal data else: %d\n",opty);             
          }
          else if (value == NULL &&  bbID != NULL && strcmp(bbID, "phi") == 0 )
          {
            v_value = ConstantInt::get(IRB.getInt64Ty(), 999);
            tl_call = IRB.CreateCall4(TL_log_int_noreg, v_line, v_size, v_value, v_is_reg);
          }
          else
            fprintf(stderr, "normal data else: %d\n",opty);             
        }
	    }
    }

	  void getInstId(Instruction *itr, char* bbid, char* instid, int &instc)
    {
		  int id = st->getLocalSlot(itr);
      bool f = itr->hasName();
      if(f) 
        strcpy(instid, (char*)itr->getName().str().c_str());
      if(!f && id >= 0)
        sprintf(instid, "%d", id);
      else if(!f && id==-1) //strcpy(instid, bad);
      {       
        char tmp[10];
        char dash[5] = "-";
        sprintf(tmp, "%d", instc);
        if(bbid!=NULL)
          strcpy(instid, bbid);
        strcat(instid, dash);
        strcat(instid, tmp);
        instc++;
      }

	  }
	
	  void getBBId(Value *BB, char *bbid)
    {
		  int id;
      id = st->getLocalSlot(BB);
      bool hasName = BB->hasName();
      if(hasName) 
        strcpy(bbid, (char*)BB->getName().str().c_str());
      if(!hasName && id >= 0) 
        sprintf(bbid, "%d", id);
      else if(!hasName && id==-1) 
        fprintf(stderr, "!!This block does not have a name or a ID!\n");
	  }

    virtual bool runOnBasicBlock(BasicBlock &BB) 
    {
      Function *F = BB.getParent();
      int instc = 0;
      char funcName[256];

      if(curr_function != F)
      {
        st->purgeFunction();
        st->incorporateFunction(F);
        curr_function = F;
      }
	    strcpy(funcName, curr_function->getName().str().c_str());
	    if(!is_tracking_function(funcName)) 
        return false;
      
      cout<<"Tracking function: " << funcName<<endl;
      //deal with phi nodes
      BasicBlock::iterator insertp = BB.getFirstInsertionPt();
      BasicBlock::iterator itr = BB.begin();
      //if (dyn_cast<PHINode>(itr))
      for(BasicBlock::iterator itr = BB.begin(); PHINode *N = dyn_cast<PHINode>(itr) ; itr++)
      {	
		    Value *curr_operand=NULL;
  	    int size = 0, opcode;
        char bbid[256], instid[256];
        char operR[256];
        int DataSize, value;
		    
        int line_number = -1;

        getBBId(&BB, bbid);
        getInstId(itr, bbid, instid, instc);
        opcode = itr->getOpcode();
        
        if (MDNode *N = itr->getMetadata("dbg")) 
        {  
          DILocation Loc(N);                      // DILocation is in DebugInfo.h
          line_number = Loc.getLineNumber();
        }
		    
        print_line(insertp, 0, line_number, funcName, bbid, instid, opcode);

        //for instructions using registers
	      int i,num_of_operands = itr->getNumOperands();
        
        if(num_of_operands > 0)
        {
		      char phi[5] = "phi";
          for(i = num_of_operands - 1; i >= 0; i--)
          {
            curr_operand = itr->getOperand(i);
            if(Instruction *I = dyn_cast<Instruction>(curr_operand))
            {
              bool is_reg = 1;
              int flag = 0;
              getInstId(I, NULL, operR, flag);
              if(flag > 0) 
                fprintf(stderr, "!!This operand instruction does not have a name or ID!\n");
              if(curr_operand->getType()->isVectorTy())
              {
                print_line(insertp, i+1, -1, operR, phi, NULL,
                            curr_operand->getType()->getTypeID(),
                              getMemSize(curr_operand->getType()), 
                              NULL,
                              is_reg);
              }
              else
              {
                print_line(insertp, i+1, -1, operR, phi, NULL,
                            I->getType()->getTypeID(),
                              getMemSize(I->getType()), 
                              NULL, 
                              is_reg);
              }
            }
            else
            {
              bool is_reg = 0;
              print_line(insertp, i+1, -1, NULL, phi, NULL,
                          curr_operand->getType()->getTypeID(),
                            getMemSize(curr_operand->getType()),
                              curr_operand, 
                              is_reg);
            }
          }
        }
        //Get result info: resR = instID
		    if(!itr->getType()->isVoidTy())
        {
          if(itr->getType()->isVectorTy())
          {
            bool is_reg = 1;
            print_line(insertp, RESULT_LINE, -1, instid, NULL, NULL,
                        itr->getType()->getTypeID(),
                          getMemSize(itr->getType()),
                            NULL, is_reg);
          }
          else if(itr->isTerminator())
            fprintf(stderr, "It is terminator...\n");
          else
          {
            bool is_reg = 1;
            print_line(insertp, RESULT_LINE, -1, instid, NULL, NULL,
                        itr->getType()->getTypeID(),
                          getMemSize(itr->getType()),
                            itr, is_reg);
          }
        }
	    }

      //for ALL instructions
      BasicBlock::iterator nextitr;
	    for( BasicBlock::iterator itr = insertp; itr !=BB.end();itr = nextitr)
      {
        Value *curr_operand=NULL;
        int size = 0, opcode;
        char bbid[256], instid[256];
        char operR[256];
        int DataSize, value;
        int line_number = -1;

        nextitr=itr;
        nextitr++;

        //Get static BasicBlock ID: produce bbid
        getBBId(&BB, bbid);	
        
        //Get static instruction ID: produce instid
        getInstId(itr, bbid, instid, instc);
        
        //Get opcode: produce opcode
        opcode = itr->getOpcode();

        if (MDNode *N = itr->getMetadata("dbg")) 
        {  
          DILocation Loc(N);                      // DILocation is in DebugInfo.h
          line_number = Loc.getLineNumber();
        }
        
        if(CallInst *I = dyn_cast<CallInst>(itr))
        {
          char callfunc[30];
          strcpy(callfunc, I->getCalledFunction()->getName().str().c_str());
          if(trace_or_not(callfunc)==-1)
            continue;
        }
        print_line(itr, 0, line_number, funcName, bbid, instid, opcode);

        //for instructions using registers
	      //Get registers: operR1, operR2, resR
	      int i, num_of_operands = itr->getNumOperands();
	
	      //if(num_of_operands > OPER) 
          //fprintf(stderr, "!!More operands than expected! It has %d for opcode name %s.\n", num_of_operands, itr->getOpcodeName());
	      if(num_of_operands > 0 )
        {
          for(i = num_of_operands - 1; i >= 0; i--)
          {
			      curr_operand = itr->getOperand(i);
            int flag = 0;
            if(Instruction *I = dyn_cast<Instruction>(curr_operand))
            {
              //flag = 0;
              bool is_reg = 1;
              getInstId(I, NULL, operR, flag);
              if(flag>0) 
                fprintf(stderr, "!!This operand instruction does not have a name or ID!\n");
              if(curr_operand->getType()->isVectorTy())
                print_line(itr, i+1, -1, operR, NULL, NULL,
                            curr_operand->getType()->getTypeID(),
                              getMemSize(curr_operand->getType()), 
                              NULL,
                              is_reg);
              else 
                print_line(itr, i+1, -1, operR, NULL, NULL, 
                            I->getType()->getTypeID(), 
                              getMemSize(I->getType()), 
                              curr_operand, 
                              is_reg);
            }
            else
            {
              bool is_reg = 0;
              if(curr_operand->getType()->isVectorTy())
              {
                char operand_id[256];
                strcpy(operand_id, curr_operand->getName().str().c_str());
                print_line(itr, i+1, -1, operand_id, NULL, NULL, 
                    //print_line(itr, i+1, NULL, NULL, NULL,
                            curr_operand->getType()->getTypeID(),
                              getMemSize(curr_operand->getType()), 
                              NULL,
                              is_reg);
              }
              
              else if(curr_operand->getType()->isLabelTy())
              {
                char label_id[256];
                getBBId(curr_operand, label_id);
                print_line(itr, i+1, -1, label_id, NULL, NULL, 
                            curr_operand->getType()->getTypeID(),
                              getMemSize(curr_operand->getType()), 
                              NULL,
                              is_reg);
              } 
              // is function 
              else if(curr_operand->getValueID()==2)
              { 
                char func_id[256];
                strcpy(func_id, curr_operand->getName().str().c_str());
                print_line(itr, i+1, -1, func_id, NULL, NULL,
                            curr_operand->getType()->getTypeID(),
                              getMemSize(curr_operand->getType()), 
                              NULL,
                              is_reg);
              }
              else 
              {
                //printf("here %d %d\n", curr_operand->getType()->getTypeID(), curr_operand->getValueID());
                char operand_id[256];
                strcpy(operand_id, curr_operand->getName().str().c_str());
                print_line(itr, i+1, -1, operand_id, NULL, NULL, 
                            curr_operand->getType()->getTypeID(),
                              getMemSize(curr_operand->getType()), 
                                curr_operand, 
                                is_reg);
              }
            }	
		      }	
	      }

        //for CALL instrctions
        /*
        if(itr->getOpcode() == Instruction::Call)
        {
          const CallInst *CI = dyn_cast<CallInst>(itr);
          Function *tf = CI->getCalledFunction();
          if(tf==NULL) 
            printf("!!The call instruction does not call any functions!\n");
          int num = CI->getNumArgOperands();	
          int i, f;
          Value *arg;
          for(i=0;i<num;i++)
          {
            arg = CI->getArgOperand(i);
            f = 0;
            if(Instruction *I = dyn_cast<Instruction>(arg))
            { 	
              bool is_reg = 1;
              char inst_id[256];
              getInstId(I, NULL, inst_id, f);
              if(f==1) 
                printf("!!the instruction does not have a name or an ID!\n");
              if(I->getType()->isVectorTy())
              {
                printf("oh, it's vector type!\n");
                print_line(itr, num_of_operands + 1 + i, inst_id, NULL, instid, 
                            arg->getType()->getTypeID(), 
                            getMemSize(arg->getType()), 
                            NULL,
                            is_reg);	
              }
              else
                print_line(itr, num_of_operands + 1 + i, inst_id, NULL, instid, 
                            arg->getType()->getTypeID(), 
                              getMemSize(arg->getType()),
                                arg, is_reg);	
            }
            else
            {
              bool is_reg = 0;
              if(arg->getType()->isLabelTy())
              {
                printf("oh, it's a label!\n");
                char label_id[256];
                getBBId(arg, label_id);
                print_line(itr, num_of_operands + 1 + i, label_id, NULL, NULL, 
                            curr_operand->getType()->getTypeID(),
                              getMemSize(curr_operand->getType()), 
                              NULL,
                              is_reg);
              } 
              // is function 
              else if(arg->getValueID()==2)
              { 
                printf("oh, it's a function!\n");
                char func_id[256];
                strcpy(func_id ,arg->getName().str().c_str());
                print_line(itr, num_of_operands + i + 1, func_id, NULL, NULL,
                                arg->getType()->getTypeID(),
                                getMemSize(arg->getType()), 
                                NULL,
                                is_reg);
              }
              else 
              {
                print_line(itr, num_of_operands + i + 1, NULL, NULL, NULL, 
                              arg->getType()->getTypeID(),
                              getMemSize(arg->getType()), 
                              arg, is_reg);
              }
            }	
          }
        }
        */

        //Get result info: resR = instID
        if(!itr->getType()->isVoidTy())
        {
          if(itr->getType()->isVectorTy())
          {
            bool is_reg = 1;
            print_line(nextitr, RESULT_LINE, -1, instid, NULL, NULL, 
                        itr->getType()->getTypeID(), 
                        getMemSize(itr->getType()), 
                        NULL, is_reg);
          }
          else if(itr->isTerminator())
            printf("It is terminator...\n");
          else
          {
            bool is_reg = 1;
            print_line(nextitr, RESULT_LINE, -1, instid, NULL, NULL, 
                        itr->getType()->getTypeID(), 
                          getMemSize(itr->getType()), 
                            itr, is_reg);
          }
        }
       } 
      return false;
    } //runBasicBlock
  }; //struct

}//namespace

char fullTrace::ID = 0;
static RegisterPass<fullTrace> X("fulltrace", "Add full Tracing Instrumentation for Aladdin", false, false);

