.text
.global TempleOS_CallNOnStack
.global TempleOS_CallN

// Aiwnios passes every argument as a raw 64-bit word, including F64 bits.
// The first eight use X0-X7; the rest occupy successive 8-byte stack slots.
TempleOS_CallN:
  stp x19,x20,[sp,-16]!
  stp x21,x22,[sp,-16]!
  stp x23,x30,[sp,-16]!
  mov x19,sp
  mov x22,x0                 // function pointer
  mov x20,x1                 // argument count
  mov x21,x2                 // argument vector
  b .Lcall_args

// Call a guest function with an arbitrary count of integer-class arguments
// on a caller-provided, 16-byte-aligned stack. The current stack remains
// intact so cooperative Yield can park and resume this fiber on the new one.
TempleOS_CallNOnStack:
  stp x19,x20,[sp,-16]!
  stp x21,x22,[sp,-16]!
  stp x23,x30,[sp,-16]!
  mov x19,sp
  mov x23,x0                 // alternate stack top
  mov x22,x1                 // function pointer
  mov x20,x2                 // argument count
  mov x21,x3                 // argument vector
  and x23,x23,#-16
  mov sp,x23
.Lcall_args:
  cmp x20,#8
  b.ls 2f
  sub x9,x20,#8
  lsl x9,x9,#3
  add x9,x9,#15
  and x9,x9,#-16
  sub sp,sp,x9
  mov x10,#8
1:
  cmp x10,x20
  b.hs 2f
  ldr x11,[x21,x10,lsl #3]
  sub x12,x10,#8
  str x11,[sp,x12,lsl #3]
  add x10,x10,#1
  b 1b
2:
  cmp x20,#0
  b.ls 3f
  ldr x0,[x21]
  cmp x20,#1
  b.ls 3f
  ldr x1,[x21,#8]
  cmp x20,#2
  b.ls 3f
  ldr x2,[x21,#16]
  cmp x20,#3
  b.ls 3f
  ldr x3,[x21,#24]
  cmp x20,#4
  b.ls 3f
  ldr x4,[x21,#32]
  cmp x20,#5
  b.ls 3f
  ldr x5,[x21,#40]
  cmp x20,#6
  b.ls 3f
  ldr x6,[x21,#48]
  cmp x20,#7
  b.ls 3f
  ldr x7,[x21,#56]
3:
  blr x22
  mov sp,x19
  ldp x23,x30,[sp],16
  ldp x21,x22,[sp],16
  ldp x19,x20,[sp],16
  ret
