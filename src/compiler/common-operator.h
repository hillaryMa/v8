// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_COMPILER_COMMON_OPERATOR_H_
#define V8_COMPILER_COMMON_OPERATOR_H_

#include "src/assembler.h"
#include "src/base/compiler-specific.h"
#include "src/compiler/frame-states.h"
#include "src/deoptimize-reason.h"
#include "src/globals.h"
#include "src/machine-type.h"
#include "src/zone/zone-containers.h"
#include "src/zone/zone-handle-set.h"

namespace v8 {
namespace internal {
namespace compiler {

// Forward declarations.
class CallDescriptor;
struct CommonOperatorGlobalCache;
class Operator;
class Type;
class Node;

// Prediction hint for branches.
enum class BranchHint : uint8_t { kNone, kTrue, kFalse };

inline BranchHint NegateBranchHint(BranchHint hint) {
  switch (hint) {
    case BranchHint::kNone:
      return hint;
    case BranchHint::kTrue:
      return BranchHint::kFalse;
    case BranchHint::kFalse:
      return BranchHint::kTrue;
  }
  UNREACHABLE();
}

inline size_t hash_value(BranchHint hint) { return static_cast<size_t>(hint); }

V8_EXPORT_PRIVATE std::ostream& operator<<(std::ostream&, BranchHint);

V8_EXPORT_PRIVATE BranchHint BranchHintOf(const Operator* const);

// Helper function for return nodes, because returns have a hidden value input.
int ValueInputCountOfReturn(Operator const* const op);

// Parameters for the {Deoptimize} operator.
class DeoptimizeParameters final {
 public:
  DeoptimizeParameters(DeoptimizeKind kind, DeoptimizeReason reason)
      : kind_(kind), reason_(reason) {}

  DeoptimizeKind kind() const { return kind_; }
  DeoptimizeReason reason() const { return reason_; }

 private:
  DeoptimizeKind const kind_;
  DeoptimizeReason const reason_;
};

bool operator==(DeoptimizeParameters, DeoptimizeParameters);
bool operator!=(DeoptimizeParameters, DeoptimizeParameters);

size_t hast_value(DeoptimizeParameters p);

std::ostream& operator<<(std::ostream&, DeoptimizeParameters p);

DeoptimizeParameters const& DeoptimizeParametersOf(Operator const* const);


class SelectParameters final {
 public:
  explicit SelectParameters(MachineRepresentation representation,
                            BranchHint hint = BranchHint::kNone)
      : representation_(representation), hint_(hint) {}

  MachineRepresentation representation() const { return representation_; }
  BranchHint hint() const { return hint_; }

 private:
  const MachineRepresentation representation_;
  const BranchHint hint_;
};

bool operator==(SelectParameters const&, SelectParameters const&);
bool operator!=(SelectParameters const&, SelectParameters const&);

size_t hash_value(SelectParameters const& p);

std::ostream& operator<<(std::ostream&, SelectParameters const& p);

V8_EXPORT_PRIVATE SelectParameters const& SelectParametersOf(
    const Operator* const);

V8_EXPORT_PRIVATE CallDescriptor const* CallDescriptorOf(const Operator* const);

V8_EXPORT_PRIVATE size_t ProjectionIndexOf(const Operator* const);

V8_EXPORT_PRIVATE MachineRepresentation
PhiRepresentationOf(const Operator* const);

// The {IrOpcode::kParameter} opcode represents an incoming parameter to the
// function. This class bundles the index and a debug name for such operators.
class ParameterInfo final {
 public:
  ParameterInfo(int index, const char* debug_name)
      : index_(index), debug_name_(debug_name) {}

  int index() const { return index_; }
  const char* debug_name() const { return debug_name_; }

 private:
  int index_;
  const char* debug_name_;
};

std::ostream& operator<<(std::ostream&, ParameterInfo const&);

V8_EXPORT_PRIVATE int ParameterIndexOf(const Operator* const);
const ParameterInfo& ParameterInfoOf(const Operator* const);

struct ObjectStateInfo final : std::pair<uint32_t, int> {
  using std::pair<uint32_t, int>::pair;
  uint32_t object_id() const { return first; }
  int size() const { return second; }
};
std::ostream& operator<<(std::ostream&, ObjectStateInfo const&);
size_t hash_value(ObjectStateInfo const& p);

struct TypedObjectStateInfo final
    : std::pair<uint32_t, const ZoneVector<MachineType>*> {
  using std::pair<uint32_t, const ZoneVector<MachineType>*>::pair;
  uint32_t object_id() const { return first; }
  const ZoneVector<MachineType>* machine_types() const { return second; }
};
std::ostream& operator<<(std::ostream&, TypedObjectStateInfo const&);
size_t hash_value(TypedObjectStateInfo const& p);

class RelocatablePtrConstantInfo final {
 public:
  enum Type { kInt32, kInt64 };

  RelocatablePtrConstantInfo(int32_t value, RelocInfo::Mode rmode)
      : value_(value), rmode_(rmode), type_(kInt32) {}
  RelocatablePtrConstantInfo(int64_t value, RelocInfo::Mode rmode)
      : value_(value), rmode_(rmode), type_(kInt64) {}

  intptr_t value() const { return value_; }
  RelocInfo::Mode rmode() const { return rmode_; }
  Type type() const { return type_; }

 private:
  intptr_t value_;
  RelocInfo::Mode rmode_;
  Type type_;
};

bool operator==(RelocatablePtrConstantInfo const& lhs,
                RelocatablePtrConstantInfo const& rhs);
bool operator!=(RelocatablePtrConstantInfo const& lhs,
                RelocatablePtrConstantInfo const& rhs);

std::ostream& operator<<(std::ostream&, RelocatablePtrConstantInfo const&);

size_t hash_value(RelocatablePtrConstantInfo const& p);

// Used to define a sparse set of inputs. This can be used to efficiently encode
// nodes that can have a lot of inputs, but where many inputs can have the same
// value.
class SparseInputMask final {
 public:
  typedef uint32_t BitMaskType;

  // The mask representing a dense input set.
  static const BitMaskType kDenseBitMask = 0x0;
  // The bits representing the end of a sparse input set.
  static const BitMaskType kEndMarker = 0x1;
  // The mask for accessing a sparse input entry in the bitmask.
  static const BitMaskType kEntryMask = 0x1;

  // The number of bits in the mask, minus one for the end marker.
  static const int kMaxSparseInputs = (sizeof(BitMaskType) * kBitsPerByte - 1);

  // An iterator over a node's sparse inputs.
  class InputIterator final {
   public:
    InputIterator() {}
    InputIterator(BitMaskType bit_mask, Node* parent);

    Node* parent() const { return parent_; }
    int real_index() const { return real_index_; }

    // Advance the iterator to the next sparse input. Only valid if the iterator
    // has not reached the end.
    void Advance();

    // Get the current sparse input's real node value. Only valid if the
    // current sparse input is real.
    Node* GetReal() const;

    // Get the current sparse input, returning either a real input node if
    // the current sparse input is real, or the given {empty_value} if the
    // current sparse input is empty.
    Node* Get(Node* empty_value) const {
      return IsReal() ? GetReal() : empty_value;
    }

    // True if the current sparse input is a real input node.
    bool IsReal() const;

    // True if the current sparse input is an empty value.
    bool IsEmpty() const { return !IsReal(); }

    // True if the iterator has reached the end of the sparse inputs.
    bool IsEnd() const;

   private:
    BitMaskType bit_mask_;
    Node* parent_;
    int real_index_;
  };

  explicit SparseInputMask(BitMaskType bit_mask) : bit_mask_(bit_mask) {}

  // Provides a SparseInputMask representing a dense input set.
  static SparseInputMask Dense() { return SparseInputMask(kDenseBitMask); }

  BitMaskType mask() const { return bit_mask_; }

  bool IsDense() const { return bit_mask_ == SparseInputMask::kDenseBitMask; }

  // Counts how many real values are in the sparse array. Only valid for
  // non-dense masks.
  int CountReal() const;

  // Returns an iterator over the sparse inputs of {node}.
  InputIterator IterateOverInputs(Node* node);

 private:
  //
  // The sparse input mask has a bitmask specifying if the node's inputs are
  // represented sparsely. If the bitmask value is 0, then the inputs are dense;
  // otherwise, they should be interpreted as follows:
  //
  //   * The bitmask represents which values are real, with 1 for real values
  //     and 0 for empty values.
  //   * The inputs to the node are the real values, in the order of the 1s from
  //     least- to most-significant.
  //   * The top bit of the bitmask is a guard indicating the end of the values,
  //     whether real or empty (and is not representative of a real input
  //     itself). This is used so that we don't have to additionally store a
  //     value count.
  //
  // So, for N 1s in the bitmask, there are N - 1 inputs into the node.
  BitMaskType bit_mask_;
};

bool operator==(SparseInputMask const& lhs, SparseInputMask const& rhs);
bool operator!=(SparseInputMask const& lhs, SparseInputMask const& rhs);

class TypedStateValueInfo final {
 public:
  TypedStateValueInfo(ZoneVector<MachineType> const* machine_types,
                      SparseInputMask sparse_input_mask)
      : machine_types_(machine_types), sparse_input_mask_(sparse_input_mask) {}

  ZoneVector<MachineType> const* machine_types() const {
    return machine_types_;
  }
  SparseInputMask sparse_input_mask() const { return sparse_input_mask_; }

 private:
  ZoneVector<MachineType> const* machine_types_;
  SparseInputMask sparse_input_mask_;
};

bool operator==(TypedStateValueInfo const& lhs, TypedStateValueInfo const& rhs);
bool operator!=(TypedStateValueInfo const& lhs, TypedStateValueInfo const& rhs);

std::ostream& operator<<(std::ostream&, TypedStateValueInfo const&);

size_t hash_value(TypedStateValueInfo const& p);

// Used to mark a region (as identified by BeginRegion/FinishRegion) as either
// JavaScript-observable or not (i.e. allocations are not JavaScript observable
// themselves, but transitioning stores are).
enum class RegionObservability : uint8_t { kObservable, kNotObservable };

size_t hash_value(RegionObservability);

std::ostream& operator<<(std::ostream&, RegionObservability);

RegionObservability RegionObservabilityOf(Operator const*) WARN_UNUSED_RESULT;

std::ostream& operator<<(std::ostream& os,
                         const ZoneVector<MachineType>* types);

ZoneHandleSet<Map> MapGuardMapsOf(Operator const*) WARN_UNUSED_RESULT;

Type* TypeGuardTypeOf(Operator const*) WARN_UNUSED_RESULT;

int OsrValueIndexOf(Operator const*);

SparseInputMask SparseInputMaskOf(Operator const*);

ZoneVector<MachineType> const* MachineTypesOf(Operator const*)
    WARN_UNUSED_RESULT;

// The ArgumentsElementsState and ArgumentsLengthState can describe the layout
// for backing stores of arguments objects of various types:
//
//                        +------------------------------------+
//  - kUnmappedArguments: | arg0, ... argK-1, argK, ... argN-1 |  {length:N}
//                        +------------------------------------+
//                        +------------------------------------+
//  - kMappedArguments:   | hole, ...   hole, argK, ... argN-1 |  {length:N}
//                        +------------------------------------+
//                                          +------------------+
//  - kRestParameter:                       | argK, ... argN-1 |  {length:N-K}
//                                          +------------------+
//
// Here {K} represents the number for formal parameters of the active function,
// whereas {N} represents the actual number of arguments passed at runtime.
// Note that {N < K} can happen and causes {K} to be capped accordingly.
//
// Also note that it is possible for an arguments object of {kMappedArguments}
// type to carry a backing store of {kUnappedArguments} type when {K == 0}.
typedef CreateArgumentsType ArgumentsStateType;

ArgumentsStateType ArgumentsStateTypeOf(Operator const*) WARN_UNUSED_RESULT;

uint32_t ObjectIdOf(Operator const*);

// Interface for building common operators that can be used at any level of IR,
// including JavaScript, mid-level, and low-level.
class V8_EXPORT_PRIVATE CommonOperatorBuilder final
    : public NON_EXPORTED_BASE(ZoneObject) {
 public:
  explicit CommonOperatorBuilder(Zone* zone);

  const Operator* Dead();
  const Operator* DeadValue();
  const Operator* Unreachable();
  const Operator* End(size_t control_input_count);
  const Operator* Branch(BranchHint = BranchHint::kNone);
  const Operator* IfTrue();
  const Operator* IfFalse();
  const Operator* IfSuccess();
  const Operator* IfException();
  const Operator* Switch(size_t control_output_count);
  const Operator* IfValue(int32_t value);
  const Operator* IfDefault();
  const Operator* Throw();
  const Operator* Deoptimize(DeoptimizeKind kind, DeoptimizeReason reason);
  const Operator* DeoptimizeIf(DeoptimizeKind kind, DeoptimizeReason reason);
  const Operator* DeoptimizeUnless(DeoptimizeKind kind,
                                   DeoptimizeReason reason);
  const Operator* TrapIf(int32_t trap_id);
  const Operator* TrapUnless(int32_t trap_id);
  const Operator* Return(int value_input_count = 1);
  const Operator* Terminate();

  const Operator* Start(int value_output_count);
  const Operator* Loop(int control_input_count);
  const Operator* Merge(int control_input_count);
  const Operator* Parameter(int index, const char* debug_name = nullptr);

  const Operator* OsrNormalEntry();
  const Operator* OsrLoopEntry();
  const Operator* OsrValue(int index);

  const Operator* Int32Constant(int32_t);
  const Operator* Int64Constant(int64_t);
  const Operator* Float32Constant(volatile float);
  const Operator* Float64Constant(volatile double);
  const Operator* ExternalConstant(const ExternalReference&);
  const Operator* NumberConstant(volatile double);
  const Operator* PointerConstant(intptr_t);
  const Operator* HeapConstant(const Handle<HeapObject>&);
  const Operator* ObjectId(uint32_t);

  const Operator* RelocatableInt32Constant(int32_t value,
                                           RelocInfo::Mode rmode);
  const Operator* RelocatableInt64Constant(int64_t value,
                                           RelocInfo::Mode rmode);

  const Operator* Select(MachineRepresentation, BranchHint = BranchHint::kNone);
  const Operator* Phi(MachineRepresentation representation,
                      int value_input_count);
  const Operator* EffectPhi(int effect_input_count);
  const Operator* InductionVariablePhi(int value_input_count);
  const Operator* LoopExit();
  const Operator* LoopExitValue();
  const Operator* LoopExitEffect();
  const Operator* Checkpoint();
  const Operator* BeginRegion(RegionObservability);
  const Operator* FinishRegion();
  const Operator* StateValues(int arguments, SparseInputMask bitmask);
  const Operator* TypedStateValues(const ZoneVector<MachineType>* types,
                                   SparseInputMask bitmask);
  const Operator* ArgumentsElementsState(ArgumentsStateType type);
  const Operator* ArgumentsLengthState(ArgumentsStateType type);
  const Operator* ObjectState(int object_id, int pointer_slots);
  const Operator* TypedObjectState(int object_id,
                                   const ZoneVector<MachineType>* types);
  const Operator* FrameState(BailoutId bailout_id,
                             OutputFrameStateCombine state_combine,
                             const FrameStateFunctionInfo* function_info);
  const Operator* Call(const CallDescriptor* descriptor);
  const Operator* CallWithCallerSavedRegisters(
      const CallDescriptor* descriptor);
  const Operator* TailCall(const CallDescriptor* descriptor);
  const Operator* Projection(size_t index);
  const Operator* Retain();
  const Operator* MapGuard(ZoneHandleSet<Map> maps);
  const Operator* TypeGuard(Type* type);

  // Constructs a new merge or phi operator with the same opcode as {op}, but
  // with {size} inputs.
  const Operator* ResizeMergeOrPhi(const Operator* op, int size);

  // Constructs function info for frame state construction.
  const FrameStateFunctionInfo* CreateFrameStateFunctionInfo(
      FrameStateType type, int parameter_count, int local_count,
      Handle<SharedFunctionInfo> shared_info);

 private:
  Zone* zone() const { return zone_; }

  const CommonOperatorGlobalCache& cache_;
  Zone* const zone_;

  DISALLOW_COPY_AND_ASSIGN(CommonOperatorBuilder);
};

}  // namespace compiler
}  // namespace internal
}  // namespace v8

#endif  // V8_COMPILER_COMMON_OPERATOR_H_
