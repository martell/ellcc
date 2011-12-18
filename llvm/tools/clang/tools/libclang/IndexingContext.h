//===- IndexingContext.h - Higher level API functions ------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Index_Internal.h"
#include "CXCursor.h"

#include "clang/AST/DeclObjC.h"
#include "clang/AST/DeclGroup.h"
#include "llvm/ADT/DenseSet.h"

namespace clang {
  class FileEntry;
  class ObjCPropertyDecl;
  class ObjCClassDecl;

namespace cxindex {
  class IndexingContext;

struct DeclInfo : public CXIdxDeclInfo {
  CXIdxEntityInfo CXEntInfo;
  enum DInfoKind {
    Info_Decl,

    Info_ObjCContainer,
      Info_ObjCInterface,
      Info_ObjCProtocol,
      Info_ObjCCategory
  };
  
  DInfoKind Kind;

  DeclInfo(bool isRedeclaration, bool isDefinition, bool isContainer)
    : Kind(Info_Decl) {
    this->isRedeclaration = isRedeclaration;
    this->isDefinition = isDefinition;
    this->isContainer = isContainer;
    attributes = 0;
    numAttributes = 0;
  }
  DeclInfo(DInfoKind K,
           bool isRedeclaration, bool isDefinition, bool isContainer)
    : Kind(K) {
    this->isRedeclaration = isRedeclaration;
    this->isDefinition = isDefinition;
    this->isContainer = isContainer;
    attributes = 0;
    numAttributes = 0;
  }

  static bool classof(const DeclInfo *) { return true; }
};

struct ObjCContainerDeclInfo : public DeclInfo {
  CXIdxObjCContainerDeclInfo ObjCContDeclInfo;

  ObjCContainerDeclInfo(bool isForwardRef,
                        bool isRedeclaration,
                        bool isImplementation)
    : DeclInfo(Info_ObjCContainer, isRedeclaration,
               /*isDefinition=*/!isForwardRef, /*isContainer=*/!isForwardRef) {
    init(isForwardRef, isImplementation);
  }
  ObjCContainerDeclInfo(DInfoKind K,
                        bool isForwardRef,
                        bool isRedeclaration,
                        bool isImplementation)
    : DeclInfo(K, isRedeclaration, /*isDefinition=*/!isForwardRef,
               /*isContainer=*/!isForwardRef) {
    init(isForwardRef, isImplementation);
  }

  static bool classof(const DeclInfo *D) {
    return Info_ObjCContainer <= D->Kind && D->Kind <= Info_ObjCCategory;
  }
  static bool classof(const ObjCContainerDeclInfo *D) { return true; }

private:
  void init(bool isForwardRef, bool isImplementation) {
    if (isForwardRef)
      ObjCContDeclInfo.kind = CXIdxObjCContainer_ForwardRef;
    else if (isImplementation)
      ObjCContDeclInfo.kind = CXIdxObjCContainer_Implementation;
    else
      ObjCContDeclInfo.kind = CXIdxObjCContainer_Interface;
  }
};

struct ObjCInterfaceDeclInfo : public ObjCContainerDeclInfo {
  CXIdxObjCInterfaceDeclInfo ObjCInterDeclInfo;
  CXIdxObjCProtocolRefListInfo ObjCProtoListInfo;

  ObjCInterfaceDeclInfo(const ObjCInterfaceDecl *D)
    : ObjCContainerDeclInfo(Info_ObjCInterface,
                            /*isForwardRef=*/false,
                            /*isRedeclaration=*/D->isInitiallyForwardDecl(),
                            /*isImplementation=*/false) { }

  static bool classof(const DeclInfo *D) {
    return D->Kind == Info_ObjCInterface;
  }
  static bool classof(const ObjCInterfaceDeclInfo *D) { return true; }
};

struct ObjCProtocolDeclInfo : public ObjCContainerDeclInfo {
  CXIdxObjCProtocolRefListInfo ObjCProtoRefListInfo;

  ObjCProtocolDeclInfo(const ObjCProtocolDecl *D)
    : ObjCContainerDeclInfo(Info_ObjCProtocol,
                            /*isForwardRef=*/false,
                            /*isRedeclaration=*/D->isInitiallyForwardDecl(),
                            /*isImplementation=*/false) { }

  static bool classof(const DeclInfo *D) {
    return D->Kind == Info_ObjCProtocol;
  }
  static bool classof(const ObjCProtocolDeclInfo *D) { return true; }
};

struct ObjCCategoryDeclInfo : public ObjCContainerDeclInfo {
  CXIdxObjCCategoryDeclInfo ObjCCatDeclInfo;

  explicit ObjCCategoryDeclInfo(bool isImplementation)
    : ObjCContainerDeclInfo(Info_ObjCCategory,
                            /*isForwardRef=*/false,
                            /*isRedeclaration=*/isImplementation,
                            /*isImplementation=*/isImplementation) { }

  static bool classof(const DeclInfo *D) {
    return D->Kind == Info_ObjCCategory;
  }
  static bool classof(const ObjCCategoryDeclInfo *D) { return true; }
};

struct AttrInfo : public CXIdxAttrInfo {
  const Attr *A;

  AttrInfo(CXIdxAttrKind Kind, CXCursor C, CXIdxLoc Loc, const Attr *A) {
    kind = Kind;
    cursor = C;
    loc = Loc;
    this->A = A;
  }

  static bool classof(const AttrInfo *) { return true; }
};

struct IBOutletCollectionInfo : public AttrInfo {
  CXIdxEntityInfo CXClassInfo;
  CXIdxIBOutletCollectionAttrInfo IBCollInfo;

  IBOutletCollectionInfo(CXCursor C, CXIdxLoc Loc, const Attr *A) :
    AttrInfo(CXIdxAttr_IBOutletCollection, C, Loc, A) {
    assert(C.kind == CXCursor_IBOutletCollectionAttr);
  }

  static bool classof(const AttrInfo *A) {
    return A->kind == CXIdxAttr_IBOutletCollection;
  }
  static bool classof(const IBOutletCollectionInfo *D) { return true; }
};

struct RefFileOccurence {
  const FileEntry *File;
  const Decl *Dcl;

  RefFileOccurence(const FileEntry *File, const Decl *Dcl)
    : File(File), Dcl(Dcl) { }
};

class IndexingContext {
  ASTContext *Ctx;
  CXClientData ClientData;
  IndexerCallbacks &CB;
  unsigned IndexOptions;
  CXTranslationUnit CXTU;
  
  typedef llvm::DenseMap<const FileEntry *, CXIdxClientFile> FileMapTy;
  typedef llvm::DenseMap<const DeclContext *, CXIdxClientContainer> ContainerMapTy;
  FileMapTy FileMap;
  ContainerMapTy ContainerMap;

  llvm::DenseSet<RefFileOccurence> RefFileOccurences;

  SmallVector<DeclGroupRef, 8> TUDeclsInObjCContainer;
  
  llvm::SmallString<256> StrScratch;
  unsigned StrAdapterCount;

  class StrAdapter {
    llvm::SmallString<256> &Scratch;
    IndexingContext &IdxCtx;

  public:
    StrAdapter(IndexingContext &indexCtx)
      : Scratch(indexCtx.StrScratch), IdxCtx(indexCtx) {
      ++IdxCtx.StrAdapterCount;
    }

    ~StrAdapter() {
      --IdxCtx.StrAdapterCount;
      if (IdxCtx.StrAdapterCount == 0)
        Scratch.clear();
    }

    const char *toCStr(StringRef Str);

    unsigned getCurSize() const { return Scratch.size(); }

    const char *getCStr(unsigned CharIndex) {
      Scratch.push_back('\0');
      return Scratch.data() + CharIndex;
    }

    SmallVectorImpl<char> &getBuffer() { return Scratch; }
  };

  struct ObjCProtocolListInfo {
    SmallVector<CXIdxObjCProtocolRefInfo, 4> ProtInfos;
    SmallVector<CXIdxEntityInfo, 4> ProtEntities;
    SmallVector<CXIdxObjCProtocolRefInfo *, 4> Prots;

    CXIdxObjCProtocolRefListInfo getListInfo() const {
      CXIdxObjCProtocolRefListInfo Info = { Prots.data(),
                                            (unsigned)Prots.size() };
      return Info;
    }

    ObjCProtocolListInfo(const ObjCProtocolList &ProtList,
                         IndexingContext &IdxCtx,
                         IndexingContext::StrAdapter &SA);
  };

  struct AttrListInfo {
    SmallVector<AttrInfo, 2> Attrs;
    SmallVector<IBOutletCollectionInfo, 2> IBCollAttrs;
    SmallVector<CXIdxAttrInfo *, 2> CXAttrs;

    const CXIdxAttrInfo *const *getAttrs() const {
      return CXAttrs.data();
    }
    unsigned getNumAttrs() const { return (unsigned)CXAttrs.size(); }

    AttrListInfo(const Decl *D,
                 IndexingContext &IdxCtx,
                 IndexingContext::StrAdapter &SA);
};

public:
  IndexingContext(CXClientData clientData, IndexerCallbacks &indexCallbacks,
                  unsigned indexOptions, CXTranslationUnit cxTU)
    : Ctx(0), ClientData(clientData), CB(indexCallbacks),
      IndexOptions(indexOptions), CXTU(cxTU), StrAdapterCount(0) { }

  ASTContext &getASTContext() const { return *Ctx; }

  void setASTContext(ASTContext &ctx);

  bool suppressRefs() const {
    return IndexOptions & CXIndexOpt_SuppressRedundantRefs;
  }

  bool shouldAbort();

  void enteredMainFile(const FileEntry *File);

  void ppIncludedFile(SourceLocation hashLoc,
                      StringRef filename, const FileEntry *File,
                      bool isImport, bool isAngled);

  void startedTranslationUnit();

  void indexDecl(const Decl *D);

  void indexTagDecl(const TagDecl *D);

  void indexTypeSourceInfo(TypeSourceInfo *TInfo, const NamedDecl *Parent,
                           const DeclContext *DC = 0);

  void indexTypeLoc(TypeLoc TL, const NamedDecl *Parent,
                           const DeclContext *DC);

  void indexDeclContext(const DeclContext *DC);
  
  void indexBody(const Stmt *S, const DeclContext *DC);

  void handleDiagnosticSet(CXDiagnosticSet CXDiagSet);

  bool handleFunction(const FunctionDecl *FD);

  bool handleVar(const VarDecl *D);

  bool handleField(const FieldDecl *D);

  bool handleEnumerator(const EnumConstantDecl *D);

  bool handleTagDecl(const TagDecl *D);
  
  bool handleTypedef(const TypedefDecl *D);

  bool handleObjCClass(const ObjCClassDecl *D);
  bool handleObjCInterface(const ObjCInterfaceDecl *D);
  bool handleObjCImplementation(const ObjCImplementationDecl *D);

  bool handleObjCForwardProtocol(const ObjCProtocolDecl *D,
                                 SourceLocation Loc,
                                 bool isRedeclaration);

  bool handleObjCProtocol(const ObjCProtocolDecl *D);

  bool handleObjCCategory(const ObjCCategoryDecl *D);
  bool handleObjCCategoryImpl(const ObjCCategoryImplDecl *D);

  bool handleObjCMethod(const ObjCMethodDecl *D);

  bool handleSynthesizedObjCProperty(const ObjCPropertyImplDecl *D);
  bool handleSynthesizedObjCMethod(const ObjCMethodDecl *D, SourceLocation Loc);

  bool handleObjCProperty(const ObjCPropertyDecl *D);

  bool handleReference(const NamedDecl *D, SourceLocation Loc, CXCursor Cursor,
                       const NamedDecl *Parent,
                       const DeclContext *DC,
                       const Expr *E = 0,
                       CXIdxEntityRefKind Kind = CXIdxEntityRef_Direct);

  bool handleReference(const NamedDecl *D, SourceLocation Loc,
                       const NamedDecl *Parent,
                       const DeclContext *DC,
                       const Expr *E = 0,
                       CXIdxEntityRefKind Kind = CXIdxEntityRef_Direct);

  bool isNotFromSourceFile(SourceLocation Loc) const;

  void indexTopLevelDecl(Decl *D);
  void indexTUDeclsInObjCContainer();
  void indexDeclGroupRef(DeclGroupRef DG);

  void addTUDeclInObjCContainer(DeclGroupRef DG) {
    TUDeclsInObjCContainer.push_back(DG);
  }

  void translateLoc(SourceLocation Loc, CXIdxClientFile *indexFile, CXFile *file,
                    unsigned *line, unsigned *column, unsigned *offset);

private:
  bool handleDecl(const NamedDecl *D,
                  SourceLocation Loc, CXCursor Cursor,
                  DeclInfo &DInfo);

  bool handleObjCContainer(const ObjCContainerDecl *D,
                           SourceLocation Loc, CXCursor Cursor,
                           ObjCContainerDeclInfo &ContDInfo);

  void addContainerInMap(const DeclContext *DC, CXIdxClientContainer container);

  bool markEntityOccurrenceInFile(const NamedDecl *D, SourceLocation Loc);

  const NamedDecl *getEntityDecl(const NamedDecl *D) const;

  CXIdxClientContainer getIndexContainer(const NamedDecl *D) const {
    return getIndexContainerForDC(D->getDeclContext());
  }

  const DeclContext *getScopedContext(const DeclContext *DC) const;
  CXIdxClientContainer getIndexContainerForDC(const DeclContext *DC) const;

  CXIdxClientFile getIndexFile(const FileEntry *File);
  
  CXIdxLoc getIndexLoc(SourceLocation Loc) const;

  void getEntityInfo(const NamedDecl *D,
                     CXIdxEntityInfo &EntityInfo,
                     StrAdapter &SA);

  CXCursor getCursor(const Decl *D) {
    return cxcursor::MakeCXCursor(const_cast<Decl*>(D), CXTU);
  }

  CXCursor getRefCursor(const NamedDecl *D, SourceLocation Loc);
};

}} // end clang::cxindex

namespace llvm {
  /// Define DenseMapInfo so that FileID's can be used as keys in DenseMap and
  /// DenseSets.
  template <>
  struct DenseMapInfo<clang::cxindex::RefFileOccurence> {
    static inline clang::cxindex::RefFileOccurence getEmptyKey() {
      return clang::cxindex::RefFileOccurence(0, 0);
    }

    static inline clang::cxindex::RefFileOccurence getTombstoneKey() {
      return clang::cxindex::RefFileOccurence((const clang::FileEntry *)~0,
                                              (const clang::Decl *)~0);
    }

    static unsigned getHashValue(clang::cxindex::RefFileOccurence S) {
      llvm::FoldingSetNodeID ID;
      ID.AddPointer(S.File);
      ID.AddPointer(S.Dcl);
      return ID.ComputeHash();
    }

    static bool isEqual(clang::cxindex::RefFileOccurence LHS,
                        clang::cxindex::RefFileOccurence RHS) {
      return LHS.File == RHS.File && LHS.Dcl == RHS.Dcl;
    }
  };
}