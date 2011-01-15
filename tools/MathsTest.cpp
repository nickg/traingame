#include "Maths.hpp"

#include <iostream>
#include <cassert>

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

  With packed float vector inside union:

  0000000000400bf0 <vfadd>:
  400bf0:       0f 28 07                movaps (%rdi),%xmm0
  400bf3:       0f 58 06                addps  (%rsi),%xmm0
  400bf6:       0f 29 44 24 a8          movaps %xmm0,-0x58(%rsp)
  400bfb:       48 8b 44 24 a8          mov    -0x58(%rsp),%rax
  400c00:       0f 29 44 24 d8          movaps %xmm0,-0x28(%rsp)
  400c05:       48 89 44 24 a0          mov    %rax,-0x60(%rsp)
  400c0a:       f3 0f 7e 4c 24 e0       movq   -0x20(%rsp),%xmm1
  400c10:       f3 0f 7e 44 24 a0       movq   -0x60(%rsp),%xmm0
  400c16:       c3                      retq   

  Without the union:

  0000000000400bf0 <vfadd>:
  400bf0:       0f 28 07                movaps (%rdi),%xmm0
  400bf3:       0f 58 06                addps  (%rsi),%xmm0
  400bf6:       c3                      retq   
  400bf7:       66 0f 1f 84 00 00 00    nopw   0x0(%rax,%rax,1)
  400bfe:       00 00 
  
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

  Packed vector:

  0000000000400dc0 <vfdiv>:
  400dc0:       f3 0f 11 44 24 e8       movss  %xmm0,-0x18(%rsp)
  400dc6:       f3 0f 11 44 24 ec       movss  %xmm0,-0x14(%rsp)
  400dcc:       f3 0f 11 44 24 f0       movss  %xmm0,-0x10(%rsp)
  400dd2:       0f 28 07                movaps (%rdi),%xmm0
  400dd5:       0f 5e 44 24 e8          divps  -0x18(%rsp),%xmm0
  400dda:       c3                      retq   
  
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

  Packed vector:

  0000000000400de0 <vfdot>:
  400de0:       0f 28 07                movaps (%rdi),%xmm0
  400de3:       0f 59 06                mulps  (%rsi),%xmm0
  400de6:       0f 29 44 24 e8          movaps %xmm0,-0x18(%rsp)
  400deb:       f3 0f 10 44 24 e8       movss  -0x18(%rsp),%xmm0
  400df1:       f3 0f 58 44 24 ec       addss  -0x14(%rsp),%xmm0
  400df7:       f3 0f 58 44 24 f0       addss  -0x10(%rsp),%xmm0
  400dfd:       c3                      retq   
  
*/

extern "C" float vfdot(const VectorF& a, const VectorF& b)
{
   return a.dot(b);
}

/*
  Baseline:

  0000000000400ea0 <vflen>:
  400ea0:       f3 0f 10 07             movss  (%rdi),%xmm0
  400ea4:       f3 0f 10 57 04          movss  0x4(%rdi),%xmm2
  400ea9:       f3 0f 59 c0             mulss  %xmm0,%xmm0
  400ead:       f3 0f 59 d2             mulss  %xmm2,%xmm2
  400eb1:       f3 0f 10 4f 08          movss  0x8(%rdi),%xmm1
  400eb6:       f3 0f 59 c9             mulss  %xmm1,%xmm1
  400eba:       f3 0f 58 c2             addss  %xmm2,%xmm0
  400ebe:       f3 0f 58 c1             addss  %xmm1,%xmm0
  400ec2:       f3 0f 51 c0             sqrtss %xmm0,%xmm0
  400ec6:       c3                      retq

  Packed vector:

  0000000000400ee0 <vflen>:
  400ee0:       48 83 ec 18             sub    $0x18,%rsp
  400ee4:       0f 28 07                movaps (%rdi),%xmm0
  400ee7:       0f 59 c0                mulps  %xmm0,%xmm0
  400eea:       0f 29 04 24             movaps %xmm0,(%rsp)
  400eee:       f3 0f 10 0c 24          movss  (%rsp),%xmm1
  400ef3:       f3 0f 58 4c 24 04       addss  0x4(%rsp),%xmm1
  400ef9:       f3 0f 58 4c 24 08       addss  0x8(%rsp),%xmm1
  400eff:       f3 0f 51 c1             sqrtss %xmm1,%xmm0
  400f03:       0f 2e c0                ucomiss %xmm0,%xmm0
  400f06:       7a 02                   jp     400f0a <vflen+0x2a>
  400f08:       74 08                   je     400f12 <vflen+0x32>
  400f0a:       0f 28 c1                movaps %xmm1,%xmm0
  400f0d:       e8 96 fd ff ff          callq  400ca8 <sqrtf@plt>
  400f12:       48 83 c4 18             add    $0x18,%rsp
  400f16:       c3                      retq

  -ffast-math:

  400e50:       0f 28 07                movaps (%rdi),%xmm0
  400e53:       0f 59 c0                mulps  %xmm0,%xmm0
  400e56:       0f 29 44 24 e8          movaps %xmm0,-0x18(%rsp)
  400e5b:       f3 0f 10 44 24 ec       movss  -0x14(%rsp),%xmm0
  400e61:       f3 0f 58 44 24 e8       addss  -0x18(%rsp),%xmm0
  400e67:       f3 0f 58 44 24 f0       addss  -0x10(%rsp),%xmm0
  400e6d:       f3 0f 51 c0             sqrtss %xmm0,%xmm0
  400e71:       c3                      retq
  
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

  Packed vector:
  
  0000000000400e80 <vfnorm>:
  400e80:       53                      push   %rbx
  400e81:       48 89 fb                mov    %rdi,%rbx
  400e84:       48 83 ec 20             sub    $0x20,%rsp
  400e88:       0f 28 17                movaps (%rdi),%xmm2
  400e8b:       0f 28 c2                movaps %xmm2,%xmm0
  400e8e:       0f 59 c2                mulps  %xmm2,%xmm0
  400e91:       0f 29 04 24             movaps %xmm0,(%rsp)
  400e95:       f3 0f 10 0c 24          movss  (%rsp),%xmm1
  400e9a:       f3 0f 58 4c 24 04       addss  0x4(%rsp),%xmm1
  400ea0:       f3 0f 58 4c 24 08       addss  0x8(%rsp),%xmm1
  400ea6:       f3 0f 51 c1             sqrtss %xmm1,%xmm0
  400eaa:       0f 2e c0                ucomiss %xmm0,%xmm0
  400ead:       7a 02                   jp     400eb1 <vfnorm+0x31>
  400eaf:       74 0b                   je     400ebc <vfnorm+0x3c>
  400eb1:       0f 28 c1                movaps %xmm1,%xmm0
  400eb4:       e8 ef fd ff ff          callq  400ca8 <sqrtf@plt>
  400eb9:       0f 28 13                movaps (%rbx),%xmm2
  400ebc:       f3 0f 11 44 24 10       movss  %xmm0,0x10(%rsp)
  400ec2:       f3 0f 11 44 24 14       movss  %xmm0,0x14(%rsp)
  400ec8:       f3 0f 11 44 24 18       movss  %xmm0,0x18(%rsp)
  400ece:       0f 5e 54 24 10          divps  0x10(%rsp),%xmm2
  400ed3:       0f 29 13                movaps %xmm2,(%rbx)
  400ed6:       48 83 c4 20             add    $0x20,%rsp
  400eda:       5b                      pop    %rbx
  400edb:       c3                      retq   
  400edc:       0f 1f 40 00             nopl   0x0(%rax)

  -ffast-math:

  0000000000400e10 <vfnorm>:
  400e10:       0f 28 0f                movaps (%rdi),%xmm1
  400e13:       0f 28 c1                movaps %xmm1,%xmm0
  400e16:       0f 59 c1                mulps  %xmm1,%xmm0
  400e19:       0f 29 44 24 d8          movaps %xmm0,-0x28(%rsp)
  400e1e:       f3 0f 10 44 24 dc       movss  -0x24(%rsp),%xmm0
  400e24:       f3 0f 58 44 24 d8       addss  -0x28(%rsp),%xmm0
  400e2a:       f3 0f 58 44 24 e0       addss  -0x20(%rsp),%xmm0
  400e30:       f3 0f 51 c0             sqrtss %xmm0,%xmm0
  400e34:       f3 0f 11 44 24 e8       movss  %xmm0,-0x18(%rsp)
  400e3a:       f3 0f 11 44 24 ec       movss  %xmm0,-0x14(%rsp)
  400e40:       f3 0f 11 44 24 f0       movss  %xmm0,-0x10(%rsp)
  400e46:       0f 5e 4c 24 e8          divps  -0x18(%rsp),%xmm1
  400e4b:       0f 29 0f                movaps %xmm1,(%rdi)
  400e4e:       c3                      retq
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

  Packed vector:

  0000000000400e00 <vfeq>:
  400e00:       0f 28 0e                movaps (%rsi),%xmm1
  400e03:       f3 0f 10 15 45 05 00    movss  0x545(%rip),%xmm2        # 401350 <_ZZ4mainE19__PRETTY_FUNCTION__+0x20>
  400e0a:       00 
  400e0b:       0f 5c 0f                subps  (%rdi),%xmm1
  400e0e:       0f 29 4c 24 e8          movaps %xmm1,-0x18(%rsp)
  400e13:       f3 0f 10 4c 24 e8       movss  -0x18(%rsp),%xmm1
  400e19:       0f 54 ca                andps  %xmm2,%xmm1
  400e1c:       0f 2e c1                ucomiss %xmm1,%xmm0
  400e1f:       76 1f                   jbe    400e40 <vfeq+0x40>
  400e21:       f3 0f 10 4c 24 ec       movss  -0x14(%rsp),%xmm1
  400e27:       0f 54 ca                andps  %xmm2,%xmm1
  400e2a:       0f 2e c1                ucomiss %xmm1,%xmm0
  400e2d:       76 11                   jbe    400e40 <vfeq+0x40>
  400e2f:       f3 0f 10 4c 24 f0       movss  -0x10(%rsp),%xmm1
  400e35:       0f 54 ca                andps  %xmm2,%xmm1
  400e38:       0f 2e c1                ucomiss %xmm1,%xmm0
  400e3b:       0f 97 c0                seta   %al
  400e3e:       c3                      retq   
  400e3f:       90                      nop
  400e40:       31 c0                   xor    %eax,%eax
  400e42:       c3                      retq

  -ffast-math:

  0000000000400d90 <vfeq>:
  400d90:       0f 28 0e                movaps (%rsi),%xmm1
  400d93:       f3 0f 10 15 25 05 00    movss  0x525(%rip),%xmm2        # 4012c0 <_ZZ4mainE19__PRETTY_FUNCTION__+0x20>
  400d9a:       00 
  400d9b:       0f 5c 0f                subps  (%rdi),%xmm1
  400d9e:       0f 29 4c 24 e8          movaps %xmm1,-0x18(%rsp)
  400da3:       f3 0f 10 4c 24 e8       movss  -0x18(%rsp),%xmm1
  400da9:       0f 54 ca                andps  %xmm2,%xmm1
  400dac:       0f 2f c1                comiss %xmm1,%xmm0
  400daf:       76 1f                   jbe    400dd0 <vfeq+0x40>
  400db1:       f3 0f 10 4c 24 ec       movss  -0x14(%rsp),%xmm1
  400db7:       0f 54 ca                andps  %xmm2,%xmm1
  400dba:       0f 2f c1                comiss %xmm1,%xmm0
  400dbd:       76 11                   jbe    400dd0 <vfeq+0x40>
  400dbf:       f3 0f 10 4c 24 f0       movss  -0x10(%rsp),%xmm1
  400dc5:       0f 54 ca                andps  %xmm2,%xmm1
  400dc8:       0f 2f c1                comiss %xmm1,%xmm0
  400dcb:       0f 97 c0                seta   %al
  400dce:       c3                      retq   
  400dcf:       90                      nop
  400dd0:       31 c0                   xor    %eax,%eax
  400dd2:       c3                      retq

  Replace abs with square:

  0000000000400dc0 <vfeq>:
  400dc0:       0f 28 0e                movaps (%rsi),%xmm1
  400dc3:       f3 0f 59 c0             mulss  %xmm0,%xmm0
  400dc7:       0f 5c 0f                subps  (%rdi),%xmm1
  400dca:       0f 59 c9                mulps  %xmm1,%xmm1
  400dcd:       0f 29 4c 24 e8          movaps %xmm1,-0x18(%rsp)
  400dd2:       0f 2f 44 24 e8          comiss -0x18(%rsp),%xmm0
  400dd7:       76 17                   jbe    400df0 <vfeq+0x30>
  400dd9:       0f 2f 44 24 ec          comiss -0x14(%rsp),%xmm0
  400dde:       76 10                   jbe    400df0 <vfeq+0x30>
  400de0:       0f 2f 44 24 f0          comiss -0x10(%rsp),%xmm0
  400de5:       0f 97 c0                seta   %al
  400de8:       c3                      retq

  Remove delta parameter:
  
  0000000000400d90 <vfeq>:
  400d90:       0f 28 06                movaps (%rsi),%xmm0
  400d93:       0f 5c 07                subps  (%rdi),%xmm0
  400d96:       0f 59 c0                mulps  %xmm0,%xmm0
  400d99:       0f 29 44 24 e8          movaps %xmm0,-0x18(%rsp)
  400d9e:       f3 0f 10 44 24 e8       movss  -0x18(%rsp),%xmm0
  400da4:       0f 2f 05 dd 04 00 00    comiss 0x4dd(%rip),%xmm0
  400dab:       73 23                   jae    400dd0 <vfeq+0x40>
  400dad:       f3 0f 10 44 24 ec       movss  -0x14(%rsp),%xmm0
  400db3:       0f 2f 05 ce 04 00 00    comiss 0x4ce(%rip),%xmm0
  400dba:       73 14                   jae    400dd0 <vfeq+0x40>
  400dbc:       f3 0f 10 44 24 f0       movss  -0x10(%rsp),%xmm0
  400dc2:       0f 2f 05 bf 04 00 00    comiss 0x4bf(%rip),%xmm0
  400dc9:       0f 92 c0                setb   %al
  400dcc:       c3                      retq
*/

extern "C" bool vfeq(const VectorF& a, const VectorF& b)
{
   return a == b;
}

int main(int argc, char **argv)
{
   VectorF a = make_vector(2.0f, 3.0f, 4.0f);
   VectorF b = make_vector(5.0f, 6.0f, 7.0f);

   VectorF c = vfadd(a, b);

   cout << c << endl;

   assert(!vfeq(a, b));
   assert(vfeq(c, c));

   vfnorm(a);
   assert(vflen(a) > 0.999f && vflen(a) < 1.001f);
   
   return 0;
}
