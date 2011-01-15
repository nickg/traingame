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

/*
  Baseline:
  
  0000000000400d20 <vfdiv>:
  400d20:       0f 28 d0                movaps %xmm0,%xmm2
  400d23:       f3 0f 10 1f             movss  (%rdi),%xmm3
  400d27:       f3 0f 10 47 04          movss  0x4(%rdi),%xmm0
  400d2c:       f3 0f 5e da             divss  %xmm2,%xmm3
  400d30:       f3 0f 10 4f 08          movss  0x8(%rdi),%xmm1
  400d35:       f3 0f 5e c2             divss  %xmm2,%xmm0
  400d39:       f3 0f 11 5c 24 e8       movss  %xmm3,-0x18(%rsp)
  400d3f:       f3 0f 5e ca             divss  %xmm2,%xmm1
  400d43:       f3 0f 11 44 24 ec       movss  %xmm0,-0x14(%rsp)
  400d49:       f3 0f 7e 44 24 e8       movq   -0x18(%rsp),%xmm0
  400d4f:       c3                      retq   

 */

extern "C" VectorF vfdiv(const VectorF& a, float f)
{
   return a / f;
}

/*
  Baseline:

  0000000000400d50 <vfdot>:
  400d50:       f3 0f 10 07             movss  (%rdi),%xmm0
  400d54:       f3 0f 10 4f 04          movss  0x4(%rdi),%xmm1
  400d59:       f3 0f 59 06             mulss  (%rsi),%xmm0
  400d5d:       f3 0f 59 4e 04          mulss  0x4(%rsi),%xmm1
  400d62:       f3 0f 58 c1             addss  %xmm1,%xmm0
  400d66:       f3 0f 10 4f 08          movss  0x8(%rdi),%xmm1
  400d6b:       f3 0f 59 4e 08          mulss  0x8(%rsi),%xmm1
  400d70:       f3 0f 58 c1             addss  %xmm1,%xmm0
  400d74:       c3                      retq
  
*/

extern "C" float vfdot(const VectorF& a, const VectorF& b)
{
   return a.dot(b);
}

/*
  Baseline:

  0000000000400d50 <vfdot>:
  400d50:       f3 0f 10 07             movss  (%rdi),%xmm0
  400d54:       f3 0f 10 4f 04          movss  0x4(%rdi),%xmm1
  400d59:       f3 0f 59 06             mulss  (%rsi),%xmm0
  400d5d:       f3 0f 59 4e 04          mulss  0x4(%rsi),%xmm1
  400d62:       f3 0f 58 c1             addss  %xmm1,%xmm0
  400d66:       f3 0f 10 4f 08          movss  0x8(%rdi),%xmm1
  400d6b:       f3 0f 59 4e 08          mulss  0x8(%rsi),%xmm1
  400d70:       f3 0f 58 c1             addss  %xmm1,%xmm0
  400d74:       c3                      retq
  
*/

extern "C" float vflen(const VectorF& a)
{
   return a.length();
}

/*
  Baseline:

  0000000000400e00 <vfnorm>:
  400e00:       f3 0f 10 1f             movss  (%rdi),%xmm3
  400e04:       f3 0f 10 57 04          movss  0x4(%rdi),%xmm2
  400e09:       0f 28 c3                movaps %xmm3,%xmm0
  400e0c:       0f 28 e2                movaps %xmm2,%xmm4
  400e0f:       f3 0f 59 c3             mulss  %xmm3,%xmm0
  400e13:       f3 0f 10 4f 08          movss  0x8(%rdi),%xmm1
  400e18:       f3 0f 59 e2             mulss  %xmm2,%xmm4
  400e1c:       f3 0f 58 c4             addss  %xmm4,%xmm0
  400e20:       0f 28 e1                movaps %xmm1,%xmm4
  400e23:       f3 0f 59 e1             mulss  %xmm1,%xmm4
  400e27:       f3 0f 58 c4             addss  %xmm4,%xmm0
  400e2b:       f3 0f 51 c0             sqrtss %xmm0,%xmm0
  400e2f:       f3 0f 5e d8             divss  %xmm0,%xmm3
  400e33:       f3 0f 5e d0             divss  %xmm0,%xmm2
  400e37:       f3 0f 11 1f             movss  %xmm3,(%rdi)
  400e3b:       f3 0f 5e c8             divss  %xmm0,%xmm1
  400e3f:       f3 0f 11 57 04          movss  %xmm2,0x4(%rdi)
  400e44:       f3 0f 11 4f 08          movss  %xmm1,0x8(%rdi)
  400e49:       c3                      retq   
*/

extern "C" void vfnorm(VectorF& a)
{
   a.normalise();
}

/*
  Baseline:

  0000000000400d80 <vfeq>:
  400d80:       f3 0f 10 0e             movss  (%rsi),%xmm1
  400d84:       f3 0f 10 15 e4 02 00    movss  0x2e4(%rip),%xmm2        # 401070 <_IO_stdin_used+0x10>
  400d8b:       00 
  400d8c:       f3 0f 5c 0f             subss  (%rdi),%xmm1
  400d90:       0f 54 ca                andps  %xmm2,%xmm1
  400d93:       0f 2e c1                ucomiss %xmm1,%xmm0
  400d96:       76 28                   jbe    400dc0 <vfeq+0x40>
  400d98:       f3 0f 10 4e 04          movss  0x4(%rsi),%xmm1
  400d9d:       f3 0f 5c 4f 04          subss  0x4(%rdi),%xmm1
  400da2:       0f 54 ca                andps  %xmm2,%xmm1
  400da5:       0f 2e c1                ucomiss %xmm1,%xmm0
  400da8:       76 16                   jbe    400dc0 <vfeq+0x40>
  400daa:       f3 0f 10 4e 08          movss  0x8(%rsi),%xmm1
  400daf:       f3 0f 5c 4f 08          subss  0x8(%rdi),%xmm1
  400db4:       0f 54 ca                andps  %xmm2,%xmm1
  400db7:       0f 2e c1                ucomiss %xmm1,%xmm0
  400dba:       0f 97 c0                seta   %al
  400dbd:       c3                      retq   
  400dbe:       66 90                   xchg   %ax,%ax
  400dc0:       31 c0                   xor    %eax,%eax
  400dc2:       c3                      retq
*/

extern "C" bool vfeq(const VectorF& a, const VectorF& b, float d)
{
   return a.approx_equal(b, d);
}

int main(int argc, char **argv)
{
   VectorF a = make_vector(2.0f, 3.0f, 4.0f);
   VectorF b = make_vector(5.0f, 6.0f, 7.0f);

   VectorF c = vfadd(a, b);

   cout << c << endl;

   assert(!vfeq(a, b, 0.1f));
   assert(vfeq(c, c, 0.1f));

   vfnorm(a);
   assert(vflen(a) > 0.999f && vflen(a) < 1.001f);
   
   return 0;
}
