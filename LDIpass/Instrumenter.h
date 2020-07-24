//===-- Instrumenter.h - instruction level instrumentation ----------------===//
//
// Copyright (C) <2019-2024>  <Wen Li>
//
//                      The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TRANSFORMS_INSTRUMENTATION_INSTRUMENTER_H
#define LLVM_LIB_TRANSFORMS_INSTRUMENTATION_INSTRUMENTER_H
#include <set>
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/BlockFrequencyInfo.h"
#include "llvm/Analysis/BranchProbabilityInfo.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/Support/BranchProbability.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "HookFunc.h"
#include "LLVMInst.h"
#include "LdaBin.h"


using namespace llvm;
using namespace std;

struct CSTaint
{
    CSTaint ()
    {
        m_InTaintBits  = 0;
        m_OutTaintBits = 0;
    }
    
    set <string> m_Callees;
    unsigned m_InTaintBits;
    unsigned m_OutTaintBits;
};

typedef map <unsigned, CSTaint>::iterator mcs_iterator;
class Flda
{
private:
    string m_FuncName;
    map <unsigned, CSTaint> m_CsID2Cst;
    set <unsigned> m_TaintInstIDs;

public:
    Flda (string FuncName)
    {
        m_FuncName = FuncName;
    }

    ~Flda ()
    {

    }

    inline CSTaint* GetCsTaint (unsigned InstId)
    {
        auto It = m_CsID2Cst.find (InstId);
        if (It == m_CsID2Cst.end ())
        {
            return NULL;
        }
        else
        {
            return &(It->second);
        }
    }

    inline CSTaint* AddCst (unsigned Id)
    {
        auto It = m_CsID2Cst.find (Id);
        if (It == m_CsID2Cst.end())
        {
            auto Pit = m_CsID2Cst.insert (make_pair(Id, CSTaint ()));
            assert (Pit.second == true);

            return &(Pit.first->second);
        }
        else
        {
            return &(It->second);
        }
    }

    inline bool IsInstTainted (unsigned Id)
    {
        auto It = m_TaintInstIDs.find (Id);
        if (It == m_TaintInstIDs.end())
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    inline void AddInstID (unsigned Id)
    {
         m_TaintInstIDs.insert (Id);
    }
};

class Instrumenter
{
private:
    Module *m_Module;

    Constant *m_TackFunc;
    Constant *m_InitFunc;
    Constant *m_ExitFunc;

    map <string, Flda> m_Fname2Flda;

    map<StringRef, Value*> m_GlobalPtrMap;

    map <Value*, unsigned> m_Value2Id;

public:
    
    Instrumenter(Module *M)
    {
        m_Module = M;


        TraceFunc Trb;
        m_TackFunc = Trb.geHookFunction (M);
        assert (m_TackFunc != NULL);
        m_InitFunc = Trb.geInitFunction (M);
        m_ExitFunc = Trb.geExitFunction (M);
    } 

    void RunInstrm ()
    {
        LoadLdaBin ();
        
        VisitFunction ();
    }
    
private:

    Value* GetGlobalPtr(StringRef strRef, IRBuilder<> *IRB)
    {
        auto It = m_GlobalPtrMap.find(strRef);
        if(It != m_GlobalPtrMap.end())
        {
            return It->second;
        }
        else
        {
            Value* VStr = IRB->CreateGlobalStringPtr(strRef);
            m_GlobalPtrMap[strRef] = VStr;
            return VStr;
        }
  }

    inline Flda* GetFlda (string Fname)
    {
        auto It = m_Fname2Flda.find (Fname);
        if (It == m_Fname2Flda.end())
        {
            return NULL;
        }
        else
        {
            return &(It->second);
        }
    }

    inline Flda* AddFlda (string Fname)
    {
        Flda* Fd = GetFlda (Fname);
        if (Fd == NULL)
        {
            auto Pit = m_Fname2Flda.insert (make_pair(Fname, Flda (Fname)));
            assert (Pit.second == true);

            return &(Pit.first->second);
        }
        else
        {
            return Fd;
        }
    }

    inline void LoadLdaBin ()
    {
        size_t N;
        FILE *Bf = fopen ("LdaBin.bin", "rb");
        assert (Bf != NULL);
        
        unsigned FldaNum = 0;
        N = fread (&FldaNum, sizeof(FldaNum), 1, Bf);
        assert (N == 1);
        printf ("FldaNum = %u \r\n", FldaNum);

        for (unsigned Fid = 0; Fid < FldaNum; Fid++)
        {
            FldaBin Fdb = {0};
            N = fread (&Fdb, sizeof(Fdb), 1, Bf);
            assert (N == 1);
            Flda *Fd = AddFlda (Fdb.FuncName);

            printf ("Load Function: %s\r\n", Fdb.FuncName);
            for (unsigned Iid = 0; Iid < Fdb.TaintInstNum; Iid++)
            {
                unsigned Id = 0;
                N = fread (&Id, sizeof(Id), 1, Bf);
                assert (N == 1);
                Fd->AddInstID (Id);
                printf ("\tTaintInst: %u\r\n", Id);
            }        

            for (unsigned Cid = 0; Cid < Fdb.TaintCINum; Cid++)
            {
                CSTaintBin Cstb;
                N = fread (&Cstb, sizeof(Cstb), 1, Bf);
                assert (N == 1);

                CSTaint *Cst = Fd->AddCst (Cstb.InstID);
                Cst->m_InTaintBits  = Cstb.InTaintBits;
                Cst->m_OutTaintBits = Cstb.OutTaintBits;
                
                for (unsigned Fid = 0; Fid < Cstb.CalleeNum; Fid++)
                {
                    char CalleeName[F_NAME_LEN] = {0};
                    N = fread (CalleeName, sizeof(CalleeName), 1, Bf);
                    assert (N == 1);
                    Cst->m_Callees.insert (CalleeName);
                }
            }
        }

        fclose (Bf);
    }

    inline string GetValueType (Instruction* Inst, Value *Val)
    {
        unsigned VType = Val->getType ()->getTypeID ();
        switch (VType)
        {
            case Type::VoidTyID:
            {
                return "";
            }
            case Type::HalfTyID:
            {
                printf ("Type=> [HalfTyID]:%u \r\n", VType);
                break;
            }
            case Type::FloatTyID:
            {
                printf ("Type=> [FloatTyID]:%u \r\n", VType);
                break;
            }
            case Type::DoubleTyID:
            {
                printf ("Type=> [DoubleTyID]:%u \r\n", VType);
                break;
            }
            case Type::X86_FP80TyID:
            {
                printf ("Type=> [X86_FP80TyID]:%u \r\n", VType);
                break;
            }
            case Type::FP128TyID:
            {
                printf ("Type=> [FP128TyID]:%u \r\n", VType);
                break;
            }
            case Type::PPC_FP128TyID:
            {
                printf ("Type=> [PPC_FP128TyID]:%u \r\n", VType);
                break;
            }
            case Type::LabelTyID:
            {
                printf ("Type=> [LabelTyID]:%u \r\n", VType);
                break;
            }
            case Type::MetadataTyID:
            {
                printf ("Type=> [MetadataTyID]:%u \r\n", VType);
                break;
            }
            case Type::X86_MMXTyID:
            {
                printf ("Type=> [X86_MMXTyID]:%u \r\n", VType);
                break;
            }
            case Type::TokenTyID:
            {
                printf ("Type=> [TokenTyID]:%u \r\n", VType);
                break;
            }
            case Type::IntegerTyID:
            {
                return "%u";
            }
            case Type::FunctionTyID:
            {
                printf ("Type=> [FunctionTyID]:%u \r\n", VType);
                break;
            }
            case Type::StructTyID:
            {
                printf ("Type=> [StructTyID]:%u \r\n", VType);
                break;
            }
            case Type::ArrayTyID:
            {
                printf ("Type=> [ArrayTyID]:%u \r\n", VType);
                break;
            }
            case Type::PointerTyID:
            {
                return "%p";
                break;
            }
            case Type::VectorTyID:
            {
                printf ("Type=> [VectorTyID]:%u \r\n", VType);
                break;
            }
            default:
            {
                assert (0);
            }
        }

        return "";
    }

    inline string GetValueName (Value *Val)
    {
        if (Val->hasName ())
        {
            return Val->getName ().data();
        }
        
        unsigned Id;
        auto It = m_Value2Id.find (Val);
        if (It != m_Value2Id.end ())
        {
            Id = It->second;
        }
        else
        {
            Id = m_Value2Id.size()+1;
            m_Value2Id[Val] = Id;
        }

        return  "Val" + to_string(Id);
    }


    inline unsigned GetArgNo (unsigned TaintBit)
    {
        unsigned No = 1;
        while (TaintBit != 0)
        {
            TaintBit = TaintBit << 1;
            if (TaintBit & (1<<31))
            {
                return No;
            }

            No++;
        }

        return 0;
    }

    inline unsigned GetInstrPara (Flda *Fd, unsigned InstNo, Instruction* Inst, 
                                      string &Format, Value **ArgBuf)
    { 
        Value *Def = NULL;
        unsigned ArgIndex = 0;
        
        LLVMInst LI (Inst);
        
        Format = "{[";
        Format += to_string (InstNo) + "]";
        
        Def = LI.GetDef ();
        if (Def != NULL)
        {
            ArgBuf [ArgIndex++] = Def;            
            Format += GetValueName (Def) + ":" + GetValueType (Inst, Def) + "=";
        }
        else
        {
            errs()<<*Inst<<"\r\n";
            if (LI.IsRet ())
            {
                Format += "Ret=";
            }
            else
            {
                CSTaint *Cst = Fd->GetCsTaint (InstNo);
                unsigned Outbits = (~Cst->m_InTaintBits) & Cst->m_OutTaintBits;
                unsigned OutArg  =  GetArgNo(Outbits);
                assert (OutArg != 0);

                Def = LI.GetUse (OutArg-1);
                assert (Def != NULL);
                
                ArgBuf [ArgIndex++] = Def;            
                Format += GetValueName (Def) + ":" + GetValueType (Inst, Def) + "=";
            }   
        }

        for (auto It = LI.begin (); It != LI.end(); It++)
        {
            Value *Val = *It;
            if (Val == Def)
            {
                continue;
            }
            
            ArgBuf [ArgIndex++] = Val;
            Format += GetValueName (Val) + ":" + GetValueType (Inst, Val) + ",";
        }
        Format += "}";

        errs()<<"\tTrg: "<<Format<<"\r\n";

        return ArgIndex;
    }

    inline void AddTrack (Instruction* Inst, string Format, Value **ArgBuf, unsigned ArgNum)
    { 
        IRBuilder<> Builder(Inst);

        Value *TFormat = GetGlobalPtr(Format, &Builder);
        switch (ArgNum)
        {
            case 1:
            {
                Builder.CreateCall(m_TackFunc, {TFormat, ArgBuf[0]});
                break;
            }
            case 2:
            {
                Builder.CreateCall(m_TackFunc, {TFormat, ArgBuf[0], ArgBuf[1]});
                break;
            }
            case 3:
            {
                Builder.CreateCall(m_TackFunc, {TFormat, ArgBuf[0], ArgBuf[1], ArgBuf[2]});
                break;
            }
            default:
            {
                assert (0);
            }
        }

        return;
    }

    inline void VisitFunction ()
    {
        for (Module::iterator it = m_Module->begin(), eit = m_Module->end(); it != eit; ++it) 
        {
            Function *F = &*it;  
            if (F->isIntrinsic() || F->isDeclaration ())
            {
                continue;
            }

            Flda *Fd = GetFlda (F->getName ().data());
            if (Fd == NULL)
            {
                continue;
            }
            
            unsigned InstId = 0;
            m_Value2Id.clear ();
            errs()<<"Process Function: "<<F->getName ()<<"\r\n";

            string Format = "";
            Value *ArgBuf[4];
            unsigned ArgNum = 0;
            for (auto ItI = inst_begin(*F), Ed = inst_end(*F); ItI != Ed; ++ItI, InstId++) 
            {
                Instruction *Inst = &*ItI;

                if (Format != "")
                {
                    AddTrack (Inst, Format, ArgBuf, ArgNum);
                    Format = "";
                }
                
                if (Fd->IsInstTainted (InstId))
                {
                    ArgNum = GetInstrPara (Fd, InstId, Inst, Format, ArgBuf);
                }
            }

        }

        return;
    }
};



#endif // LLVM_LIB_TRANSFORMS_INSTRUMENTATION_INSTRUMENTER_H
