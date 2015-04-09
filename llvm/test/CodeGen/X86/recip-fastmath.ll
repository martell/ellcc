; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mattr=sse2 | FileCheck %s
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mattr=avx,use-recip-est | FileCheck %s --check-prefix=RECIP
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mattr=avx,use-recip-est -x86-recip-refinement-steps=2 | FileCheck %s --check-prefix=REFINE

; If the target's divss/divps instructions are substantially
; slower than rcpss/rcpps with a Newton-Raphson refinement,
; we should generate the estimate sequence.

; See PR21385 ( http://llvm.org/bugs/show_bug.cgi?id=21385 )
; for details about the accuracy, speed, and implementation
; differences of x86 reciprocal estimates.

define float @reciprocal_estimate(float %x) #0 {
  %div = fdiv fast float 1.0, %x
  ret float %div

; CHECK-LABEL: reciprocal_estimate:
; CHECK: movss
; CHECK-NEXT: divss
; CHECK-NEXT: movaps
; CHECK-NEXT: retq

; RECIP-LABEL: reciprocal_estimate:
; RECIP: vrcpss
; RECIP: vmulss
; RECIP: vsubss
; RECIP: vmulss
; RECIP: vaddss
; RECIP-NEXT: retq

; REFINE-LABEL: reciprocal_estimate:
; REFINE: vrcpss
; REFINE: vmulss
; REFINE: vsubss
; REFINE: vmulss
; REFINE: vaddss
; REFINE: vmulss
; REFINE: vsubss
; REFINE: vmulss
; REFINE: vaddss
; REFINE-NEXT: retq
}

define <4 x float> @reciprocal_estimate_v4f32(<4 x float> %x) #0 {
  %div = fdiv fast <4 x float> <float 1.0, float 1.0, float 1.0, float 1.0>, %x
  ret <4 x float> %div

; CHECK-LABEL: reciprocal_estimate_v4f32:
; CHECK: movaps
; CHECK-NEXT: divps
; CHECK-NEXT: movaps
; CHECK-NEXT: retq

; RECIP-LABEL: reciprocal_estimate_v4f32:
; RECIP: vrcpps
; RECIP: vmulps
; RECIP: vsubps
; RECIP: vmulps
; RECIP: vaddps
; RECIP-NEXT: retq

; REFINE-LABEL: reciprocal_estimate_v4f32:
; REFINE: vrcpps
; REFINE: vmulps
; REFINE: vsubps
; REFINE: vmulps
; REFINE: vaddps
; REFINE: vmulps
; REFINE: vsubps
; REFINE: vmulps
; REFINE: vaddps
; REFINE-NEXT: retq
}

define <8 x float> @reciprocal_estimate_v8f32(<8 x float> %x) #0 {
  %div = fdiv fast <8 x float> <float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0>, %x
  ret <8 x float> %div

; CHECK-LABEL: reciprocal_estimate_v8f32:
; CHECK: movaps
; CHECK: movaps
; CHECK-NEXT: divps
; CHECK-NEXT: divps
; CHECK-NEXT: movaps
; CHECK-NEXT: movaps
; CHECK-NEXT: retq

; RECIP-LABEL: reciprocal_estimate_v8f32:
; RECIP: vrcpps
; RECIP: vmulps
; RECIP: vsubps
; RECIP: vmulps
; RECIP: vaddps
; RECIP-NEXT: retq

; REFINE-LABEL: reciprocal_estimate_v8f32:
; REFINE: vrcpps
; REFINE: vmulps
; REFINE: vsubps
; REFINE: vmulps
; REFINE: vaddps
; REFINE: vmulps
; REFINE: vsubps
; REFINE: vmulps
; REFINE: vaddps
; REFINE-NEXT: retq
}

attributes #0 = { "unsafe-fp-math"="true" }
