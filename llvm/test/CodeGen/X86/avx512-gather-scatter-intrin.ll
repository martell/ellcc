; RUN: llc < %s -mtriple=x86_64-apple-darwin -mcpu=skx | FileCheck %s

declare <16 x float> @llvm.x86.avx512.gather.dps.512 (<16 x float>, i8*, <16 x i32>, i16, i32)
declare void @llvm.x86.avx512.scatter.dps.512 (i8*, i16, <16 x i32>, <16 x float>, i32)
declare <8 x double> @llvm.x86.avx512.gather.dpd.512 (<8 x double>, i8*, <8 x i32>, i8, i32)
declare void @llvm.x86.avx512.scatter.dpd.512 (i8*, i8, <8 x i32>, <8 x double>, i32)

declare <8 x float> @llvm.x86.avx512.gather.qps.512 (<8 x float>, i8*, <8 x i64>, i8, i32)
declare void @llvm.x86.avx512.scatter.qps.512 (i8*, i8, <8 x i64>, <8 x float>, i32)
declare <8 x double> @llvm.x86.avx512.gather.qpd.512 (<8 x double>, i8*, <8 x i64>, i8, i32)
declare void @llvm.x86.avx512.scatter.qpd.512 (i8*, i8, <8 x i64>, <8 x double>, i32)

define void @gather_mask_dps(<16 x i32> %ind, <16 x float> %src, i16 %mask, i8* %base, i8* %stbuf)  {
; CHECK-LABEL: gather_mask_dps:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovw %edi, %k1
; CHECK-NEXT:    kmovq %k1, %k2
; CHECK-NEXT:    vgatherdps (%rsi,%zmm0,4), %zmm1 {%k2}
; CHECK-NEXT:    vpaddd {{.*}}(%rip), %zmm0, %zmm0
; CHECK-NEXT:    vscatterdps %zmm1, (%rdx,%zmm0,4) {%k1}
; CHECK-NEXT:    retq
  %x = call <16 x float> @llvm.x86.avx512.gather.dps.512 (<16 x float> %src, i8* %base, <16 x i32>%ind, i16 %mask, i32 4)
  %ind2 = add <16 x i32> %ind, <i32 0, i32 1, i32 2, i32 3, i32 0, i32 1, i32 2, i32 3, i32 0, i32 1, i32 2, i32 3, i32 0, i32 1, i32 2, i32 3>
  call void @llvm.x86.avx512.scatter.dps.512 (i8* %stbuf, i16 %mask, <16 x i32>%ind2, <16 x float> %x, i32 4)
  ret void
}

define void @gather_mask_dpd(<8 x i32> %ind, <8 x double> %src, i8 %mask, i8* %base, i8* %stbuf)  {
; CHECK-LABEL: gather_mask_dpd:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %edi, %k1
; CHECK-NEXT:    kmovq %k1, %k2
; CHECK-NEXT:    vgatherdpd (%rsi,%ymm0,4), %zmm1 {%k2}
; CHECK-NEXT:    vpaddd {{.*}}(%rip), %ymm0, %ymm0
; CHECK-NEXT:    vscatterdpd %zmm1, (%rdx,%ymm0,4) {%k1}
; CHECK-NEXT:    retq
  %x = call <8 x double> @llvm.x86.avx512.gather.dpd.512 (<8 x double> %src, i8* %base, <8 x i32>%ind, i8 %mask, i32 4)
  %ind2 = add <8 x i32> %ind, <i32 0, i32 1, i32 2, i32 3, i32 0, i32 1, i32 2, i32 3>
  call void @llvm.x86.avx512.scatter.dpd.512 (i8* %stbuf, i8 %mask, <8 x i32>%ind2, <8 x double> %x, i32 4)
  ret void
}

define void @gather_mask_qps(<8 x i64> %ind, <8 x float> %src, i8 %mask, i8* %base, i8* %stbuf)  {
; CHECK-LABEL: gather_mask_qps:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %edi, %k1
; CHECK-NEXT:    kmovq %k1, %k2
; CHECK-NEXT:    vgatherqps (%rsi,%zmm0,4), %ymm1 {%k2}
; CHECK-NEXT:    vpaddq {{.*}}(%rip), %zmm0, %zmm0
; CHECK-NEXT:    vscatterqps %ymm1, (%rdx,%zmm0,4) {%k1}
; CHECK-NEXT:    retq
  %x = call <8 x float> @llvm.x86.avx512.gather.qps.512 (<8 x float> %src, i8* %base, <8 x i64>%ind, i8 %mask, i32 4)
  %ind2 = add <8 x i64> %ind, <i64 0, i64 1, i64 2, i64 3, i64 0, i64 1, i64 2, i64 3>
  call void @llvm.x86.avx512.scatter.qps.512 (i8* %stbuf, i8 %mask, <8 x i64>%ind2, <8 x float> %x, i32 4)
  ret void
}

define void @gather_mask_qpd(<8 x i64> %ind, <8 x double> %src, i8 %mask, i8* %base, i8* %stbuf)  {
; CHECK-LABEL: gather_mask_qpd:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %edi, %k1
; CHECK-NEXT:    kmovq %k1, %k2
; CHECK-NEXT:    vgatherqpd (%rsi,%zmm0,4), %zmm1 {%k2}
; CHECK-NEXT:    vpaddq {{.*}}(%rip), %zmm0, %zmm0
; CHECK-NEXT:    vscatterqpd %zmm1, (%rdx,%zmm0,4) {%k1}
; CHECK-NEXT:    retq
  %x = call <8 x double> @llvm.x86.avx512.gather.qpd.512 (<8 x double> %src, i8* %base, <8 x i64>%ind, i8 %mask, i32 4)
  %ind2 = add <8 x i64> %ind, <i64 0, i64 1, i64 2, i64 3, i64 0, i64 1, i64 2, i64 3>
  call void @llvm.x86.avx512.scatter.qpd.512 (i8* %stbuf, i8 %mask, <8 x i64>%ind2, <8 x double> %x, i32 4)
  ret void
}
;;
;; Integer Gather/Scatter
;;
declare <16 x i32> @llvm.x86.avx512.gather.dpi.512 (<16 x i32>, i8*, <16 x i32>, i16, i32)
declare void @llvm.x86.avx512.scatter.dpi.512 (i8*, i16, <16 x i32>, <16 x i32>, i32)
declare <8 x i64> @llvm.x86.avx512.gather.dpq.512 (<8 x i64>, i8*, <8 x i32>, i8, i32)
declare void @llvm.x86.avx512.scatter.dpq.512 (i8*, i8, <8 x i32>, <8 x i64>, i32)

declare <8 x i32> @llvm.x86.avx512.gather.qpi.512 (<8 x i32>, i8*, <8 x i64>, i8, i32)
declare void @llvm.x86.avx512.scatter.qpi.512 (i8*, i8, <8 x i64>, <8 x i32>, i32)
declare <8 x i64> @llvm.x86.avx512.gather.qpq.512 (<8 x i64>, i8*, <8 x i64>, i8, i32)
declare void @llvm.x86.avx512.scatter.qpq.512 (i8*, i8, <8 x i64>, <8 x i64>, i32)

define void @gather_mask_dd(<16 x i32> %ind, <16 x i32> %src, i16 %mask, i8* %base, i8* %stbuf)  {
; CHECK-LABEL: gather_mask_dd:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovw %edi, %k1
; CHECK-NEXT:    kmovq %k1, %k2
; CHECK-NEXT:    vpgatherdd (%rsi,%zmm0,4), %zmm1 {%k2}
; CHECK-NEXT:    vpaddd {{.*}}(%rip), %zmm0, %zmm0
; CHECK-NEXT:    vpscatterdd %zmm1, (%rdx,%zmm0,4) {%k1}
; CHECK-NEXT:    retq
  %x = call <16 x i32> @llvm.x86.avx512.gather.dpi.512 (<16 x i32> %src, i8* %base, <16 x i32>%ind, i16 %mask, i32 4)
  %ind2 = add <16 x i32> %ind, <i32 0, i32 1, i32 2, i32 3, i32 0, i32 1, i32 2, i32 3, i32 0, i32 1, i32 2, i32 3, i32 0, i32 1, i32 2, i32 3>
  call void @llvm.x86.avx512.scatter.dpi.512 (i8* %stbuf, i16 %mask, <16 x i32>%ind2, <16 x i32> %x, i32 4)
  ret void
}

define void @gather_mask_qd(<8 x i64> %ind, <8 x i32> %src, i8 %mask, i8* %base, i8* %stbuf)  {
; CHECK-LABEL: gather_mask_qd:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %edi, %k1
; CHECK-NEXT:    kmovq %k1, %k2
; CHECK-NEXT:    vpgatherqd (%rsi,%zmm0,4), %ymm1 {%k2}
; CHECK-NEXT:    vpaddq {{.*}}(%rip), %zmm0, %zmm0
; CHECK-NEXT:    vpscatterqd %ymm1, (%rdx,%zmm0,4) {%k1}
; CHECK-NEXT:    retq
  %x = call <8 x i32> @llvm.x86.avx512.gather.qpi.512 (<8 x i32> %src, i8* %base, <8 x i64>%ind, i8 %mask, i32 4)
  %ind2 = add <8 x i64> %ind, <i64 0, i64 1, i64 2, i64 3, i64 0, i64 1, i64 2, i64 3>
  call void @llvm.x86.avx512.scatter.qpi.512 (i8* %stbuf, i8 %mask, <8 x i64>%ind2, <8 x i32> %x, i32 4)
  ret void
}

define void @gather_mask_qq(<8 x i64> %ind, <8 x i64> %src, i8 %mask, i8* %base, i8* %stbuf)  {
; CHECK-LABEL: gather_mask_qq:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %edi, %k1
; CHECK-NEXT:    kmovq %k1, %k2
; CHECK-NEXT:    vpgatherqq (%rsi,%zmm0,4), %zmm1 {%k2}
; CHECK-NEXT:    vpaddq {{.*}}(%rip), %zmm0, %zmm0
; CHECK-NEXT:    vpscatterqq %zmm1, (%rdx,%zmm0,4) {%k1}
; CHECK-NEXT:    retq
  %x = call <8 x i64> @llvm.x86.avx512.gather.qpq.512 (<8 x i64> %src, i8* %base, <8 x i64>%ind, i8 %mask, i32 4)
  %ind2 = add <8 x i64> %ind, <i64 0, i64 1, i64 2, i64 3, i64 0, i64 1, i64 2, i64 3>
  call void @llvm.x86.avx512.scatter.qpq.512 (i8* %stbuf, i8 %mask, <8 x i64>%ind2, <8 x i64> %x, i32 4)
  ret void
}

define void @gather_mask_dq(<8 x i32> %ind, <8 x i64> %src, i8 %mask, i8* %base, i8* %stbuf)  {
; CHECK-LABEL: gather_mask_dq:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %edi, %k1
; CHECK-NEXT:    kmovq %k1, %k2
; CHECK-NEXT:    vpgatherdq (%rsi,%ymm0,4), %zmm1 {%k2}
; CHECK-NEXT:    vpaddd {{.*}}(%rip), %ymm0, %ymm0
; CHECK-NEXT:    vpscatterdq %zmm1, (%rdx,%ymm0,4) {%k1}
; CHECK-NEXT:    retq
  %x = call <8 x i64> @llvm.x86.avx512.gather.dpq.512 (<8 x i64> %src, i8* %base, <8 x i32>%ind, i8 %mask, i32 4)
  %ind2 = add <8 x i32> %ind, <i32 0, i32 1, i32 2, i32 3, i32 0, i32 1, i32 2, i32 3>
  call void @llvm.x86.avx512.scatter.dpq.512 (i8* %stbuf, i8 %mask, <8 x i32>%ind2, <8 x i64> %x, i32 4)
  ret void
}

define void @gather_mask_dpd_execdomain(<8 x i32> %ind, <8 x double> %src, i8 %mask, i8* %base, <8 x double>* %stbuf)  {
; CHECK-LABEL: gather_mask_dpd_execdomain:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %edi, %k1
; CHECK-NEXT:    vgatherdpd (%rsi,%ymm0,4), %zmm1 {%k1}
; CHECK-NEXT:    vmovapd %zmm1, (%rdx)
; CHECK-NEXT:    retq
  %x = call <8 x double> @llvm.x86.avx512.gather.dpd.512 (<8 x double> %src, i8* %base, <8 x i32>%ind, i8 %mask, i32 4)
  store <8 x double> %x, <8 x double>* %stbuf
  ret void
}

define void @gather_mask_qpd_execdomain(<8 x i64> %ind, <8 x double> %src, i8 %mask, i8* %base, <8 x double>* %stbuf)  {
; CHECK-LABEL: gather_mask_qpd_execdomain:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %edi, %k1
; CHECK-NEXT:    vgatherqpd (%rsi,%zmm0,4), %zmm1 {%k1}
; CHECK-NEXT:    vmovapd %zmm1, (%rdx)
; CHECK-NEXT:    retq
  %x = call <8 x double> @llvm.x86.avx512.gather.qpd.512 (<8 x double> %src, i8* %base, <8 x i64>%ind, i8 %mask, i32 4)
  store <8 x double> %x, <8 x double>* %stbuf
  ret void
}

define <16 x float> @gather_mask_dps_execdomain(<16 x i32> %ind, <16 x float> %src, i16 %mask, i8* %base)  {
; CHECK-LABEL: gather_mask_dps_execdomain:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovw %edi, %k1
; CHECK-NEXT:    vgatherdps (%rsi,%zmm0,4), %zmm1 {%k1}
; CHECK-NEXT:    vmovaps %zmm1, %zmm0
; CHECK-NEXT:    retq
  %res = call <16 x float> @llvm.x86.avx512.gather.dps.512 (<16 x float> %src, i8* %base, <16 x i32>%ind, i16 %mask, i32 4)
  ret <16 x float> %res;
}

define <8 x float> @gather_mask_qps_execdomain(<8 x i64> %ind, <8 x float> %src, i8 %mask, i8* %base)  {
; CHECK-LABEL: gather_mask_qps_execdomain:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %edi, %k1
; CHECK-NEXT:    vgatherqps (%rsi,%zmm0,4), %ymm1 {%k1}
; CHECK-NEXT:    vmovaps %zmm1, %zmm0
; CHECK-NEXT:    retq
  %res = call <8 x float> @llvm.x86.avx512.gather.qps.512 (<8 x float> %src, i8* %base, <8 x i64>%ind, i8 %mask, i32 4)
  ret <8 x float> %res;
}

define void @scatter_mask_dpd_execdomain(<8 x i32> %ind, <8 x double>* %src, i8 %mask, i8* %base, i8* %stbuf)  {
; CHECK-LABEL: scatter_mask_dpd_execdomain:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    vmovapd (%rdi), %zmm1
; CHECK-NEXT:    vscatterdpd %zmm1, (%rcx,%ymm0,4) {%k1}
; CHECK-NEXT:    retq
  %x = load <8 x double>, <8 x double>* %src, align 64
  call void @llvm.x86.avx512.scatter.dpd.512 (i8* %stbuf, i8 %mask, <8 x i32>%ind, <8 x double> %x, i32 4)
  ret void
}

define void @scatter_mask_qpd_execdomain(<8 x i64> %ind, <8 x double>* %src, i8 %mask, i8* %base, i8* %stbuf)  {
; CHECK-LABEL: scatter_mask_qpd_execdomain:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    vmovapd (%rdi), %zmm1
; CHECK-NEXT:    vscatterqpd %zmm1, (%rcx,%zmm0,4) {%k1}
; CHECK-NEXT:    retq
  %x = load <8 x double>, <8 x double>* %src, align 64
  call void @llvm.x86.avx512.scatter.qpd.512 (i8* %stbuf, i8 %mask, <8 x i64>%ind, <8 x double> %x, i32 4)
  ret void
}

define void @scatter_mask_dps_execdomain(<16 x i32> %ind, <16 x float>* %src, i16 %mask, i8* %base, i8* %stbuf)  {
; CHECK-LABEL: scatter_mask_dps_execdomain:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovw %esi, %k1
; CHECK-NEXT:    vmovaps (%rdi), %zmm1
; CHECK-NEXT:    vscatterdps %zmm1, (%rcx,%zmm0,4) {%k1}
; CHECK-NEXT:    retq
  %x = load <16 x float>, <16 x float>* %src, align 64
  call void @llvm.x86.avx512.scatter.dps.512 (i8* %stbuf, i16 %mask, <16 x i32>%ind, <16 x float> %x, i32 4)
  ret void
}

define void @scatter_mask_qps_execdomain(<8 x i64> %ind, <8 x float>* %src, i8 %mask, i8* %base, i8* %stbuf)  {
; CHECK-LABEL: scatter_mask_qps_execdomain:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    vmovaps (%rdi), %ymm1
; CHECK-NEXT:    vscatterqps %ymm1, (%rcx,%zmm0,4) {%k1}
; CHECK-NEXT:    retq
  %x = load <8 x float>, <8 x float>* %src, align 32
  call void @llvm.x86.avx512.scatter.qps.512 (i8* %stbuf, i8 %mask, <8 x i64>%ind, <8 x float> %x, i32 4)
  ret void
}

define void @gather_qps(<8 x i64> %ind, <8 x float> %src, i8* %base, i8* %stbuf)  {
; CHECK-LABEL: gather_qps:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kxnorw %k1, %k1, %k1
; CHECK-NEXT:    kxnorw %k2, %k2, %k2
; CHECK-NEXT:    vgatherqps (%rdi,%zmm0,4), %ymm1 {%k2}
; CHECK-NEXT:    vpaddq {{.*}}(%rip), %zmm0, %zmm0
; CHECK-NEXT:    vscatterqps %ymm1, (%rsi,%zmm0,4) {%k1}
; CHECK-NEXT:    retq
  %x = call <8 x float> @llvm.x86.avx512.gather.qps.512 (<8 x float> %src, i8* %base, <8 x i64>%ind, i8 -1, i32 4)
  %ind2 = add <8 x i64> %ind, <i64 0, i64 1, i64 2, i64 3, i64 0, i64 1, i64 2, i64 3>
  call void @llvm.x86.avx512.scatter.qps.512 (i8* %stbuf, i8 -1, <8 x i64>%ind2, <8 x float> %x, i32 4)
  ret void
}

declare  void @llvm.x86.avx512.gatherpf.qps.512(i8, <8 x i64>, i8* , i32, i32);
declare  void @llvm.x86.avx512.scatterpf.qps.512(i8, <8 x i64>, i8* , i32, i32);
define void @prefetch(<8 x i64> %ind, i8* %base) {
; CHECK-LABEL: prefetch:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kxnorw %k1, %k1, %k1
; CHECK-NEXT:    vgatherpf0qps (%rdi,%zmm0,4) {%k1}
; CHECK-NEXT:    vgatherpf1qps (%rdi,%zmm0,4) {%k1}
; CHECK-NEXT:    vscatterpf0qps (%rdi,%zmm0,2) {%k1}
; CHECK-NEXT:    vscatterpf1qps (%rdi,%zmm0,2) {%k1}
; CHECK-NEXT:    retq
  call void @llvm.x86.avx512.gatherpf.qps.512(i8 -1, <8 x i64> %ind, i8* %base, i32 4, i32 0)
  call void @llvm.x86.avx512.gatherpf.qps.512(i8 -1, <8 x i64> %ind, i8* %base, i32 4, i32 1)
  call void @llvm.x86.avx512.scatterpf.qps.512(i8 -1, <8 x i64> %ind, i8* %base, i32 2, i32 0)
  call void @llvm.x86.avx512.scatterpf.qps.512(i8 -1, <8 x i64> %ind, i8* %base, i32 2, i32 1)
  ret void
}


declare <2 x double> @llvm.x86.avx512.gather3div2.df(<2 x double>, i8*, <2 x i64>, i8, i32)

define <2 x double>@test_int_x86_avx512_gather3div2_df(<2 x double> %x0, i8* %x1, <2 x i64> %x2, i8 %x3) {
; CHECK-LABEL: test_int_x86_avx512_gather3div2_df:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    vmovaps %zmm0, %zmm2
; CHECK-NEXT:    vgatherqpd (%rdi,%xmm1,4), %xmm2 {%k1}
; CHECK-NEXT:    kxnorw %k1, %k1, %k1
; CHECK-NEXT:    vgatherqpd (%rdi,%xmm1,2), %xmm0 {%k1}
; CHECK-NEXT:    vaddpd %xmm0, %xmm2, %xmm0
; CHECK-NEXT:    retq
  %res = call <2 x double> @llvm.x86.avx512.gather3div2.df(<2 x double> %x0, i8* %x1, <2 x i64> %x2, i8 %x3, i32 4)
  %res1 = call <2 x double> @llvm.x86.avx512.gather3div2.df(<2 x double> %x0, i8* %x1, <2 x i64> %x2, i8 -1, i32 2)
  %res2 = fadd <2 x double> %res, %res1
  ret <2 x double> %res2
}

declare <4 x i32> @llvm.x86.avx512.gather3div2.di(<2 x i64>, i8*, <2 x i64>, i8, i32)

define <4 x i32>@test_int_x86_avx512_gather3div2_di(<2 x i64> %x0, i8* %x1, <2 x i64> %x2, i8 %x3) {
; CHECK-LABEL: test_int_x86_avx512_gather3div2_di:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    vpgatherqq (%rdi,%xmm1,8), %xmm0 {%k1}
; CHECK-NEXT:    vpaddd %xmm0, %xmm0, %xmm0
; CHECK-NEXT:    retq
  %res = call <4 x i32> @llvm.x86.avx512.gather3div2.di(<2 x i64> %x0, i8* %x1, <2 x i64> %x2, i8 %x3, i32 8)
  %res1 = call <4 x i32> @llvm.x86.avx512.gather3div2.di(<2 x i64> %x0, i8* %x1, <2 x i64> %x2, i8 %x3, i32 8)
  %res2 = add <4 x i32> %res, %res1
  ret <4 x i32> %res2
}

declare <4 x double> @llvm.x86.avx512.gather3div4.df(<4 x double>, i8*, <4 x i64>, i8, i32)

define <4 x double>@test_int_x86_avx512_gather3div4_df(<4 x double> %x0, i8* %x1, <4 x i64> %x2, i8 %x3) {
; CHECK-LABEL: test_int_x86_avx512_gather3div4_df:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    vmovaps %zmm0, %zmm2
; CHECK-NEXT:    vgatherqpd (%rdi,%ymm1,4), %ymm2 {%k1}
; CHECK-NEXT:    kxnorw %k1, %k1, %k1
; CHECK-NEXT:    vgatherqpd (%rdi,%ymm1,2), %ymm0 {%k1}
; CHECK-NEXT:    vaddpd %ymm0, %ymm2, %ymm0
; CHECK-NEXT:    retq
  %res = call <4 x double> @llvm.x86.avx512.gather3div4.df(<4 x double> %x0, i8* %x1, <4 x i64> %x2, i8 %x3, i32 4)
  %res1 = call <4 x double> @llvm.x86.avx512.gather3div4.df(<4 x double> %x0, i8* %x1, <4 x i64> %x2, i8 -1, i32 2)
  %res2 = fadd <4 x double> %res, %res1
  ret <4 x double> %res2
}

declare <8 x i32> @llvm.x86.avx512.gather3div4.di(<4 x i64>, i8*, <4 x i64>, i8, i32)

define <8 x i32>@test_int_x86_avx512_gather3div4_di(<4 x i64> %x0, i8* %x1, <4 x i64> %x2, i8 %x3) {
; CHECK-LABEL: test_int_x86_avx512_gather3div4_di:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    vmovaps %zmm0, %zmm2
; CHECK-NEXT:    vpgatherqq (%rdi,%ymm1,8), %ymm2 {%k1}
; CHECK-NEXT:    kxnorw %k1, %k1, %k1
; CHECK-NEXT:    vpgatherqq (%rdi,%ymm1,8), %ymm0 {%k1}
; CHECK-NEXT:    vpaddd %ymm0, %ymm2, %ymm0
; CHECK-NEXT:    retq
  %res = call <8 x i32> @llvm.x86.avx512.gather3div4.di(<4 x i64> %x0, i8* %x1, <4 x i64> %x2, i8 %x3, i32 8)
  %res1 = call <8 x i32> @llvm.x86.avx512.gather3div4.di(<4 x i64> %x0, i8* %x1, <4 x i64> %x2, i8 -1, i32 8)
  %res2 = add <8 x i32> %res, %res1
  ret <8 x i32> %res2
}

declare <4 x float> @llvm.x86.avx512.gather3div4.sf(<4 x float>, i8*, <2 x i64>, i8, i32)

define <4 x float>@test_int_x86_avx512_gather3div4_sf(<4 x float> %x0, i8* %x1, <2 x i64> %x2, i8 %x3) {
; CHECK-LABEL: test_int_x86_avx512_gather3div4_sf:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    vmovaps %zmm0, %zmm2
; CHECK-NEXT:    vgatherqps (%rdi,%xmm1,4), %xmm2 {%k1}
; CHECK-NEXT:    kxnorw %k1, %k1, %k1
; CHECK-NEXT:    vgatherqps (%rdi,%xmm1,2), %xmm0 {%k1}
; CHECK-NEXT:    vaddps %xmm0, %xmm2, %xmm0
; CHECK-NEXT:    retq
  %res = call <4 x float> @llvm.x86.avx512.gather3div4.sf(<4 x float> %x0, i8* %x1, <2 x i64> %x2, i8 %x3, i32 4)
  %res1 = call <4 x float> @llvm.x86.avx512.gather3div4.sf(<4 x float> %x0, i8* %x1, <2 x i64> %x2, i8 -1, i32 2)
  %res2 = fadd <4 x float> %res, %res1
  ret <4 x float> %res2
}

declare <4 x i32> @llvm.x86.avx512.gather3div4.si(<4 x i32>, i8*, <2 x i64>, i8, i32)

define <4 x i32>@test_int_x86_avx512_gather3div4_si(<4 x i32> %x0, i8* %x1, <2 x i64> %x2, i8 %x3) {
; CHECK-LABEL: test_int_x86_avx512_gather3div4_si:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    kxnorw %k2, %k2, %k2
; CHECK-NEXT:    vmovaps %zmm0, %zmm2
; CHECK-NEXT:    vpgatherqd (%rdi,%xmm1,4), %xmm2 {%k2}
; CHECK-NEXT:    vpgatherqd (%rdi,%xmm1,4), %xmm0 {%k1}
; CHECK-NEXT:    vpaddd %xmm0, %xmm2, %xmm0
; CHECK-NEXT:    retq
  %res = call <4 x i32> @llvm.x86.avx512.gather3div4.si(<4 x i32> %x0, i8* %x1, <2 x i64> %x2, i8 -1, i32 4)
  %res1 = call <4 x i32> @llvm.x86.avx512.gather3div4.si(<4 x i32> %x0, i8* %x1, <2 x i64> %x2, i8 %x3, i32 4)
  %res2 = add <4 x i32> %res, %res1
  ret <4 x i32> %res2
}

declare <4 x float> @llvm.x86.avx512.gather3div8.sf(<4 x float>, i8*, <4 x i64>, i8, i32)

define <4 x float>@test_int_x86_avx512_gather3div8_sf(<4 x float> %x0, i8* %x1, <4 x i64> %x2, i8 %x3) {
; CHECK-LABEL: test_int_x86_avx512_gather3div8_sf:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    vmovaps %zmm0, %zmm2
; CHECK-NEXT:    vgatherqps (%rdi,%ymm1,4), %xmm2 {%k1}
; CHECK-NEXT:    kxnorw %k1, %k1, %k1
; CHECK-NEXT:    vgatherqps (%rdi,%ymm1,2), %xmm0 {%k1}
; CHECK-NEXT:    vaddps %xmm0, %xmm2, %xmm0
; CHECK-NEXT:    retq
  %res = call <4 x float> @llvm.x86.avx512.gather3div8.sf(<4 x float> %x0, i8* %x1, <4 x i64> %x2, i8 %x3, i32 4)
  %res1 = call <4 x float> @llvm.x86.avx512.gather3div8.sf(<4 x float> %x0, i8* %x1, <4 x i64> %x2, i8 -1, i32 2)
  %res2 = fadd <4 x float> %res, %res1
  ret <4 x float> %res2
}

declare <4 x i32> @llvm.x86.avx512.gather3div8.si(<4 x i32>, i8*, <4 x i64>, i8, i32)

define <4 x i32>@test_int_x86_avx512_gather3div8_si(<4 x i32> %x0, i8* %x1, <4 x i64> %x2, i8 %x3) {
; CHECK-LABEL: test_int_x86_avx512_gather3div8_si:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    vmovaps %zmm0, %zmm2
; CHECK-NEXT:    kmovq %k1, %k2
; CHECK-NEXT:    vpgatherqd (%rdi,%ymm1,4), %xmm2 {%k2}
; CHECK-NEXT:    vpgatherqd (%rdi,%ymm1,2), %xmm0 {%k1}
; CHECK-NEXT:    vpaddd %xmm0, %xmm2, %xmm0
; CHECK-NEXT:    retq
  %res = call <4 x i32> @llvm.x86.avx512.gather3div8.si(<4 x i32> %x0, i8* %x1, <4 x i64> %x2, i8 %x3, i32 4)
  %res1 = call <4 x i32> @llvm.x86.avx512.gather3div8.si(<4 x i32> %x0, i8* %x1, <4 x i64> %x2, i8 %x3, i32 2)
  %res2 = add <4 x i32> %res, %res1
  ret <4 x i32> %res2
}

declare <2 x double> @llvm.x86.avx512.gather3siv2.df(<2 x double>, i8*, <4 x i32>, i8, i32)

define <2 x double>@test_int_x86_avx512_gather3siv2_df(<2 x double> %x0, i8* %x1, <4 x i32> %x2, i8 %x3) {
; CHECK-LABEL: test_int_x86_avx512_gather3siv2_df:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    vmovaps %zmm0, %zmm2
; CHECK-NEXT:    vgatherdpd (%rdi,%xmm1,4), %xmm2 {%k1}
; CHECK-NEXT:    kxnorw %k1, %k1, %k1
; CHECK-NEXT:    vgatherdpd (%rdi,%xmm1,2), %xmm0 {%k1}
; CHECK-NEXT:    vaddpd %xmm0, %xmm2, %xmm0
; CHECK-NEXT:    retq
  %res = call <2 x double> @llvm.x86.avx512.gather3siv2.df(<2 x double> %x0, i8* %x1, <4 x i32> %x2, i8 %x3, i32 4)
  %res1 = call <2 x double> @llvm.x86.avx512.gather3siv2.df(<2 x double> %x0, i8* %x1, <4 x i32> %x2, i8 -1, i32 2)
  %res2 = fadd <2 x double> %res, %res1
  ret <2 x double> %res2
}

declare <4 x i32> @llvm.x86.avx512.gather3siv2.di(<2 x i64>, i8*, <4 x i32>, i8, i32)

define <4 x i32>@test_int_x86_avx512_gather3siv2_di(<2 x i64> %x0, i8* %x1, <4 x i32> %x2, i8 %x3) {
; CHECK-LABEL: test_int_x86_avx512_gather3siv2_di:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    vpgatherdq (%rdi,%xmm1,8), %xmm0 {%k1}
; CHECK-NEXT:    vpaddd %xmm0, %xmm0, %xmm0
; CHECK-NEXT:    retq
  %res = call <4 x i32> @llvm.x86.avx512.gather3siv2.di(<2 x i64> %x0, i8* %x1, <4 x i32> %x2, i8 %x3, i32 8)
  %res1 = call <4 x i32> @llvm.x86.avx512.gather3siv2.di(<2 x i64> %x0, i8* %x1, <4 x i32> %x2, i8 %x3, i32 8)
  %res2 = add <4 x i32> %res, %res1
  ret <4 x i32> %res2
}

declare <4 x double> @llvm.x86.avx512.gather3siv4.df(<4 x double>, i8*, <4 x i32>, i8, i32)

define <4 x double>@test_int_x86_avx512_gather3siv4_df(<4 x double> %x0, i8* %x1, <4 x i32> %x2, i8 %x3) {
; CHECK-LABEL: test_int_x86_avx512_gather3siv4_df:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    vmovaps %zmm0, %zmm2
; CHECK-NEXT:    vgatherdpd (%rdi,%xmm1,4), %ymm2 {%k1}
; CHECK-NEXT:    kxnorw %k1, %k1, %k1
; CHECK-NEXT:    vgatherdpd (%rdi,%xmm1,2), %ymm0 {%k1}
; CHECK-NEXT:    vaddpd %ymm0, %ymm2, %ymm0
; CHECK-NEXT:    retq
  %res = call <4 x double> @llvm.x86.avx512.gather3siv4.df(<4 x double> %x0, i8* %x1, <4 x i32> %x2, i8 %x3, i32 4)
  %res1 = call <4 x double> @llvm.x86.avx512.gather3siv4.df(<4 x double> %x0, i8* %x1, <4 x i32> %x2, i8 -1, i32 2)
  %res2 = fadd <4 x double> %res, %res1
  ret <4 x double> %res2
}

declare <8 x i32> @llvm.x86.avx512.gather3siv4.di(<4 x i64>, i8*, <4 x i32>, i8, i32)

define <8 x i32>@test_int_x86_avx512_gather3siv4_di(<4 x i64> %x0, i8* %x1, <4 x i32> %x2, i8 %x3) {
; CHECK-LABEL: test_int_x86_avx512_gather3siv4_di:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    vpgatherdq (%rdi,%xmm1,8), %ymm0 {%k1}
; CHECK-NEXT:    vpaddd %ymm0, %ymm0, %ymm0
; CHECK-NEXT:    retq
  %res = call <8 x i32> @llvm.x86.avx512.gather3siv4.di(<4 x i64> %x0, i8* %x1, <4 x i32> %x2, i8 %x3, i32 8)
  %res1 = call <8 x i32> @llvm.x86.avx512.gather3siv4.di(<4 x i64> %x0, i8* %x1, <4 x i32> %x2, i8 %x3, i32 8)
  %res2 = add <8 x i32> %res, %res1
  ret <8 x i32> %res2
}

declare <4 x float> @llvm.x86.avx512.gather3siv4.sf(<4 x float>, i8*, <4 x i32>, i8, i32)

define <4 x float>@test_int_x86_avx512_gather3siv4_sf(<4 x float> %x0, i8* %x1, <4 x i32> %x2, i8 %x3) {
; CHECK-LABEL: test_int_x86_avx512_gather3siv4_sf:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    vmovaps %zmm0, %zmm2
; CHECK-NEXT:    vgatherdps (%rdi,%xmm1,4), %xmm2 {%k1}
; CHECK-NEXT:    kxnorw %k1, %k1, %k1
; CHECK-NEXT:    vgatherdps (%rdi,%xmm1,2), %xmm0 {%k1}
; CHECK-NEXT:    vaddps %xmm0, %xmm2, %xmm0
; CHECK-NEXT:    retq
  %res = call <4 x float> @llvm.x86.avx512.gather3siv4.sf(<4 x float> %x0, i8* %x1, <4 x i32> %x2, i8 %x3, i32 4)
  %res1 = call <4 x float> @llvm.x86.avx512.gather3siv4.sf(<4 x float> %x0, i8* %x1, <4 x i32> %x2, i8 -1, i32 2)
  %res2 = fadd <4 x float> %res, %res1
  ret <4 x float> %res2
}

declare <4 x i32> @llvm.x86.avx512.gather3siv4.si(<4 x i32>, i8*, <4 x i32>, i8, i32)

define <4 x i32>@test_int_x86_avx512_gather3siv4_si(<4 x i32> %x0, i8* %x1, <4 x i32> %x2, i8 %x3) {
; CHECK-LABEL: test_int_x86_avx512_gather3siv4_si:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    kxnorw %k2, %k2, %k2
; CHECK-NEXT:    vmovaps %zmm0, %zmm2
; CHECK-NEXT:    vpgatherdd (%rdi,%xmm1,4), %xmm2 {%k2}
; CHECK-NEXT:    vpgatherdd (%rdi,%xmm1,2), %xmm0 {%k1}
; CHECK-NEXT:    vpaddd %xmm0, %xmm2, %xmm0
; CHECK-NEXT:    retq
  %res = call <4 x i32> @llvm.x86.avx512.gather3siv4.si(<4 x i32> %x0, i8* %x1, <4 x i32> %x2, i8 -1, i32 4)
  %res1 = call <4 x i32> @llvm.x86.avx512.gather3siv4.si(<4 x i32> %x0, i8* %x1, <4 x i32> %x2, i8 %x3, i32 2)
  %res2 = add <4 x i32> %res, %res1
  ret <4 x i32> %res2
}

declare <8 x float> @llvm.x86.avx512.gather3siv8.sf(<8 x float>, i8*, <8 x i32>, i8, i32)

define <8 x float>@test_int_x86_avx512_gather3siv8_sf(<8 x float> %x0, i8* %x1, <8 x i32> %x2, i8 %x3) {
; CHECK-LABEL: test_int_x86_avx512_gather3siv8_sf:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    vmovaps %zmm0, %zmm2
; CHECK-NEXT:    vgatherdps (%rdi,%ymm1,4), %ymm2 {%k1}
; CHECK-NEXT:    kxnorw %k1, %k1, %k1
; CHECK-NEXT:    vgatherdps (%rdi,%ymm1,2), %ymm0 {%k1}
; CHECK-NEXT:    vaddps %ymm0, %ymm2, %ymm0
; CHECK-NEXT:    retq
  %res = call <8 x float> @llvm.x86.avx512.gather3siv8.sf(<8 x float> %x0, i8* %x1, <8 x i32> %x2, i8 %x3, i32 4)
  %res1 = call <8 x float> @llvm.x86.avx512.gather3siv8.sf(<8 x float> %x0, i8* %x1, <8 x i32> %x2, i8 -1, i32 2)
  %res2 = fadd <8 x float> %res, %res1
  ret <8 x float> %res2
}

declare <8 x i32> @llvm.x86.avx512.gather3siv8.si(<8 x i32>, i8*, <8 x i32>, i8, i32)

define <8 x i32>@test_int_x86_avx512_gather3siv8_si(<8 x i32> %x0, i8* %x1, <8 x i32> %x2, i8 %x3) {
; CHECK-LABEL: test_int_x86_avx512_gather3siv8_si:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    vmovaps %zmm0, %zmm2
; CHECK-NEXT:    kmovq %k1, %k2
; CHECK-NEXT:    vpgatherdd (%rdi,%ymm1,4), %ymm2 {%k2}
; CHECK-NEXT:    vpgatherdd (%rdi,%ymm1,2), %ymm0 {%k1}
; CHECK-NEXT:    vpaddd %ymm0, %ymm2, %ymm0
; CHECK-NEXT:    retq
  %res = call <8 x i32> @llvm.x86.avx512.gather3siv8.si(<8 x i32> %x0, i8* %x1, <8 x i32> %x2, i8 %x3, i32 4)
  %res1 = call <8 x i32> @llvm.x86.avx512.gather3siv8.si(<8 x i32> %x0, i8* %x1, <8 x i32> %x2, i8 %x3, i32 2)
  %res2 = add <8 x i32> %res, %res1
  ret <8 x i32> %res2
}

declare void @llvm.x86.avx512.scatterdiv2.df(i8*, i8, <2 x i64>, <2 x double>, i32)

define void@test_int_x86_avx512_scatterdiv2_df(i8* %x0, i8 %x1, <2 x i64> %x2, <2 x double> %x3) {
; CHECK-LABEL: test_int_x86_avx512_scatterdiv2_df:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    kxnorw %k2, %k2, %k2
; CHECK-NEXT:    vscatterqpd %xmm1, (%rdi,%xmm0,2) {%k2}
; CHECK-NEXT:    vscatterqpd %xmm1, (%rdi,%xmm0,4) {%k1}
; CHECK-NEXT:    retq
  call void @llvm.x86.avx512.scatterdiv2.df(i8* %x0, i8 -1, <2 x i64> %x2, <2 x double> %x3, i32 2)
  call void @llvm.x86.avx512.scatterdiv2.df(i8* %x0, i8 %x1, <2 x i64> %x2, <2 x double> %x3, i32 4)
  ret void
}

declare void @llvm.x86.avx512.scatterdiv2.di(i8*, i8, <2 x i64>, <2 x i64>, i32)

define void@test_int_x86_avx512_scatterdiv2_di(i8* %x0, i8 %x1, <2 x i64> %x2, <2 x i64> %x3) {
; CHECK-LABEL: test_int_x86_avx512_scatterdiv2_di:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    vpscatterqq %xmm1, (%rdi,%xmm0,2) {%k1}
; CHECK-NEXT:    kxnorw %k1, %k1, %k1
; CHECK-NEXT:    vpscatterqq %xmm1, (%rdi,%xmm0,4) {%k1}
; CHECK-NEXT:    retq
  call void @llvm.x86.avx512.scatterdiv2.di(i8* %x0, i8 %x1, <2 x i64> %x2, <2 x i64> %x3, i32 2)
  call void @llvm.x86.avx512.scatterdiv2.di(i8* %x0, i8 -1, <2 x i64> %x2, <2 x i64> %x3, i32 4)
  ret void
}

declare void @llvm.x86.avx512.scatterdiv4.df(i8*, i8, <4 x i64>, <4 x double>, i32)

define void@test_int_x86_avx512_scatterdiv4_df(i8* %x0, i8 %x1, <4 x i64> %x2, <4 x double> %x3) {
; CHECK-LABEL: test_int_x86_avx512_scatterdiv4_df:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    vscatterqpd %ymm1, (%rdi,%ymm0,2) {%k1}
; CHECK-NEXT:    kxnorw %k1, %k1, %k1
; CHECK-NEXT:    vscatterqpd %ymm1, (%rdi,%ymm0,4) {%k1}
; CHECK-NEXT:    retq
  call void @llvm.x86.avx512.scatterdiv4.df(i8* %x0, i8 %x1, <4 x i64> %x2, <4 x double> %x3, i32 2)
  call void @llvm.x86.avx512.scatterdiv4.df(i8* %x0, i8 -1, <4 x i64> %x2, <4 x double> %x3, i32 4)
  ret void
}

declare void @llvm.x86.avx512.scatterdiv4.di(i8*, i8, <4 x i64>, <4 x i64>, i32)

define void@test_int_x86_avx512_scatterdiv4_di(i8* %x0, i8 %x1, <4 x i64> %x2, <4 x i64> %x3) {
; CHECK-LABEL: test_int_x86_avx512_scatterdiv4_di:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    vpscatterqq %ymm1, (%rdi,%ymm0,2) {%k1}
; CHECK-NEXT:    kxnorw %k1, %k1, %k1
; CHECK-NEXT:    vpscatterqq %ymm1, (%rdi,%ymm0,4) {%k1}
; CHECK-NEXT:    retq
  call void @llvm.x86.avx512.scatterdiv4.di(i8* %x0, i8 %x1, <4 x i64> %x2, <4 x i64> %x3, i32 2)
  call void @llvm.x86.avx512.scatterdiv4.di(i8* %x0, i8 -1, <4 x i64> %x2, <4 x i64> %x3, i32 4)
  ret void
}

declare void @llvm.x86.avx512.scatterdiv4.sf(i8*, i8, <2 x i64>, <4 x float>, i32)

define void@test_int_x86_avx512_scatterdiv4_sf(i8* %x0, i8 %x1, <2 x i64> %x2, <4 x float> %x3) {
; CHECK-LABEL: test_int_x86_avx512_scatterdiv4_sf:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    vscatterqps %xmm1, (%rdi,%xmm0,2) {%k1}
; CHECK-NEXT:    kxnorw %k1, %k1, %k1
; CHECK-NEXT:    vscatterqps %xmm1, (%rdi,%xmm0,4) {%k1}
; CHECK-NEXT:    retq
  call void @llvm.x86.avx512.scatterdiv4.sf(i8* %x0, i8 %x1, <2 x i64> %x2, <4 x float> %x3, i32 2)
  call void @llvm.x86.avx512.scatterdiv4.sf(i8* %x0, i8 -1, <2 x i64> %x2, <4 x float> %x3, i32 4)
  ret void
}

declare void @llvm.x86.avx512.scatterdiv4.si(i8*, i8, <2 x i64>, <4 x i32>, i32)

define void@test_int_x86_avx512_scatterdiv4_si(i8* %x0, i8 %x1, <2 x i64> %x2, <4 x i32> %x3) {
; CHECK-LABEL: test_int_x86_avx512_scatterdiv4_si:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    kxnorw %k2, %k2, %k2
; CHECK-NEXT:    vpscatterqd %xmm1, (%rdi,%xmm0,2) {%k2}
; CHECK-NEXT:    vpscatterqd %xmm1, (%rdi,%xmm0,4) {%k1}
; CHECK-NEXT:    retq
  call void @llvm.x86.avx512.scatterdiv4.si(i8* %x0, i8 -1, <2 x i64> %x2, <4 x i32> %x3, i32 2)
  call void @llvm.x86.avx512.scatterdiv4.si(i8* %x0, i8 %x1, <2 x i64> %x2, <4 x i32> %x3, i32 4)
  ret void
}

declare void @llvm.x86.avx512.scatterdiv8.sf(i8*, i8, <4 x i64>, <4 x float>, i32)

define void@test_int_x86_avx512_scatterdiv8_sf(i8* %x0, i8 %x1, <4 x i64> %x2, <4 x float> %x3) {
; CHECK-LABEL: test_int_x86_avx512_scatterdiv8_sf:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    vscatterqps %xmm1, (%rdi,%ymm0,2) {%k1}
; CHECK-NEXT:    kxnorw %k1, %k1, %k1
; CHECK-NEXT:    vscatterqps %xmm1, (%rdi,%ymm0,4) {%k1}
; CHECK-NEXT:    retq
  call void @llvm.x86.avx512.scatterdiv8.sf(i8* %x0, i8 %x1, <4 x i64> %x2, <4 x float> %x3, i32 2)
  call void @llvm.x86.avx512.scatterdiv8.sf(i8* %x0, i8 -1, <4 x i64> %x2, <4 x float> %x3, i32 4)
  ret void
}

declare void @llvm.x86.avx512.scatterdiv8.si(i8*, i8, <4 x i64>, <4 x i32>, i32)

define void@test_int_x86_avx512_scatterdiv8_si(i8* %x0, i8 %x1, <4 x i64> %x2, <4 x i32> %x3) {
; CHECK-LABEL: test_int_x86_avx512_scatterdiv8_si:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    vpscatterqd %xmm1, (%rdi,%ymm0,2) {%k1}
; CHECK-NEXT:    kxnorw %k1, %k1, %k1
; CHECK-NEXT:    vpscatterqd %xmm1, (%rdi,%ymm0,4) {%k1}
; CHECK-NEXT:    retq
  call void @llvm.x86.avx512.scatterdiv8.si(i8* %x0, i8 %x1, <4 x i64> %x2, <4 x i32> %x3, i32 2)
  call void @llvm.x86.avx512.scatterdiv8.si(i8* %x0, i8 -1, <4 x i64> %x2, <4 x i32> %x3, i32 4)
  ret void
}

declare void @llvm.x86.avx512.scattersiv2.df(i8*, i8, <4 x i32>, <2 x double>, i32)

define void@test_int_x86_avx512_scattersiv2_df(i8* %x0, i8 %x1, <4 x i32> %x2, <2 x double> %x3) {
; CHECK-LABEL: test_int_x86_avx512_scattersiv2_df:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    kxnorw %k2, %k2, %k2
; CHECK-NEXT:    vscatterdpd %xmm1, (%rdi,%xmm0,2) {%k2}
; CHECK-NEXT:    vscatterdpd %xmm1, (%rdi,%xmm0,4) {%k1}
; CHECK-NEXT:    retq
  call void @llvm.x86.avx512.scattersiv2.df(i8* %x0, i8 -1, <4 x i32> %x2, <2 x double> %x3, i32 2)
  call void @llvm.x86.avx512.scattersiv2.df(i8* %x0, i8 %x1, <4 x i32> %x2, <2 x double> %x3, i32 4)
  ret void
}

declare void @llvm.x86.avx512.scattersiv2.di(i8*, i8, <4 x i32>, <2 x i64>, i32)

define void@test_int_x86_avx512_scattersiv2_di(i8* %x0, i8 %x1, <4 x i32> %x2, <2 x i64> %x3) {
; CHECK-LABEL: test_int_x86_avx512_scattersiv2_di:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    kxnorw %k2, %k2, %k2
; CHECK-NEXT:    vpscatterdq %xmm1, (%rdi,%xmm0,2) {%k2}
; CHECK-NEXT:    vpscatterdq %xmm1, (%rdi,%xmm0,4) {%k1}
; CHECK-NEXT:    retq
  call void @llvm.x86.avx512.scattersiv2.di(i8* %x0, i8 -1, <4 x i32> %x2, <2 x i64> %x3, i32 2)
  call void @llvm.x86.avx512.scattersiv2.di(i8* %x0, i8 %x1, <4 x i32> %x2, <2 x i64> %x3, i32 4)
  ret void
}

declare void @llvm.x86.avx512.scattersiv4.df(i8*, i8, <4 x i32>, <4 x double>, i32)

define void@test_int_x86_avx512_scattersiv4_df(i8* %x0, i8 %x1, <4 x i32> %x2, <4 x double> %x3) {
; CHECK-LABEL: test_int_x86_avx512_scattersiv4_df:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    vscatterdpd %ymm1, (%rdi,%xmm0,2) {%k1}
; CHECK-NEXT:    kxnorw %k1, %k1, %k1
; CHECK-NEXT:    vscatterdpd %ymm1, (%rdi,%xmm0,4) {%k1}
; CHECK-NEXT:    retq
  call void @llvm.x86.avx512.scattersiv4.df(i8* %x0, i8 %x1, <4 x i32> %x2, <4 x double> %x3, i32 2)
  call void @llvm.x86.avx512.scattersiv4.df(i8* %x0, i8 -1, <4 x i32> %x2, <4 x double> %x3, i32 4)
  ret void
}

declare void @llvm.x86.avx512.scattersiv4.di(i8*, i8, <4 x i32>, <4 x i64>, i32)

define void@test_int_x86_avx512_scattersiv4_di(i8* %x0, i8 %x1, <4 x i32> %x2, <4 x i64> %x3) {
; CHECK-LABEL: test_int_x86_avx512_scattersiv4_di:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    kxnorw %k2, %k2, %k2
; CHECK-NEXT:    vpscatterdq %ymm1, (%rdi,%xmm0,2) {%k2}
; CHECK-NEXT:    vpscatterdq %ymm1, (%rdi,%xmm0,4) {%k1}
; CHECK-NEXT:    retq
  call void @llvm.x86.avx512.scattersiv4.di(i8* %x0, i8 -1, <4 x i32> %x2, <4 x i64> %x3, i32 2)
  call void @llvm.x86.avx512.scattersiv4.di(i8* %x0, i8 %x1, <4 x i32> %x2, <4 x i64> %x3, i32 4)
  ret void
}

declare void @llvm.x86.avx512.scattersiv4.sf(i8*, i8, <4 x i32>, <4 x float>, i32)

define void@test_int_x86_avx512_scattersiv4_sf(i8* %x0, i8 %x1, <4 x i32> %x2, <4 x float> %x3) {
; CHECK-LABEL: test_int_x86_avx512_scattersiv4_sf:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    vscatterdps %xmm1, (%rdi,%xmm0,2) {%k1}
; CHECK-NEXT:    kxnorw %k1, %k1, %k1
; CHECK-NEXT:    vscatterdps %xmm1, (%rdi,%xmm0,4) {%k1}
; CHECK-NEXT:    retq
  call void @llvm.x86.avx512.scattersiv4.sf(i8* %x0, i8 %x1, <4 x i32> %x2, <4 x float> %x3, i32 2)
  call void @llvm.x86.avx512.scattersiv4.sf(i8* %x0, i8 -1, <4 x i32> %x2, <4 x float> %x3, i32 4)
  ret void
}

declare void @llvm.x86.avx512.scattersiv4.si(i8*, i8, <4 x i32>, <4 x i32>, i32)

define void@test_int_x86_avx512_scattersiv4_si(i8* %x0, i8 %x1, <4 x i32> %x2, <4 x i32> %x3) {
; CHECK-LABEL: test_int_x86_avx512_scattersiv4_si:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    vpscatterdd %xmm1, (%rdi,%xmm0,2) {%k1}
; CHECK-NEXT:    kxnorw %k1, %k1, %k1
; CHECK-NEXT:    vpscatterdd %xmm1, (%rdi,%xmm0,4) {%k1}
; CHECK-NEXT:    retq
  call void @llvm.x86.avx512.scattersiv4.si(i8* %x0, i8 %x1, <4 x i32> %x2, <4 x i32> %x3, i32 2)
  call void @llvm.x86.avx512.scattersiv4.si(i8* %x0, i8 -1, <4 x i32> %x2, <4 x i32> %x3, i32 4)
  ret void
}

declare void @llvm.x86.avx512.scattersiv8.sf(i8*, i8, <8 x i32>, <8 x float>, i32)

define void@test_int_x86_avx512_scattersiv8_sf(i8* %x0, i8 %x1, <8 x i32> %x2, <8 x float> %x3) {
; CHECK-LABEL: test_int_x86_avx512_scattersiv8_sf:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    vscatterdps %ymm1, (%rdi,%ymm0,2) {%k1}
; CHECK-NEXT:    kxnorw %k1, %k1, %k1
; CHECK-NEXT:    vscatterdps %ymm1, (%rdi,%ymm0,4) {%k1}
; CHECK-NEXT:    retq
  call void @llvm.x86.avx512.scattersiv8.sf(i8* %x0, i8 %x1, <8 x i32> %x2, <8 x float> %x3, i32 2)
  call void @llvm.x86.avx512.scattersiv8.sf(i8* %x0, i8 -1, <8 x i32> %x2, <8 x float> %x3, i32 4)
  ret void
}

declare void @llvm.x86.avx512.scattersiv8.si(i8*, i8, <8 x i32>, <8 x i32>, i32)

define void@test_int_x86_avx512_scattersiv8_si(i8* %x0, i8 %x1, <8 x i32> %x2, <8 x i32> %x3) {
; CHECK-LABEL: test_int_x86_avx512_scattersiv8_si:
; CHECK:       ## BB#0:
; CHECK-NEXT:    kmovb %esi, %k1
; CHECK-NEXT:    vpscatterdd %ymm1, (%rdi,%ymm0,2) {%k1}
; CHECK-NEXT:    kxnorw %k1, %k1, %k1
; CHECK-NEXT:    vpscatterdd %ymm1, (%rdi,%ymm0,4) {%k1}
; CHECK-NEXT:    retq
  call void @llvm.x86.avx512.scattersiv8.si(i8* %x0, i8 %x1, <8 x i32> %x2, <8 x i32> %x3, i32 2)
  call void @llvm.x86.avx512.scattersiv8.si(i8* %x0, i8 -1, <8 x i32> %x2, <8 x i32> %x3, i32 4)
  ret void
}

