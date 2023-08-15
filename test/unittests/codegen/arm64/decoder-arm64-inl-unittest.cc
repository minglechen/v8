#include "src/codegen/arm64/assembler-arm64-inl.h"
#include "src/codegen/arm64/constants-arm64.h"
#include "src/codegen/arm64/decoder-arm64.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace v8 {
namespace internal {

class DecoderArm64Test: public ::testing::Test {
 public:
  DecoderArm64Test() = default;
  ~DecoderArm64Test() override = default;
};

class DecoderArm64TestDecoder : public DecoderVisitor {
  public:
    DecoderArm64TestDecoder(Decoder<DispatchingDecoderVisitor>* decoder) : decoder_(decoder) {};

#define DECLARE(A) virtual void Visit##A(Instruction* instr) { EXPECT_TRUE(true); };
    VISITOR_LIST(DECLARE)
#undef DECLARE

    virtual void Decode(Instruction* instr) { decoder_->Decode(instr); }

  private:
    Decoder<DispatchingDecoderVisitor>* decoder_;
};

#if defined(__CHERI_PURE_CAPABILITY__)
TEST_F(DecoderArm64Test, DecodeAddSubCapability) {
  using TestOp = uint32_t;
  Register src = c1;
  Register dst = c2;
  TestOp addSubCap = ADD_CAP | Assembler::ImmAddSubCapability(10) | Assembler::Cn(c1) | Assembler::Cd(c2); 
  auto decoder = new DecoderArm64TestDecoder(new Decoder<DispatchingDecoderVisitor>());
  auto addSubCapInstr = reinterpret_cast<Instruction*>(&addSubCap);
  decoder->Decode(addSubCapInstr);
}
#endif

}  // namespace internal
}  // namespace v8

