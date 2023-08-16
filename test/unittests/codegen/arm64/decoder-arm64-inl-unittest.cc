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

class DecoderArm64TestVisitor: public DecoderVisitor {
  public:
    DecoderArm64TestVisitor() {};

#define DECLARE(A) virtual void Visit##A(Instruction* instr) {		\
  	EXPECT_TRUE(false) << "Instruction incorrectly decoded";	\
    };
    VISITOR_LIST(DECLARE)
#undef DECLARE
};

#if defined(__CHERI_PURE_CAPABILITY__)
TEST_F(DecoderArm64Test, DecodeAddCapability) {
  class AddCapabilityVisitor : public DecoderArm64TestVisitor {
    public:
      AddCapabilityVisitor() {};

      void VisitMorelloAddSubCapability(Instruction* instr) override {
	EXPECT_EQ(instr->Mask(MorelloAddSubCapabilityFMask), MorelloAddSubCapabilityFixed);
	switch(instr->Mask(MorelloAddSubCapabilityMask)) {
	  case ADD_CAP:
	    EXPECT_TRUE(true);
	    EXPECT_EQ(instr->Mask(Cn_mask), Assembler::Cn(c1));
	    EXPECT_EQ(instr->Mask(Cd_mask), Assembler::Cd(c2));
	    EXPECT_EQ(instr->Mask(ImmAddSubCapability_mask), Assembler::ImmAddSubCapability(10));
	    break;
	  default:
  	    EXPECT_TRUE(false) << "Add/sub capability incorrectly decoded (" << std::hex << instr->Mask(MorelloAddSubCapabilityMask) << ")";
	    break;
	}
      } 
  };
  using TestOp = uint32_t;
  TestOp addCap = ADD_CAP | Assembler::ImmAddSubCapability(10) | Assembler::Cn(c1) | Assembler::Cd(c2); 
  auto decoder = new Decoder<DispatchingDecoderVisitor>();
  decoder->AppendVisitor(new AddCapabilityVisitor());
  auto addCapInstr = reinterpret_cast<Instruction*>(&addCap);
  decoder->Decode(addCapInstr);
}

TEST_F(DecoderArm64Test, DecodeSubCapability) {
  class SubCapabilityVisitor : public DecoderArm64TestVisitor {
    public:
      SubCapabilityVisitor() {};

      void VisitMorelloAddSubCapability(Instruction* instr) override {
	EXPECT_EQ(instr->Mask(MorelloAddSubCapabilityFMask), MorelloAddSubCapabilityFixed);
	switch(instr->Mask(MorelloAddSubCapabilityMask)) {
	  case SUB_CAP:
	    EXPECT_TRUE(true);
	    EXPECT_EQ(instr->Mask(Cn_mask), Assembler::Cn(c1));
	    EXPECT_EQ(instr->Mask(Cd_mask), Assembler::Cd(c2));
	    EXPECT_EQ(instr->Mask(ImmAddSubCapability_mask), Assembler::ImmAddSubCapability(10));
	    break;
	  default:
  	    EXPECT_TRUE(false) << "Add/sub capability incorrectly decoded (" << std::hex << instr->Mask(MorelloAddSubCapabilityMask) << ")";
	    break;
	}
      } 
  };
  using TestOp = uint32_t;
  TestOp subCap = SUB_CAP | Assembler::ImmAddSubCapability(10) | Assembler::Cn(c1) | Assembler::Cd(c2); 
  auto decoder = new Decoder<DispatchingDecoderVisitor>();
  decoder->AppendVisitor(new SubCapabilityVisitor());
  auto subCapInstr = reinterpret_cast<Instruction*>(&subCap);
  decoder->Decode(subCapInstr);
}
#endif

}  // namespace internal
}  // namespace v8

