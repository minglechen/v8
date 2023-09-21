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

      void VisitAddSubCapabilityImmediate(Instruction* instr) override {
	EXPECT_EQ(instr->Mask(AddSubCapabilityImmediateFMask), AddSubCapabilityImmediateFixed);
	switch(instr->Mask(AddSubCapabilityImmediateMask)) {
	  case ADDCAP_c_imm:
	    EXPECT_TRUE(true);
	    EXPECT_EQ(instr->Mask(Cn_mask), Assembler::Cn(c1));
	    EXPECT_EQ(instr->Mask(Cd_mask), Assembler::Cd(c2));
	    EXPECT_EQ(instr->Mask(ImmAddSubCapability_mask), Assembler::ImmAddSubCapability(10));
	    break;
	  default:
  	    EXPECT_TRUE(false) << "Add/sub capability incorrectly decoded (" << std::hex << instr->Mask(AddSubCapabilityImmediateMask) << ")";
	    break;
	}
      } 
  };
  using TestOp = uint32_t;
  TestOp addCap = ADDCAP_c_imm | Assembler::ImmAddSubCapability(10) | Assembler::Cn(c1) | Assembler::Cd(c2); 
  auto decoder = new Decoder<DispatchingDecoderVisitor>();
  decoder->AppendVisitor(new AddCapabilityVisitor());
  auto addCapInstr = reinterpret_cast<Instruction*>(&addCap);
  decoder->Decode(addCapInstr);
}

TEST_F(DecoderArm64Test, DecodeSubCapability) {
  class SubCapabilityVisitor : public DecoderArm64TestVisitor {
    public:
      SubCapabilityVisitor() {};

      void VisitAddSubCapabilityImmediate(Instruction* instr) override {
	EXPECT_EQ(instr->Mask(AddSubCapabilityImmediateFMask), AddSubCapabilityImmediateFixed);
	switch(instr->Mask(AddSubCapabilityImmediateMask)) {
	  case SUBCAP_c_imm:
	    EXPECT_TRUE(true);
	    EXPECT_EQ(instr->Mask(Cn_mask), Assembler::Cn(c1));
	    EXPECT_EQ(instr->Mask(Cd_mask), Assembler::Cd(c2));
	    EXPECT_EQ(instr->Mask(ImmAddSubCapability_mask), Assembler::ImmAddSubCapability(10));
	    break;
	  default:
  	    EXPECT_TRUE(false) << "Add/sub capability incorrectly decoded (" << std::hex << instr->Mask(AddSubCapabilityImmediateMask) << ")";
	    break;
	}
      } 
  };
  using TestOp = uint32_t;
  TestOp subCap = SUBCAP_c_imm | Assembler::ImmAddSubCapability(10) | Assembler::Cn(c1) | Assembler::Cd(c2); 
  auto decoder = new Decoder<DispatchingDecoderVisitor>();
  decoder->AppendVisitor(new SubCapabilityVisitor());
  auto subCapInstr = reinterpret_cast<Instruction*>(&subCap);
  decoder->Decode(subCapInstr);
}

TEST_F(DecoderArm64Test, DecodeCopyCapability) {
  class CopyCapabilityVisitor : public DecoderArm64TestVisitor {
    public:
      CopyCapabilityVisitor() {};

      void VisitCopyCapability(Instruction* instr) override {
	EXPECT_EQ(instr->Mask(CopyCapabilityFMask), CopyCapabilityFixed);
	switch(instr->Mask(CopyCapabilityMask)) {
	  case CPY:
	    EXPECT_TRUE(true);
	    EXPECT_EQ(instr->Mask(Cn_mask), Assembler::Cn(c1));
	    EXPECT_EQ(instr->Mask(Cd_mask), Assembler::Cd(c2));
	    break;
	  default:
  	    EXPECT_TRUE(false) << "Copy capability incorrectly decoded (" << std::hex << instr->Mask(CopyCapabilityMask) << ")";
	    break;
	}
      } 
  };
  using TestOp = uint32_t;
  TestOp copyCap = CPY |  Assembler::Cn(c1) | Assembler::Cd(c2); 
  auto decoder = new Decoder<DispatchingDecoderVisitor>();
  decoder->AppendVisitor(new CopyCapabilityVisitor());
  auto copyCapInstr = reinterpret_cast<Instruction*>(&copyCap);
  decoder->Decode(copyCapInstr);
}

TEST_F(DecoderArm64Test, DecodeLoadCapUnsignedOffsetCapNormal) {
  class LoadCapabilityVisitor : public DecoderArm64TestVisitor {
    public:
      LoadCapabilityVisitor() {};

      void VisitLoadStoreCapUnsignedOffsetCapNormal(Instruction* instr) override {
	EXPECT_EQ(instr->Mask(LoadStoreCapUnsignedOffsetCapNormalFMask), LoadStoreCapUnsignedOffsetCapNormalFixed);
	switch(instr->Mask(LoadStoreCapUnsignedOffsetCapNormalMask)) {
          case LDR_c_unsigned_cap_normal:
	    EXPECT_TRUE(true);
	    EXPECT_EQ(instr->Mask(Ct_mask), Assembler::Ct(c1));
	    EXPECT_EQ(instr->Mask(Cn_mask), Assembler::CnCSP(c2));
	    EXPECT_EQ(instr->Mask(ImmLS_mask), PatchingAssembler::ImmLSUnsigned(1 >> 4));
	    break;
	  default:
  	    EXPECT_TRUE(false) << "Load capability incorrectly decoded (" << std::hex << instr->Mask(LoadStoreCapUnsignedOffsetCapNormalMask) << ")";
	    break;
	}
      } 
  };
  using TestOp = uint32_t;
  constexpr unsigned size = 4;
  TestOp loadCap = LDR_c_unsigned_cap_normal |  Assembler::Ct(c1) | Assembler::CnCSP(c2) | PatchingAssembler::ImmLSUnsigned(1 >> size);
  auto decoder = new Decoder<DispatchingDecoderVisitor>();
  decoder->AppendVisitor(new LoadCapabilityVisitor());
  auto loadCapInstr = reinterpret_cast<Instruction*>(&loadCap);
  decoder->Decode(loadCapInstr);
}

TEST_F(DecoderArm64Test, DecodeStoreCapUnsignedOffsetCapNormal) {
  class StoreCapabilityVisitor : public DecoderArm64TestVisitor {
    public:
      StoreCapabilityVisitor() {};

      void VisitLoadStoreCapUnsignedOffsetCapNormal(Instruction* instr) override {
	EXPECT_EQ(instr->Mask(LoadStoreCapUnsignedOffsetCapNormalFMask), LoadStoreCapUnsignedOffsetCapNormalFixed);
	switch(instr->Mask(LoadStoreCapUnsignedOffsetCapNormalMask)) {
          case STR_c_unsigned_cap_normal:
	    EXPECT_TRUE(true);
	    EXPECT_EQ(instr->Mask(Ct_mask), Assembler::Ct(c1));
	    EXPECT_EQ(instr->Mask(Cn_mask), Assembler::CnCSP(c2));
	    EXPECT_EQ(instr->Mask(ImmLS_mask), PatchingAssembler::ImmLSUnsigned(1 >> 4));
	    break;
	  default:
  	    EXPECT_TRUE(false) << "Store capability incorrectly decoded (" << std::hex << instr->Mask(LoadStoreCapUnsignedOffsetCapNormalMask) << ")";
	    break;
	}
      } 
  };
  using TestOp = uint32_t;
  constexpr unsigned size = 4;
  TestOp storeCap = STR_c_unsigned_cap_normal |  Assembler::Ct(c1) | Assembler::CnCSP(c2) | PatchingAssembler::ImmLSUnsigned(1 >> size);
  auto decoder = new Decoder<DispatchingDecoderVisitor>();
  decoder->AppendVisitor(new StoreCapabilityVisitor());
  auto storeCapInstr = reinterpret_cast<Instruction*>(&storeCap);
  decoder->Decode(storeCapInstr);
}

TEST_F(DecoderArm64Test, DecodeStorePairCapPostIndex) {
  constexpr unsigned size = 4;
  class StorePairCapabilityVisitor : public DecoderArm64TestVisitor {
    public:
      StorePairCapabilityVisitor() {};

      void VisitLoadStorePairCapPostIndex(Instruction* instr) override {
	EXPECT_EQ(instr->Mask(LoadStorePairCapPostIndexFMask), LoadStorePairCapPostIndexFixed);
	switch(instr->Mask(LoadStorePairCapPostIndexMask)) {
          case STP_c_post:
	    EXPECT_TRUE(true);
	    EXPECT_EQ(instr->Mask(Ct_mask), Assembler::Ct(c1));
	    EXPECT_EQ(instr->Mask(Ct2_mask), Assembler::Ct2(c2));
	    EXPECT_EQ(instr->Mask(Rt_mask), Assembler::Rt(x1));
	    EXPECT_EQ(instr->Mask(ImmLSPair_mask), PatchingAssembler::ImmLSPair(16, size));
	    break;
	  default:
  	    EXPECT_TRUE(false) << "Store pair capability incorrectly decoded (" << std::hex << instr->Mask(LoadStorePairCapPostIndexMask) << ")";
	    break;
	}
      } 
  };
  using TestOp = uint32_t;
  TestOp storePairCap = STP_c_post | Assembler::Ct(c1) | Assembler::Ct2(c2) |
	                Assembler::Rt(x1) | Assembler::ImmLSPair(16, size);
  auto decoder = new Decoder<DispatchingDecoderVisitor>();
  decoder->AppendVisitor(new StorePairCapabilityVisitor());
  auto storePairCapInstr = reinterpret_cast<Instruction*>(&storePairCap);
  decoder->Decode(storePairCapInstr);
}

TEST_F(DecoderArm64Test, DecodeLoadPairCapPostIndex) {
  constexpr unsigned size = 4;
  class LoadPairCapabilityVisitor : public DecoderArm64TestVisitor {
    public:
      LoadPairCapabilityVisitor() {};

      void VisitLoadStorePairCapPostIndex(Instruction* instr) override {
	EXPECT_EQ(instr->Mask(LoadStorePairCapPostIndexFMask), LoadStorePairCapPostIndexFixed);
	switch(instr->Mask(LoadStorePairCapPostIndexMask)) {
          case LDP_c_post:
	    EXPECT_TRUE(true);
	    EXPECT_EQ(instr->Mask(Ct_mask), Assembler::Ct(c1));
	    EXPECT_EQ(instr->Mask(Ct2_mask), Assembler::Ct2(c2));
	    EXPECT_EQ(instr->Mask(Rt_mask), Assembler::Rt(x1));
	    EXPECT_EQ(instr->Mask(ImmLSPair_mask), PatchingAssembler::ImmLSPair(16, size));
	    break;
	  default:
  	    EXPECT_TRUE(false) << "Store pair capability incorrectly decoded (" << std::hex << instr->Mask(LoadStorePairCapPostIndexMask) << ")";
	    break;
	}
      } 
  };
  using TestOp = uint32_t;
  TestOp loadPairCap = LDP_c_post | Assembler::Ct(c1) | Assembler::Ct2(c2) |
	               Assembler::Rt(x1) | Assembler::ImmLSPair(16, size);
  auto decoder = new Decoder<DispatchingDecoderVisitor>();
  decoder->AppendVisitor(new LoadPairCapabilityVisitor());
  auto loadPairCapInstr = reinterpret_cast<Instruction*>(&loadPairCap);
  decoder->Decode(loadPairCapInstr);
}

TEST_F(DecoderArm64Test, DecodeStorePairCapPreIndex) {
  constexpr unsigned size = 4;
  class StorePairCapabilityVisitor : public DecoderArm64TestVisitor {
    public:
      StorePairCapabilityVisitor() {};

      void VisitLoadStorePairCapPreIndex(Instruction* instr) override {
	EXPECT_EQ(instr->Mask(LoadStorePairCapPreIndexFMask), LoadStorePairCapPreIndexFixed);
	switch(instr->Mask(LoadStorePairCapPreIndexMask)) {
          case STP_c_pre:
	    EXPECT_TRUE(true);
	    EXPECT_EQ(instr->Mask(Ct_mask), Assembler::Ct(c1));
	    EXPECT_EQ(instr->Mask(Ct2_mask), Assembler::Ct2(c2));
	    EXPECT_EQ(instr->Mask(Rt_mask), Assembler::Rt(x1));
	    EXPECT_EQ(instr->Mask(ImmLSPair_mask), PatchingAssembler::ImmLSPair(16, size));
	    break;
	  default:
  	    EXPECT_TRUE(false) << "Store pair capability incorrectly decoded (" << std::hex << instr->Mask(LoadStorePairCapPreIndexMask) << ")";
	    break;
	}
      } 
  };
  using TestOp = uint32_t;
  TestOp storePairCap = STP_c_pre | Assembler::Ct(c1) | Assembler::Ct2(c2) |
	                Assembler::Rt(x1) | Assembler::ImmLSPair(16, size);
  auto decoder = new Decoder<DispatchingDecoderVisitor>();
  decoder->AppendVisitor(new StorePairCapabilityVisitor());
  auto storePairCapInstr = reinterpret_cast<Instruction*>(&storePairCap);
  decoder->Decode(storePairCapInstr);
}

TEST_F(DecoderArm64Test, DecodeLoadPairCapPreIndex) {
  constexpr unsigned size = 4;
  class LoadPairCapabilityVisitor : public DecoderArm64TestVisitor {
    public:
      LoadPairCapabilityVisitor() {};

      void VisitLoadStorePairCapPreIndex(Instruction* instr) override {
	EXPECT_EQ(instr->Mask(LoadStorePairCapPreIndexFMask), LoadStorePairCapPreIndexFixed);
	switch(instr->Mask(LoadStorePairCapPreIndexMask)) {
          case LDP_c_pre:
	    EXPECT_TRUE(true);
	    EXPECT_EQ(instr->Mask(Ct_mask), Assembler::Ct(c1));
	    EXPECT_EQ(instr->Mask(Ct2_mask), Assembler::Ct2(c2));
	    EXPECT_EQ(instr->Mask(Rt_mask), Assembler::Rt(x1));
	    EXPECT_EQ(instr->Mask(ImmLSPair_mask), PatchingAssembler::ImmLSPair(16, size));
	    break;
	  default:
  	    EXPECT_TRUE(false) << "Store pair capability incorrectly decoded (" << std::hex << instr->Mask(LoadStorePairCapPreIndexMask) << ")";
	    break;
	}
      } 
  };
  using TestOp = uint32_t;
  TestOp loadPairCap = LDP_c_pre | Assembler::Ct(c1) | Assembler::Ct2(c2) |
	               Assembler::Rt(x1) | Assembler::ImmLSPair(16, size);
  auto decoder = new Decoder<DispatchingDecoderVisitor>();
  decoder->AppendVisitor(new LoadPairCapabilityVisitor());
  auto loadPairCapInstr = reinterpret_cast<Instruction*>(&loadPairCap);
  decoder->Decode(loadPairCapInstr);
}

TEST_F(DecoderArm64Test, DecodeStorePairCapOffset) {
  constexpr unsigned size = 4;
  class StorePairCapabilityVisitor : public DecoderArm64TestVisitor {
    public:
      StorePairCapabilityVisitor() {};

      void VisitLoadStorePairCapOffset(Instruction* instr) override {
	EXPECT_EQ(instr->Mask(LoadStorePairCapOffsetFMask), LoadStorePairCapOffsetFixed);
	switch(instr->Mask(LoadStorePairCapOffsetMask)) {
          case STP_c_off:
	    EXPECT_TRUE(true);
	    EXPECT_EQ(instr->Mask(Ct_mask), Assembler::Ct(c1));
	    EXPECT_EQ(instr->Mask(Ct2_mask), Assembler::Ct2(c2));
	    EXPECT_EQ(instr->Mask(Rt_mask), Assembler::Rt(x1));
	    EXPECT_EQ(instr->Mask(ImmLSPair_mask), PatchingAssembler::ImmLSPair(16, size));
	    break;
	  default:
  	    EXPECT_TRUE(false) << "Store pair capability incorrectly decoded (" << std::hex << instr->Mask(LoadStorePairCapOffsetMask) << ")";
	    break;
	}
      } 
  };
  using TestOp = uint32_t;
  TestOp storePairCap = STP_c_off | Assembler::Ct(c1) | Assembler::Ct2(c2) |
	                Assembler::Rt(x1) | Assembler::ImmLSPair(16, size);
  auto decoder = new Decoder<DispatchingDecoderVisitor>();
  decoder->AppendVisitor(new StorePairCapabilityVisitor());
  auto storePairCapInstr = reinterpret_cast<Instruction*>(&storePairCap);
  decoder->Decode(storePairCapInstr);
}

TEST_F(DecoderArm64Test, DecodeLoadPairCapOffset) {
  constexpr unsigned size = 4;
  class LoadPairCapabilityVisitor : public DecoderArm64TestVisitor {
    public:
      LoadPairCapabilityVisitor() {};

      void VisitLoadStorePairCapOffset(Instruction* instr) override {
	EXPECT_EQ(instr->Mask(LoadStorePairCapOffsetFMask), LoadStorePairCapOffsetFixed);
	switch(instr->Mask(LoadStorePairCapOffsetMask)) {
          case LDP_c_off:
	    EXPECT_TRUE(true);
	    EXPECT_EQ(instr->Mask(Ct_mask), Assembler::Ct(c1));
	    EXPECT_EQ(instr->Mask(Ct2_mask), Assembler::Ct2(c2));
	    EXPECT_EQ(instr->Mask(Rt_mask), Assembler::Rt(x1));
	    EXPECT_EQ(instr->Mask(ImmLSPair_mask), PatchingAssembler::ImmLSPair(16, size));
	    break;
	  default:
  	    EXPECT_TRUE(false) << "Store pair capability incorrectly decoded (" << std::hex << instr->Mask(LoadStorePairCapOffsetMask) << ")";
	    break;
	}
      } 
  };
  using TestOp = uint32_t;
  TestOp loadPairCap = LDP_c_off | Assembler::Ct(c1) | Assembler::Ct2(c2) |
	               Assembler::Rt(x1) | Assembler::ImmLSPair(16, size);
  auto decoder = new Decoder<DispatchingDecoderVisitor>();
  decoder->AppendVisitor(new LoadPairCapabilityVisitor());
  auto loadPairCapInstr = reinterpret_cast<Instruction*>(&loadPairCap);
  decoder->Decode(loadPairCapInstr);
}
#endif // __CHERI_PURE_CAPABILITY__

}  // namespace internal
}  // namespace v8

