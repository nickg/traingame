#include "Maths.hpp"

#include <iostream>

/*

  Baseline:
  
  0000000000400bc0 <vfadd>:
  400bc0:       66 0f d6 44 24 d8       movq   %xmm0,-0x28(%rsp)
  400bc6:       f3 0f 58 cb             addss  %xmm3,%xmm1
  400bca:       66 0f d6 54 24 c8       movq   %xmm2,-0x38(%rsp)
  400bd0:       f3 0f 10 44 24 dc       movss  -0x24(%rsp),%xmm0
  400bd6:       f3 0f 10 54 24 d8       movss  -0x28(%rsp),%xmm2
  400bdc:       f3 0f 58 44 24 cc       addss  -0x34(%rsp),%xmm0
  400be2:       f3 0f 58 54 24 c8       addss  -0x38(%rsp),%xmm2
  400be8:       f3 0f 11 44 24 ec       movss  %xmm0,-0x14(%rsp)
  400bee:       f3 0f 11 54 24 e8       movss  %xmm2,-0x18(%rsp)
  400bf4:       f3 0f 7e 44 24 e8       movq   -0x18(%rsp),%xmm0
  400bfa:       c3                      retq

  Make arguments const&:

  0000000000400bc0 <vfadd>:
  400bc0:       f3 0f 10 47 04          movss  0x4(%rdi),%xmm0
  400bc5:       f3 0f 10 17             movss  (%rdi),%xmm2
  400bc9:       f3 0f 58 46 04          addss  0x4(%rsi),%xmm0
  400bce:       f3 0f 58 16             addss  (%rsi),%xmm2
  400bd2:       f3 0f 10 4f 08          movss  0x8(%rdi),%xmm1
  400bd7:       f3 0f 58 4e 08          addss  0x8(%rsi),%xmm1
  400bdc:       f3 0f 11 44 24 ec       movss  %xmm0,-0x14(%rsp)
  400be2:       f3 0f 11 54 24 e8       movss  %xmm2,-0x18(%rsp)
  400be8:       f3 0f 7e 44 24 e8       movq   -0x18(%rsp),%xmm0
  400bee:       c3                      retq   

 */

extern "C" VectorF vfadd(const VectorF& a, const VectorF& b)
{
   return a + b;
}

int main(int argc, char **argv)
{
   VectorF a = make_vector(2.0f, 3.0f, 4.0f);
   VectorF b = make_vector(5.0f, 6.0f, 7.0f);

   VectorF c = vfadd(a, b);

   cout << c << endl;
   
   return 0;
}
