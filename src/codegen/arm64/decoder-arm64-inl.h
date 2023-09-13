// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_CODEGEN_ARM64_DECODER_ARM64_INL_H_
#define V8_CODEGEN_ARM64_DECODER_ARM64_INL_H_

#include "src/codegen/arm64/decoder-arm64.h"
#include "src/common/globals.h"
#include "src/utils/utils.h"

namespace v8 {
namespace internal {

// Top-level instruction decode function.
template <typename V>
void Decoder<V>::Decode(Instruction* instr) {
  // Top-level encodings for A64 [31-29][28-24 op0][23-0]
  if (instr->Bits(28, 27) == 0) {
    switch (instr->Bits(27, 24)) {
#if defined(__CHERI_PURE_CAPABILITY__)
      // op0: 00010 Morello encodings
      case 0x2:
	DecodeMorello(instr);
	break;
#endif
      // op0: 0000x Reserved
      // op0: 00011 UNALLOCATED
      // op0: 001xx UNALLOCATED
      default:
	V::VisitUnallocated(instr);
	break;
    }
  } else {
    switch (instr->Bits(27, 24)) {
      // 0:   PC relative addressing.
      case 0x0:
        DecodePCRelAddressing(instr);
        break;

      // 1:   Add/sub immediate.
      case 0x1:
        DecodeAddSubImmediate(instr);
        break;

      // A:   Logical shifted register.
      //      Add/sub with carry.
      //      Conditional compare register.
      //      Conditional compare immediate.
      //      Conditional select.
      //      Data processing 1 source.
      //      Data processing 2 source.
      // B:   Add/sub shifted register.
      //      Add/sub extended register.
      //      Data processing 3 source.
      case 0xA:
      case 0xB:
        DecodeDataProcessing(instr);
        break;

      // 2:   Logical immediate.
      //      Move wide immediate.
      case 0x2:
        DecodeLogical(instr);
        break;

      // 3:   Bitfield.
      //      Extract.
      case 0x3:
        DecodeBitfieldExtract(instr);
        break;

      // 4:   Unconditional branch immediate.
      //      Exception generation.
      //      Compare and branch immediate.
      // 5:   Compare and branch immediate.
      //      Conditional branch.
      //      System.
      // 6,7: Unconditional branch.
      //      Test and branch immediate.
      case 0x4:
      case 0x5:
      case 0x6:
      case 0x7:
        DecodeBranchSystemException(instr);
        break;

      // 8,9: Load/store register pair post-index.
      //      Load register literal.
      //      Load/store register unscaled immediate.
      //      Load/store register immediate post-index.
      //      Load/store register immediate pre-index.
      //      Load/store register offset.
      // C,D: Load/store register pair offset.
      //      Load/store register pair pre-index.
      //      Load/store register unsigned immediate.
      //      Advanced SIMD.
      case 0x8:
      case 0x9:
      case 0xC:
      case 0xD:
        DecodeLoadStore(instr);
        break;

      // E:   FP fixed point conversion.
      //      FP integer conversion.
      //      FP data processing 1 source.
      //      FP compare.
      //      FP immediate.
      //      FP data processing 2 source.
      //      FP conditional compare.
      //      FP conditional select.
      //      Advanced SIMD.
      // F:   FP data processing 3 source.
      //      Advanced SIMD.
      case 0xE:
      case 0xF:
        DecodeFP(instr);
        break;
    }
  }
}

template <typename V>
void Decoder<V>::DecodePCRelAddressing(Instruction* instr) {
  DCHECK_EQ(0x0, instr->Bits(27, 24));
  // We know bit 28 is set, as <b28:b27> = 0 is filtered out at the top level
  // decode.
  DCHECK_EQ(0x1, instr->Bit(28));
  V::VisitPCRelAddressing(instr);
}

template <typename V>
void Decoder<V>::DecodeBranchSystemException(Instruction* instr) {
  DCHECK_EQ(0x4, instr->Bits(27, 24) & 0xC);  // 0x4, 0x5, 0x6, 0x7

  switch (instr->Bits(31, 29)) {
    case 0:
    case 4: {
      V::VisitUnconditionalBranch(instr);
      break;
    }
    case 1:
    case 5: {
      if (instr->Bit(25) == 0) {
        V::VisitCompareBranch(instr);
      } else {
        V::VisitTestBranch(instr);
      }
      break;
    }
    case 2: {
      if (instr->Bit(25) == 0) {
        if ((instr->Bit(24) == 0x1) ||
            (instr->Mask(0x01000010) == 0x00000010)) {
          V::VisitUnallocated(instr);
        } else {
          V::VisitConditionalBranch(instr);
        }
      } else {
        V::VisitUnallocated(instr);
      }
      break;
    }
    case 6: {
      if (instr->Bit(25) == 0) {
        if (instr->Bit(24) == 0) {
          if ((instr->Bits(4, 2) != 0) ||
              (instr->Mask(0x00E0001D) == 0x00200001) ||
              (instr->Mask(0x00E0001D) == 0x00400001) ||
              (instr->Mask(0x00E0001E) == 0x00200002) ||
              (instr->Mask(0x00E0001E) == 0x00400002) ||
              (instr->Mask(0x00E0001C) == 0x00600000) ||
              (instr->Mask(0x00E0001C) == 0x00800000) ||
              (instr->Mask(0x00E0001F) == 0x00A00000) ||
              (instr->Mask(0x00C0001C) == 0x00C00000)) {
            V::VisitUnallocated(instr);
          } else {
            V::VisitException(instr);
          }
        } else {
          if (instr->Bits(23, 22) == 0) {
            const Instr masked_003FF0E0 = instr->Mask(0x003FF0E0);
            if ((instr->Bits(21, 19) == 0x4) ||
                (masked_003FF0E0 == 0x00033000) ||
                (masked_003FF0E0 == 0x003FF020) ||
                (masked_003FF0E0 == 0x003FF060) ||
                (masked_003FF0E0 == 0x003FF0E0) ||
                (instr->Mask(0x00388000) == 0x00008000) ||
                (instr->Mask(0x0038E000) == 0x00000000) ||
                (instr->Mask(0x0039E000) == 0x00002000) ||
                (instr->Mask(0x003AE000) == 0x00002000) ||
                (instr->Mask(0x003CE000) == 0x00042000) ||
                (instr->Mask(0x0038F000) == 0x00005000) ||
                (instr->Mask(0x0038E000) == 0x00006000)) {
              V::VisitUnallocated(instr);
            } else {
              V::VisitSystem(instr);
            }
          } else {
            V::VisitUnallocated(instr);
          }
        }
      } else {
        if ((instr->Bit(24) == 0x1) || (instr->Bits(20, 16) != 0x1F) ||
            (instr->Bits(15, 10) != 0) || (instr->Bits(4, 0) != 0) ||
            (instr->Bits(24, 21) == 0x3) || (instr->Bits(24, 22) == 0x3)) {
          V::VisitUnallocated(instr);
        } else {
          V::VisitUnconditionalBranchToRegister(instr);
        }
      }
      break;
    }
    case 3:
    case 7: {
      V::VisitUnallocated(instr);
      break;
    }
  }
}

template <typename V>
void Decoder<V>::DecodeLoadStore(Instruction* instr) {
  DCHECK_EQ(0x8, instr->Bits(27, 24) & 0xA);  // 0x8, 0x9, 0xC, 0xD

  if ((instr->Bit(28) == 0) && (instr->Bit(29) == 0) && (instr->Bit(26) == 1)) {
    DecodeNEONLoadStore(instr);
    return;
  }

  if (instr->Bit(24) == 0) {
    if (instr->Bit(28) == 0) {
      if (instr->Bit(29) == 0) {
        if (instr->Bit(26) == 0) {
          if (instr->Mask(0xA08000) == 0x800000 ||
              instr->Mask(0xA00000) == 0xA00000) {
            V::VisitUnallocated(instr);
          } else if (instr->Mask(0x808000) == 0) {
            // Load/Store exclusive without acquire/release are unimplemented.
            V::VisitUnimplemented(instr);
          } else {
            V::VisitLoadStoreAcquireRelease(instr);
          }
        }
      } else {
        if ((instr->Bits(31, 30) == 0x3) ||
            (instr->Mask(0xC4400000) == 0x40000000)) {
          V::VisitUnallocated(instr);
        } else {
          if (instr->Bit(23) == 0) {
            if (instr->Mask(0xC4400000) == 0xC0400000) {
              V::VisitUnallocated(instr);
            } else {
              // Nontemporals are unimplemented.
              V::VisitUnimplemented(instr);
            }
          } else {
            V::VisitLoadStorePairPostIndex(instr);
          }
        }
      }
    } else {
      if (instr->Bit(29) == 0) {
        if (instr->Mask(0xC4000000) == 0xC4000000) {
          V::VisitUnallocated(instr);
        } else {
          V::VisitLoadLiteral(instr);
        }
      } else {
        if ((instr->Mask(0x84C00000) == 0x80C00000) ||
            (instr->Mask(0x44800000) == 0x44800000) ||
            (instr->Mask(0x84800000) == 0x84800000)) {
          V::VisitUnallocated(instr);
        } else {
          if (instr->Bit(21) == 0) {
            switch (instr->Bits(11, 10)) {
              case 0: {
                V::VisitLoadStoreUnscaledOffset(instr);
                break;
              }
              case 1: {
                if (instr->Mask(0xC4C00000) == 0xC0800000) {
                  V::VisitUnallocated(instr);
                } else {
                  V::VisitLoadStorePostIndex(instr);
                }
                break;
              }
              case 2: {
                // TODO(all): VisitLoadStoreRegisterOffsetUnpriv.
                V::VisitUnimplemented(instr);
                break;
              }
              case 3: {
                if (instr->Mask(0xC4C00000) == 0xC0800000) {
                  V::VisitUnallocated(instr);
                } else {
                  V::VisitLoadStorePreIndex(instr);
                }
                break;
              }
            }
          } else {
            if (instr->Bits(11, 10) == 0x2) {
              if (instr->Bit(14) == 0) {
                V::VisitUnallocated(instr);
              } else {
                V::VisitLoadStoreRegisterOffset(instr);
              }
            } else {
              V::VisitUnallocated(instr);
            }
          }
        }
      }
    }
  } else {
    if (instr->Bit(28) == 0) {
      if (instr->Bit(29) == 0) {
        V::VisitUnallocated(instr);
      } else {
        if ((instr->Bits(31, 30) == 0x3) ||
            (instr->Mask(0xC4400000) == 0x40000000)) {
          V::VisitUnallocated(instr);
        } else {
          if (instr->Bit(23) == 0) {
            V::VisitLoadStorePairOffset(instr);
          } else {
            V::VisitLoadStorePairPreIndex(instr);
          }
        }
      }
    } else {
      if (instr->Bit(29) == 0) {
        V::VisitUnallocated(instr);
      } else {
        if ((instr->Mask(0x84C00000) == 0x80C00000) ||
            (instr->Mask(0x44800000) == 0x44800000) ||
            (instr->Mask(0x84800000) == 0x84800000)) {
          V::VisitUnallocated(instr);
        } else {
          V::VisitLoadStoreUnsignedOffset(instr);
        }
      }
    }
  }
}

template <typename V>
void Decoder<V>::DecodeLogical(Instruction* instr) {
  DCHECK_EQ(0x2, instr->Bits(27, 24));

  if (instr->Mask(0x80400000) == 0x00400000) {
    V::VisitUnallocated(instr);
  } else {
    if (instr->Bit(23) == 0) {
      V::VisitLogicalImmediate(instr);
    } else {
      if (instr->Bits(30, 29) == 0x1) {
        V::VisitUnallocated(instr);
      } else {
        V::VisitMoveWideImmediate(instr);
      }
    }
  }
}

template <typename V>
void Decoder<V>::DecodeBitfieldExtract(Instruction* instr) {
  DCHECK_EQ(0x3, instr->Bits(27, 24));

  if ((instr->Mask(0x80400000) == 0x80000000) ||
      (instr->Mask(0x80400000) == 0x00400000) ||
      (instr->Mask(0x80008000) == 0x00008000)) {
    V::VisitUnallocated(instr);
  } else if (instr->Bit(23) == 0) {
    if ((instr->Mask(0x80200000) == 0x00200000) ||
        (instr->Mask(0x60000000) == 0x60000000)) {
      V::VisitUnallocated(instr);
    } else {
      V::VisitBitfield(instr);
    }
  } else {
    if ((instr->Mask(0x60200000) == 0x00200000) ||
        (instr->Mask(0x60000000) != 0x00000000)) {
      V::VisitUnallocated(instr);
    } else {
      V::VisitExtract(instr);
    }
  }
}

template <typename V>
void Decoder<V>::DecodeAddSubImmediate(Instruction* instr) {
  DCHECK_EQ(0x1, instr->Bits(27, 24));
  if (instr->Bit(23) == 1) {
    V::VisitUnallocated(instr);
  } else {
    V::VisitAddSubImmediate(instr);
  }
}

template <typename V>
void Decoder<V>::DecodeDataProcessing(Instruction* instr) {
  DCHECK((instr->Bits(27, 24) == 0xA) || (instr->Bits(27, 24) == 0xB));

  if (instr->Bit(24) == 0) {
    if (instr->Bit(28) == 0) {
      if (instr->Mask(0x80008000) == 0x00008000) {
        V::VisitUnallocated(instr);
      } else {
        V::VisitLogicalShifted(instr);
      }
    } else {
      switch (instr->Bits(23, 21)) {
        case 0: {
          if (instr->Mask(0x0000FC00) != 0) {
            V::VisitUnallocated(instr);
          } else {
            V::VisitAddSubWithCarry(instr);
          }
          break;
        }
        case 2: {
          if ((instr->Bit(29) == 0) || (instr->Mask(0x00000410) != 0)) {
            V::VisitUnallocated(instr);
          } else {
            if (instr->Bit(11) == 0) {
              V::VisitConditionalCompareRegister(instr);
            } else {
              V::VisitConditionalCompareImmediate(instr);
            }
          }
          break;
        }
        case 4: {
          if (instr->Mask(0x20000800) != 0x00000000) {
            V::VisitUnallocated(instr);
          } else {
            V::VisitConditionalSelect(instr);
          }
          break;
        }
        case 6: {
          if (instr->Bit(29) == 0x1) {
            V::VisitUnallocated(instr);
          } else {
            if (instr->Bit(30) == 0) {
              if ((instr->Bit(15) == 0x1) || (instr->Bits(15, 11) == 0) ||
                  (instr->Bits(15, 12) == 0x1) ||
                  (instr->Bits(15, 12) == 0x3) ||
                  (instr->Bits(15, 13) == 0x3) ||
                  (instr->Mask(0x8000EC00) == 0x00004C00) ||
                  (instr->Mask(0x8000E800) == 0x80004000) ||
                  (instr->Mask(0x8000E400) == 0x80004000)) {
                V::VisitUnallocated(instr);
              } else {
                V::VisitDataProcessing2Source(instr);
              }
            } else {
              if ((instr->Bit(13) == 1) || (instr->Bits(20, 16) != 0) ||
                  (instr->Bits(15, 14) != 0) ||
                  (instr->Mask(0xA01FFC00) == 0x00000C00) ||
                  (instr->Mask(0x201FF800) == 0x00001800)) {
                V::VisitUnallocated(instr);
              } else {
                V::VisitDataProcessing1Source(instr);
              }
            }
            break;
          }
          V8_FALLTHROUGH;
        }
        case 1:
        case 3:
        case 5:
        case 7:
          V::VisitUnallocated(instr);
          break;
      }
    }
  } else {
    if (instr->Bit(28) == 0) {
      if (instr->Bit(21) == 0) {
        if ((instr->Bits(23, 22) == 0x3) ||
            (instr->Mask(0x80008000) == 0x00008000)) {
          V::VisitUnallocated(instr);
        } else {
          V::VisitAddSubShifted(instr);
        }
      } else {
        if ((instr->Mask(0x00C00000) != 0x00000000) ||
            (instr->Mask(0x00001400) == 0x00001400) ||
            (instr->Mask(0x00001800) == 0x00001800)) {
          V::VisitUnallocated(instr);
        } else {
          V::VisitAddSubExtended(instr);
        }
      }
    } else {
      if ((instr->Bit(30) == 0x1) || (instr->Bits(30, 29) == 0x1) ||
          (instr->Mask(0xE0600000) == 0x00200000) ||
          (instr->Mask(0xE0608000) == 0x00400000) ||
          (instr->Mask(0x60608000) == 0x00408000) ||
          (instr->Mask(0x60E00000) == 0x00E00000) ||
          (instr->Mask(0x60E00000) == 0x00800000) ||
          (instr->Mask(0x60E00000) == 0x00600000)) {
        V::VisitUnallocated(instr);
      } else {
        V::VisitDataProcessing3Source(instr);
      }
    }
  }
}

template <typename V>
void Decoder<V>::DecodeFP(Instruction* instr) {
  DCHECK((instr->Bits(27, 24) == 0xE) || (instr->Bits(27, 24) == 0xF));

  if (instr->Bit(28) == 0) {
    DecodeNEONVectorDataProcessing(instr);
  } else {
    if (instr->Bits(31, 30) == 0x3) {
      V::VisitUnallocated(instr);
    } else if (instr->Bits(31, 30) == 0x1) {
      DecodeNEONScalarDataProcessing(instr);
    } else {
      if (instr->Bit(29) == 0) {
        if (instr->Bit(24) == 0) {
          if (instr->Bit(21) == 0) {
            if ((instr->Bit(23) == 1) || (instr->Bit(18) == 1) ||
                (instr->Mask(0x80008000) == 0x00000000) ||
                (instr->Mask(0x000E0000) == 0x00000000) ||
                (instr->Mask(0x000E0000) == 0x000A0000) ||
                (instr->Mask(0x00160000) == 0x00000000) ||
                (instr->Mask(0x00160000) == 0x00120000)) {
              V::VisitUnallocated(instr);
            } else {
              V::VisitFPFixedPointConvert(instr);
            }
          } else {
            if (instr->Bits(15, 10) == 32) {
              V::VisitUnallocated(instr);
            } else if (instr->Bits(15, 10) == 0) {
              if ((instr->Bits(23, 22) == 0x3) ||
                  (instr->Mask(0x000E0000) == 0x000A0000) ||
                  (instr->Mask(0x000E0000) == 0x000C0000) ||
                  (instr->Mask(0x00160000) == 0x00120000) ||
                  (instr->Mask(0x00160000) == 0x00140000) ||
                  (instr->Mask(0x20C40000) == 0x00800000) ||
                  (instr->Mask(0x20C60000) == 0x00840000) ||
                  (instr->Mask(0xA0C60000) == 0x80060000) ||
                  (instr->Mask(0xA0C60000) == 0x00860000) ||
                  (instr->Mask(0xA0CE0000) == 0x80860000) ||
                  (instr->Mask(0xA0CE0000) == 0x804E0000) ||
                  (instr->Mask(0xA0CE0000) == 0x000E0000) ||
                  (instr->Mask(0xA0D60000) == 0x00160000) ||
                  (instr->Mask(0xA0D60000) == 0x80560000) ||
                  (instr->Mask(0xA0D60000) == 0x80960000)) {
                V::VisitUnallocated(instr);
              } else {
                V::VisitFPIntegerConvert(instr);
              }
            } else if (instr->Bits(14, 10) == 16) {
              const Instr masked_A0DF8000 = instr->Mask(0xA0DF8000);
              if ((instr->Mask(0x80180000) != 0) ||
                  (masked_A0DF8000 == 0x00020000) ||
                  (masked_A0DF8000 == 0x00030000) ||
                  (masked_A0DF8000 == 0x00068000) ||
                  (masked_A0DF8000 == 0x00428000) ||
                  (masked_A0DF8000 == 0x00430000) ||
                  (masked_A0DF8000 == 0x00468000) ||
                  (instr->Mask(0xA0D80000) == 0x00800000) ||
                  (instr->Mask(0xA0DE0000) == 0x00C00000) ||
                  (instr->Mask(0xA0DF0000) == 0x00C30000) ||
                  (instr->Mask(0xA0DC0000) == 0x00C40000)) {
                V::VisitUnallocated(instr);
              } else {
                V::VisitFPDataProcessing1Source(instr);
              }
            } else if (instr->Bits(13, 10) == 8) {
              if ((instr->Bits(15, 14) != 0) || (instr->Bits(2, 0) != 0) ||
                  (instr->Mask(0x80800000) != 0x00000000)) {
                V::VisitUnallocated(instr);
              } else {
                V::VisitFPCompare(instr);
              }
            } else if (instr->Bits(12, 10) == 4) {
              if ((instr->Bits(9, 5) != 0) ||
                  (instr->Mask(0x80800000) != 0x00000000)) {
                V::VisitUnallocated(instr);
              } else {
                V::VisitFPImmediate(instr);
              }
            } else {
              if (instr->Mask(0x80800000) != 0x00000000) {
                V::VisitUnallocated(instr);
              } else {
                switch (instr->Bits(11, 10)) {
                  case 1: {
                    V::VisitFPConditionalCompare(instr);
                    break;
                  }
                  case 2: {
                    if ((instr->Bits(15, 14) == 0x3) ||
                        (instr->Mask(0x00009000) == 0x00009000) ||
                        (instr->Mask(0x0000A000) == 0x0000A000)) {
                      V::VisitUnallocated(instr);
                    } else {
                      V::VisitFPDataProcessing2Source(instr);
                    }
                    break;
                  }
                  case 3: {
                    V::VisitFPConditionalSelect(instr);
                    break;
                  }
                  default:
                    UNREACHABLE();
                }
              }
            }
          }
        } else {
          // Bit 30 == 1 has been handled earlier.
          DCHECK_EQ(0, instr->Bit(30));
          if (instr->Mask(0xA0800000) != 0) {
            V::VisitUnallocated(instr);
          } else {
            V::VisitFPDataProcessing3Source(instr);
          }
        }
      } else {
        V::VisitUnallocated(instr);
      }
    }
  }
}

template <typename V>
void Decoder<V>::DecodeNEONLoadStore(Instruction* instr) {
  DCHECK_EQ(0x6, instr->Bits(29, 25));
  if (instr->Bit(31) == 0) {
    if ((instr->Bit(24) == 0) && (instr->Bit(21) == 1)) {
      V::VisitUnallocated(instr);
      return;
    }

    if (instr->Bit(23) == 0) {
      if (instr->Bits(20, 16) == 0) {
        if (instr->Bit(24) == 0) {
          V::VisitNEONLoadStoreMultiStruct(instr);
        } else {
          V::VisitNEONLoadStoreSingleStruct(instr);
        }
      } else {
        V::VisitUnallocated(instr);
      }
    } else {
      if (instr->Bit(24) == 0) {
        V::VisitNEONLoadStoreMultiStructPostIndex(instr);
      } else {
        V::VisitNEONLoadStoreSingleStructPostIndex(instr);
      }
    }
  } else {
    V::VisitUnallocated(instr);
  }
}

template <typename V>
void Decoder<V>::DecodeNEONVectorDataProcessing(Instruction* instr) {
  DCHECK_EQ(0x7, instr->Bits(28, 25));
  if (instr->Bit(31) == 0) {
    if (instr->Bit(24) == 0) {
      if (instr->Bit(21) == 0) {
        if (instr->Bit(15) == 0) {
          if (instr->Bit(10) == 0) {
            if (instr->Bit(29) == 0) {
              if (instr->Bit(11) == 0) {
                V::VisitNEONTable(instr);
              } else {
                V::VisitNEONPerm(instr);
              }
            } else {
              V::VisitNEONExtract(instr);
            }
          } else {
            if (instr->Bits(23, 22) == 0) {
              V::VisitNEONCopy(instr);
            } else {
              V::VisitUnallocated(instr);
            }
          }
        } else {
          V::VisitUnallocated(instr);
        }
      } else {
        if (instr->Bit(10) == 0) {
          if (instr->Bit(11) == 0) {
            V::VisitNEON3Different(instr);
          } else {
            if (instr->Bits(18, 17) == 0) {
              if (instr->Bit(20) == 0) {
                if (instr->Bit(19) == 0) {
                  V::VisitNEON2RegMisc(instr);
                } else {
                  if (instr->Bits(30, 29) == 0x2) {
                    V::VisitUnallocated(instr);
                  } else {
                    V::VisitUnallocated(instr);
                  }
                }
              } else {
                if (instr->Bit(19) == 0) {
                  V::VisitNEONAcrossLanes(instr);
                } else {
                  V::VisitUnallocated(instr);
                }
              }
            } else {
              V::VisitUnallocated(instr);
            }
          }
        } else {
          V::VisitNEON3Same(instr);
        }
      }
    } else {
      if (instr->Bit(10) == 0) {
        V::VisitNEONByIndexedElement(instr);
      } else {
        if (instr->Bit(23) == 0) {
          if (instr->Bits(22, 19) == 0) {
            V::VisitNEONModifiedImmediate(instr);
          } else {
            V::VisitNEONShiftImmediate(instr);
          }
        } else {
          V::VisitUnallocated(instr);
        }
      }
    }
  } else {
    V::VisitUnallocated(instr);
  }
}

template <typename V>
void Decoder<V>::DecodeNEONScalarDataProcessing(Instruction* instr) {
  DCHECK_EQ(0xF, instr->Bits(28, 25));
  if (instr->Bit(24) == 0) {
    if (instr->Bit(21) == 0) {
      if (instr->Bit(15) == 0) {
        if (instr->Bit(10) == 0) {
          if (instr->Bit(29) == 0) {
            if (instr->Bit(11) == 0) {
              V::VisitUnallocated(instr);
            } else {
              V::VisitUnallocated(instr);
            }
          } else {
            V::VisitUnallocated(instr);
          }
        } else {
          if (instr->Bits(23, 22) == 0) {
            V::VisitNEONScalarCopy(instr);
          } else {
            V::VisitUnallocated(instr);
          }
        }
      } else {
        V::VisitUnallocated(instr);
      }
    } else {
      if (instr->Bit(10) == 0) {
        if (instr->Bit(11) == 0) {
          V::VisitNEONScalar3Diff(instr);
        } else {
          if (instr->Bits(18, 17) == 0) {
            if (instr->Bit(20) == 0) {
              if (instr->Bit(19) == 0) {
                V::VisitNEONScalar2RegMisc(instr);
              } else {
                if (instr->Bit(29) == 0) {
                  V::VisitUnallocated(instr);
                } else {
                  V::VisitUnallocated(instr);
                }
              }
            } else {
              if (instr->Bit(19) == 0) {
                V::VisitNEONScalarPairwise(instr);
              } else {
                V::VisitUnallocated(instr);
              }
            }
          } else {
            V::VisitUnallocated(instr);
          }
        }
      } else {
        V::VisitNEONScalar3Same(instr);
      }
    }
  } else {
    if (instr->Bit(10) == 0) {
      V::VisitNEONScalarByIndexedElement(instr);
    } else {
      if (instr->Bit(23) == 0) {
        V::VisitNEONScalarShiftImmediate(instr);
      } else {
        V::VisitUnallocated(instr);
      }
    }
  }
}

#if defined(__CHERI_PURE_CAPABILITY__)
template <typename V>
void Decoder<V>::DecodeMorello(Instruction* instr) {
  DCHECK_EQ(0x2, instr->Bits(28, 24));
  // Morello encodings: [31-29 op0][0 0010][23-21 op1][20-16][15 op2][14-13][12-10 op3][9-0]
  switch (instr->Bits(31, 29)) {
    case 0x0:
      // op0: 000 Morello add/subtract capability
      DecodeMorelloAddSubCapability(instr);
      break;
    case 0x1:
       // op0: 001 Morello load store misc1
       DecodeMorelloLoadStoreMisc1(instr);
      break;
    case 0x2:
      // op0: 010 Morello load store misc2
       DecodeMorelloLoadStoreMisc2(instr);
      break;
    case 0x3:
      // op0: 011 Morello load store misc3
       DecodeMorelloLoadStoreMisc3(instr);
      break;
    case 0x4:
        if (instr->Bit(23) == 0) {
          if (instr->Bit(23) == 0) {
	    // op0: 100 op1: 00x: LDR (literal)
	    DecodeMorelloLdrLiteral(instr);
	  } else {
	    // op0: 100 op1: 01x: Morello load/store unsigned offset via alternaitve base
	    DecodeMorelloLoadStoreUnsignedOffsetViaAlternativeBase(instr);
	  }
	} else  {
	  // op0: 100 op1: 1xx: Morello load/store register via alternative base
	  DecodeMorelloLoadStoreRegisterViaAlternativeBase(instr);
	}
      break;
    case 0x5:
      // op0: 101 Morello load store misc4
       DecodeMorelloLoadStoreMisc4(instr);
      break;
    case 0x6:
      if (instr->Bit(23) == 0) {
        // op0: 110 op1: 0xx Morello load unsigned offset
	DecodeMorelloLoadStoreUnsignedOffset(instr);
      } else {
	switch (instr->Bits(22, 21)) {
	  case 0x0:
	    // op0: 110 op1: 100 Morello get/set system register
	    DecodeMorelloGetSetSystemRegister(instr);
	    break;
	  case 0x1:
	    // op0: 110 op1: 101 Morello ADD (extended register)
	    DecodeMorelloAddExtendedRegister(instr);
	    break;
	  default:
	    // op0: 110 op1: 11x Morello morello_misc 
	    DecodeMorelloMisc(instr);
	    break;
      }
      break;
    case 0x7:
      // op0: 111 Morello load/store unscaled immediate via alternative base
      DecodeMorelloLoadStoreUnscaledImmediateViaAlternateBase(instr);
      break;
    }
  }
}

template <typename V>
void Decoder<V>::DecodeMorelloAddSubCapability(Instruction* instr) {
  DCHECK_EQ(0x02, instr->Bits(31, 24)); // [0000 0010][A][sh][imm12][Cn][Cd]
  V::VisitAddSubCapabilityImmediate(instr);
}

template <typename V>
void Decoder<V>::DecodeMorelloBranch(Instruction* instr) {
  DCHECK_EQ(0x2, instr->Bits(31, 15)); // TODO
  auto opc = instr->Bits(14, 13);
  switch (opc) {
    case 0x0:
      // BR (indirect)
      break;
    case 0x1:
      // BLR (indirect)
      break;
    case 0x2:
      // RET
      break;
    case 0x3:
      // BX 
      DCHECK_EQ(0x1F, instr->Bits(5, 9));
      break;
    default:
      // Fatal
      break;
  }
}

template <typename V>
void Decoder<V>::DecodeMorelloAddExtendedRegister(Instruction* instr) {
  DCHECK_EQ(0x615, instr->Bits(31, 21)); // [110 0001 0101][Rm][option][imm3][Cn][Cd]
}

template <typename V>
void Decoder<V>::DecodeMorelloGetSetSystemRegister(Instruction* instr) {
  DCHECK_EQ(0x314, instr->Bits(31, 21)); // [110 0001 0100][L][o0][op1][CRn][CRm][op2][Ct] 
  if (instr->Bit(20) == 0) {
     // L: 0 MSR
  } else {
     // L: 1 MSR
  }
}

template <typename V>
void Decoder<V>::DecodeMorelloLdrLiteral(Instruction* instr) {
  DCHECK_EQ(0x208, instr->Bits(31, 22) == 0x2); // [10 0000 1000][imm17][Ct]
}

template <typename V>
void Decoder<V>::DecodeMorelloLoadStoreMisc1(Instruction* instr) {
  DCHECK_EQ(0x42, instr->Bits(31, 24)); //  [0010 0010][L][op][Rs][o2][Ct2][Rn][Ct]
  if (instr->Bit(23) == 0) {
    // op0: 0 Morello load/ exclusive
    if (instr->Bit(22) == 0) {
      if (instr->Bit(21) == 0) {
        DCHECK_EQ(0x1F, instr->Bits(14,10)); // Ct2: 11111
        if (instr->Bit(15) == 0) {
          // L: 0, op: 0, o2: 0 STXR
	} else {
          // L: 0, op: 0, o2: 1 STLXR
	}
      } else {
        if (instr->Bit(15) == 0) {
          // L: 0, op: 1, o2: 0 STXP
        } else {
          // L: 0, op: 1, o2: 1 STLXP
        }
      }
    } else {
      DCHECK_EQ(0x1F, instr->Bits(20, 16)); // Rs: 11111
      if (instr->Bit(21) == 0) {
        DCHECK_EQ(0x1F, instr->Bits(14, 10)); // Ct2: 11111
        if (instr->Bit(15) == 0) {
          // L: 1, op: 0, o2: 0 LDXR 
        } else {
          // L: 1, op: 0, o2: 1 LDAXR
        }
      } else {
        if (instr->Bit(15) == 0) {
          // L: 1, op: 1, o2: 0 LDXP
        } else {
          // L: 1, op: 1, o2: 1 LDAXP 
        }
      }
    }
  } else {
    // op0: 1 Morello load/store pair postindex
    if (instr->Bit(22) == 0) {
      // L: 0 STP
    } else {
      // L: 1 LDP
    }
  }
}

template <typename V>
void Decoder<V>::DecodeMorelloLoadStoreMisc2(Instruction* instr) {
  DCHECK_EQ(0x84, instr->Bits(31, 24)); // [0 1000 0100][L][0][Rs][0][Ct2][Rn][Ct]
  if (instr->Bit(23) == 0) {
    if (instr->Bit(21) == 0) {
      if (instr->Bit(15) == 0) {
	// op0: 0, op1: 0, op2: 0 Morello load/store acquire/release capability via alternative base
	DCHECK_EQ(0x1F, instr->Bits(20, 16)); // Rs: 11111
	DCHECK_EQ(0x1F, instr->Bits(14, 10)); // Ct2: 11111
	if (instr->Bit(22) == 0) {
	  // L: 0 STLR (capability, alternative base)
	} else {
	  // L: 1 LDAR (capability, alternative base)
	}
      } else {
	// op0: 0, op1: 0, op2: 1 Morello load/store acquire/release
	DCHECK_EQ(0x1F, instr->Bits(20, 16)); // Rs: 11111
	DCHECK_EQ(0x1F, instr->Bits(14, 10)); // Ct2: 11111
 	if (instr->Bit(22) == 0) {
	  // L: 0 STLR (capability, normal base)
	} else {
	  // L: 1 LDAR (capability, normal base)
	}
     }
    } else {
      // op0: 0, op1: 1 Morello load/store acquire/release via alternative base
      DCHECK_EQ(0x1F, instr->Bits(20, 16)); // Rs: 11111
      DCHECK_EQ(0x1F, instr->Bits(14, 10)); // Rt2: 11111
      if (instr->Bit(22) == 0) {
        if (instr->Bit(15) == 0) {
	  // L: 0, op: 0 STLRB
	} else {
	  // L: 0, op: 1 STLR (integer)
	}
      } else {
        if (instr->Bit(15) == 0) {
	  // L: 1, op: 0 LDARB 
	} else {
	  // L: 1, op: 1 LDAR (integer) 
	}
      } 
    }
  } else {
    // op0: 1 Morello load/store pair
    if (instr->Bit(22) == 0) {
      // L: 0 STP (signed offset)
    } else {
      // L: 1 LDP (signed offset)
    }
  }
}

template <typename V>
void Decoder<V>::DecodeMorelloLoadStoreMisc3(Instruction* instr) {
  DCHECK_EQ(0xC4, instr->Bits(31, 23)); // [0 1100 0100][L][imm7][Ct2][Rn][Ct]
  if (instr->Bit(23) == 0) {
    // op0: 0 Morello load/store pair non-temporal
    if (instr->Bit(22) == 0) {
      // L: 0 STNP
    } else {
      // L: 1 LDNP
    }
  } else {
    // op0: 1 Morello load/store pair preindex
    if (instr->Bit(22) == 0) {
      // L: 0 STP (pre-indexed)
    } else {
      // L: 1 LDP (pre-indexed)
    }
  }
}

template <typename V>
void Decoder<V>::DecodeMorelloLoadStoreMisc4(Instruction* instr) {
  DCHECK_EQ(0xA2, instr->Bits(31, 24)); // [1010 0010][][op0][][op1][]
  if(instr->Bit(21) == 0) {
    if (instr->Bits(10, 11) == 0x00) {
      // op0: 0, op1: xxxx00 Morello load/store unscaled immediate
    } else if (instr->Bits(10, 11) == 0x01) {
      // op0: 0, op1: xxxx01 Morello load/store immediate translated
    } else if (instr->Bits(10, 11) == 0x02) {
      // op0: 0, op1: xxxx20 Morello load/store unscaled translated 
    } else {
      // op0: 0, op1: xxxx11 Morello load/store immediate preindex
    }
  } else {
    // TODO
  }
}

template <typename V>
void Decoder<V>::DecodeMorelloLoadStoreRegisterViaAlternativeBase(Instruction* instr) {
  DCHECK_EQ(0x105, instr->Bits(31, 23)); // [1 0000 0101][L][op][Rm][A][1][B][S][opc][Rn][Rt]
  if (instr->Bit(22) == 0x00)  {
    if (instr->Bit(21) == 0) {
      if (instr->Bits(11, 10) == 0x00) {
        // L: 0, op: 0, opc: 00 STRB (register offset)
      } else if (instr->Bits(11, 10) == 0x01) {
        // L: 0, op: 0, opc: 01 LDRSB -- doubleword
      } else if (instr->Bits(11, 10) == 0x02) {
        // L: 0, op: 0, opc: 10 LDRSH -- doubleword
      } else {
        // L: 0, op: 10 opc: 11 STH
      }
    } else {
      if (instr->Bits(11, 10) == 0x00) {
        // L: 0, op: 1, opc: 00 STR (register offset, integer) -- word
      } else if (instr->Bits(11, 10) == 0x01) {
        // L: 0, op: 1, opc: 01 STR (register offset, integer) -- double word
      } else if (instr->Bits(11, 10) == 0x01) {
        // L: 0, op: 1, opc: 10 STR (register offset, SIMD&FP) -- 64-bit 
      } else {
        // L: 0, op: 1, opc: 11 STR (register offset, SIMD&FP) -- 32-bit 
      }
    }
  } else {
    if (instr->Bit(21) == 0) {
      if (instr->Bits(11, 10) == 0x00) {
        // L: 1, op: 0, opc: 00 STRB (register offset)
      } else if (instr->Bits(11, 10) == 0x01) {
        // L: 1, op: 0, opc: 01 LDRSB -- doubleword
      } else if (instr->Bits(11, 10) == 0x02) {
        // L: 1, op: 0, opc: 10 LDRSH -- doubleword
      } else {
        // L: 1, op: 10 opc: 11 STH
      }
    } else {
      if (instr->Bits(11, 10) == 0x00) {
        // L: 1, op: 1, opc: 00 LDR (register offset, integer) -- word
      } else if (instr->Bits(11, 10) == 0x01) {
        // L: 1, op: 1, opc: 01 LDR (register offset, integer) -- double word
      } else if (instr->Bits(11, 10) == 0x01) {
        // L: 1, op: 1, opc: 10 LDR (register offset, SIMF&FP0 -- 64-bit
      } else {
        // L: 1, op: 1, opc: 11 LDR (register offset, SIMF&FP0 -- 32-bit
      }
    }
  }
}

template <typename V>
void Decoder<V>::DecodeMorelloLoadStoreUnsignedOffset(Instruction* instr) {
  DCHECK_EQ(0x184, instr->Bits(31, 23)); // [1 1000 0100][L][imm12][Rn][Ct] 
  if (instr->Bit(22) == 0) {
    // L: 0 STR (unsigned offset, capability normal base)
    V::VisitLoadStoreCapUnsignedOffsetCapNormal(instr);
  } else {
    // L: 1 LDR (unsigned offset, capability normal base)
    V::VisitLoadStoreCapUnsignedOffsetCapNormal(instr);
  }
}

template <typename V>
void Decoder<V>::DecodeMorelloLoadStoreUnsignedOffsetViaAlternativeBase(Instruction* instr) {
  DCHECK_EQ(0x209, instr->Bits(31, 22)); // [10 0000 1001][L][imm9][op][Rn][Rt] 
  if (instr->Bit(21) == 0) {
    if (instr->Bits(11, 10) == 0x00) {
      // L: 0, op: 00 STR (unsigned offset, capability alternate base)
    } else if (instr->Bits(11, 10) == 0x01) {
      // L: 0, op: 01 STRB (unsigned offset)
    } else if (instr->Bits(11, 10) == 0x02) {
      // L: 0, op: 10 STR (unsigned offset, integer) -- word
    } else {
      // L: 0, op: 11 STR (unsigned offset, integer) -- double word
    }
  } else {
    if (instr->Bits(11, 10) == 0x00) {
      // L: 1, op: 00 LDR (unsigned offset, capability alternate base)
    } else if (instr->Bits(11, 10) == 0x01) {
      // L: 1, op: 01 LDRB (unsigned offset)
    } else if (instr->Bits(11, 10) == 0x02) {
      // L: 1, op: 10 STR (unsigned offset, integer) -- word
    } else {
      // L: 1, op: 11 LDR (unsigned offset, integer) -- double word
    }
   }
}

template <typename V>
void Decoder<V>::DecodeMorelloLoadStoreUnscaledImmediateViaAlternateBase(Instruction* instr) {
  DCHECK_EQ(0x02, instr->Bits(31, 24)); // [1110  0010][op1][V][imm9][op2][Rn][Rd] 
  if (instr->Bits(23, 22) == 0x00) {
    if (instr->Bit(21 == 0)) {
      if (instr->Bits(11, 10) == 0x00) {
        // op1: 00, V: 0, op2: 00 STURB
      } else if (instr->Bits(11, 10) == 0x01) {
        // op1: 00, V: 0, op2: 01 LDURB
      } else if (instr->Bits(11, 10) == 0x10) {
        // op1: 00, V: 0, op2: 10 LDURSB -- doubleword 
      } else {
        DCHECK_EQ(0x03, instr->Bits(11, 10));
        // op1: 00, V: 0, op2: 11 LDURSB -- word 
      }
    } else {
      if (instr->Bits(11, 10) == 0x00) {
        // op1: 00, V: 1, op2: 00 STUR (SIMD&FP) -- 8-bit
      } else if (instr->Bits(11, 10) == 0x01) {
        // op1: 00, V: 1, op2: 01 LDUR (SIMD&FP) -- 8-bit
      } else if (instr->Bits(11, 10) == 0x10) {
        // op1: 00, V: 1, op2: 10 STUR (SIMD&FP) -- 128-bit 
      } else {
        DCHECK_EQ(0x03, instr->Bits(11, 10));
        // op1: 00, V: 1, op2: 11 LDUR (SIMD&FP) -- 128-bit
      }
    }
  } else if (instr->Bits(23, 22) == 0x01) {
    if (instr->Bit(21 == 0)) {
      if (instr->Bits(11, 10) == 0x00) {
        // op1: 01, V: 0, op2: 00 STURH
      } else if (instr->Bits(11, 10) == 0x01) {
        // op1: 01, V: 0, op2: 01 LDURH
      } else if (instr->Bits(11, 10) == 0x10) {
        // op1: 01, V: 0, op2: 10 LDURSH -- doubleword
      } else {
        DCHECK_EQ(0x03, instr->Bits(11, 10));
        // op1: 01, V: 0, op2: 11 LDURSH -- word
      }
    } else {
      if (instr->Bits(11, 10) == 0x00) {
        // op1: 01, V: 1, op2: 00 STUR (SIMD&FP) -- 16-bit
      } else if (instr->Bits(11, 10) == 0x01) {
        // op1: 01, V: 1, op2: 01 LDUR (SIMD&FP) -- 16-bit
      } else {
	V::VisitUnallocated(instr);
      }
    }
  } else if (instr->Bits(23, 22) == 0x10) {
    if (instr->Bit(21 == 0)) {
      if (instr->Bits(11, 10) == 0x00) {
        // op1: 10, V: 0, op2: 00 STUR (integer) -- word
      } else if (instr->Bits(11, 10) == 0x01) {
        // op1: 10, V: 0, op2: 01 LDUR (integer) -- word
      } else if (instr->Bits(11, 10) == 0x10) {
        // op1: 10, V: 0, op2: 10 LDURSW
      } else {
        DCHECK_EQ(0x03, instr->Bits(11, 10));
        // op1: 10, V: 0, op2: 11 STUR (capability, alternate base)
      }
    } else {
      if (instr->Bits(11, 10) == 0x00) {
        // op1: 10, V: 1, op2: 00 STUR (SIMD&FP) --32-bit 
      } else if (instr->Bits(11, 10) == 0x01) {
        // op1: 10, V: 1, op2: 01 LDUR (SIMD&FP) -- 32-bit 
      } else {
	V::VisitUnallocated(instr);
      }
    }
  } else {
    DCHECK_EQ(0x03, instr->Bits(23, 22));
    if (instr->Bit(21 == 0)) {
      if (instr->Bits(11, 10) == 0x00) {
        // op1: 11, V: 0, op2: 00 STUR (integer) -- doubleword
      } else if (instr->Bits(11, 10) == 0x01) {
        // op1: 11, V: 0, op2: 01 LDUR (integer) -- doubleword
      } else if (instr->Bits(11, 10) == 0x11) {
        // op1: 11, V: 0, op2: 11 LDUR (capability, alternate base) 
      } else {
	V::VisitUnallocated(instr);
      }
    } else {
      if (instr->Bits(11, 10) == 0x00) {
        // op1: 11, V: 1, op2: 00 STUR (SIMD&FP) -- 64-bit 
      } else if (instr->Bits(11, 10) == 0x01) {
        // op1: 11, V: 1, op2: 01 LDUR (SIMD&FP) -- 64-bit
      } else {
	V::VisitUnallocated(instr);
      }
    }
  }
}

template <typename V>
void Decoder<V>::DecodeMorelloMisc(Instruction* instr) {
  DCHECK_EQ(0x30B, instr->Bits(31, 22)); // [11 0000 1011][op0][op1][op2][][op3] 
  if (instr->Bit(21) == 0x0) {
    if (instr->Bits(12, 11) == 0x0) {
      if (instr->Bit(10) == 0x0) {
        if (instr->Bits(15, 13) == 0x7) {
          // op0: 0xxxxx111, op1: 00, op2: 0 SCFLGS
        } else if (instr->Bits(15, 13) == 0x6) {
          // op0: 0xxxxx110, op1: 00, op2: 0 CVT (to pointer) 
        } else if (instr->Bit(15) == 0x1) {
          // op0: 0xxxxx110, op1: 00, op2: 0 Morello set field 2
        } else {
          // op0: 0xxxxx0xx, op1: 00, op2: 0 Morello set field 1
        }
      } else {
        DCHECK_EQ(0x1, instr->Bit(10));
        if (instr->Bit(15) == 0x0) {
          // op0: 0xxxxx0xx, op1: 00, op2: 1 Morello miscellaneous capability 1
        } else {
          DCHECK_EQ(0x0, instr->Bits(4, 1));
    	  if (instr->Bit(0) == 0) {
            // op0: 0xxxxx0xx, op1: 00, op2: 1 op3: 00000 Morello branch to sealed
	  } else {
            // op0: 0xxxxx0xx, op1: 00, op2: 1 op3: 00001 Morello 2 src cap
	  }
        }
      }
    } else if (instr->Bits(12, 11) == 0x1) {
      DCHECK_EQ(0x1, instr->Bit(13));
      // op0: 0xxxxx0xx, op1: 00, op2: 1 op3: 00001 Morello miscellaneous capability 2
    } else if (instr->Bits(12, 11) == 0x2) {
      DCHECK_EQ(0x0, instr->Bit(10));
      if (instr->Bit(20) == 0x0) {
        DCHECK_EQ(0x0, instr->Bit(19));
	if (instr->Bit(18) == 0x0) {
	  if (instr->Bit(17) == 0x0) {
            if (instr->Bits(16, 15) == 0x2) {
	      // op0: 0000010xx, op1: 10, op2: 0 Morello get field 2
	    } else if (instr->Bits(16, 15) == 0x3) {
	      // op0: 0000011xx, op1: 10, op2: 0 Morello miscellaneous capability 0
	      if (instr->Bits(14, 13) == 0x0) {
		// opc: 00 CLRTAG
	      } else if (instr->Bits(14, 13) == 0x2) {
	        // opc: 10 MOV/CPY
	        V::VisitCopyCapability(instr);
	      } else {
	        V::VisitUnallocated(instr);
	      }
	    } else {
	      CHECK_EQ(0x0, instr->Bit(16));
	      // op0: 000000xxx, op1: 10, op2: 0 Morello get field 1
	    }
	  } else {
	    DCHECK_EQ(0x1, instr->Bit(17));
	    DCHECK_EQ(0x0, instr->Bits(16, 15));
	    if (instr->Bits(4, 0) == 0x0) {
	      // op0: 0000100xx, op1: 10, op2: 0 op3: 00000 Morello branch
              DecodeMorelloBranch(instr);
	    } else if (instr->Bits(4, 0) == 0x1) {
	      // op0: 0000100xx, op1: 10, op2: 0 op3: 00001 Morello checks
	    } else if (instr->Bits(4, 0) == 0x2) {
	      // op0: 0000100xx, op1: 10, op2: 0 op3: 00010 Morello branch sealed direct 
	    } else {
	      DCHECK_EQ(0x3, instr->Bits(4, 0));
	      // op0: 0000100xx, op1: 10, op2: 0 op3: 00011 Morello branch sealed restricted 
	    }
	  }
	} else {
	  DCHECK_EQ(0x1, instr->Bit(18));
	  if (instr->Bit(16) == 0x0) {
	    if (instr->Bit(17) == 0x0) {
	      if (instr->Bit(15) == 0x0) {
	        // op0: 0001000xx, op1: 10, op2: 0 Morello load pair and branch
	      } else {
	        // op0: 0001001xx, op1: 10, op2: 0 Morello load/store tags
	      }
	    } else {
	      DCHECK_EQ(0x1, instr->Bit(17));
	      DCHECK_EQ(0x0, instr->Bit(15));
	      // op0: 0001110xx, op1: 10, op2: 0 Morello 1 src 1 dest
	    }
	  } else {
	    DCHECK_EQ(0x1, instr->Bit(16));
	    if (instr->Bit(17) == 0x0) {
	      if (instr->Bit(15) == 0x0) {
	        // op0: 0001010xx, op1: 10, op2: 0 Morello convert to pointer
	      } else {
	        // op0: 0001011xx, op1: 10, op2: 0 Morello convert to capability with implicit operand
	      }
	    } else {
	      DCHECK_EQ(0x1, instr->Bit(17));
	      // op0: 000110xxx, op1: 10, op2: 0 CLRPERM (immediate)
	    }
	  }
	}
      } else {
        DCHECK_EQ(0x0, instr->Bits(4 ,1));
        // op0: 01xxxxxxx, op1: 10, op2: 0, op3: 0000x Morello brach sealed indirect
      }
    }
  } else {
    DCHECK_EQ(0x1, instr->Bit(21));
    if (instr->Bits(12, 11) == 0x3) {
      if (instr->Bit(10) == 0x0) {
        DCHECK_EQ(0x0, instr->Bit(13));
        // op0: 1xxxxx0x0, op1: = 11 op2: 0 Morello convert capability
      } else {
        DCHECK_EQ(0x1, instr->Bit(10));
        DCHECK_EQ(0x0, instr->Bits(14, 13));
        // op0: 1xxxxx100, op1: = 11 op2: 0  SUBS
      }
    } else {
      if (instr->Bit(10) == 0x0) {
        DCHECK_EQ(0x1, instr->Bit(14));
        // op0: 1xxxxxxxx, op1: != 11 op2: 0  Morello logical immediate
      } else {
        DCHECK_EQ(0x1, instr->Bit(10));
        // op0: 1xxxxxx1x, op2: 1 Morello load/store capability via alternate base
      }
    }
  }
}
#endif

}  // namespace internal
}  // namespace v8

#endif  // V8_CODEGEN_ARM64_DECODER_ARM64_INL_H_
