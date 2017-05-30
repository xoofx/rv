#ifndef RV_TRANSFORM_IRPOLISHER_H
#define RV_TRANSFORM_IRPOLISHER_H

#include <unordered_map>
#include <functional>

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"

namespace rv {

// The IR polisher tries to modify the llvm IR so that the
// correct instruction patterns are selected. The main issue
// is that there is no machine type for <n x i1> on x86-64,
// so the IR polisher tries to replace these by vectors of
// i32/i64 instead. Note that this requires SSE41/AVX2 for
// the integer vector instructions.
class IRPolisher {
  llvm::Function &F;
  llvm::Type* boolVector;

  struct ExtInst {
    llvm::Instruction* inst;
    unsigned bitWidth;

    ExtInst(llvm::Instruction *inst, unsigned bitWidth)
      : inst(inst), bitWidth(bitWidth)
    {}

    struct Hash {
      size_t operator () (const ExtInst &i) const {
        std::hash<llvm::Instruction*> hash;
        return hash(i.inst) ^ i.bitWidth;
      }
    };

    struct Cmp {
      size_t operator () (const ExtInst &a, const ExtInst &b) const {
        return a.bitWidth == b.bitWidth && a.inst == b.inst;
      }
    };
  };

  std::unordered_map<ExtInst, llvm::Value*, ExtInst::Hash, ExtInst::Cmp> masks;

  bool isBooleanVector(const llvm::Type*);
  bool canReplaceInst(const llvm::Instruction*, unsigned&);

  llvm::Value *replaceCmpInst(llvm::IRBuilder<>&, llvm::CmpInst*, llvm::Value*, llvm::Value*);

  llvm::Value *getMaskForInst(llvm::Instruction*, unsigned);
  llvm::Value *getMaskForValue(llvm::IRBuilder<>&, llvm::Value*, unsigned);
  llvm::Value *getMaskForValueOrInst(llvm::IRBuilder<>&, llvm::Value*, unsigned);
  llvm::Value *getConditionFromMask(llvm::IRBuilder<>&, llvm::Value*);

public:
  IRPolisher(llvm::Function &f) : F(f) {}

  void polish();
};

}

#endif // RV_TRANSFORM_IRPOLISHER_H