f:

g:

func:
    esr 4
    iloadc 0
    ineg
    iloadc 1
    iadd
    iloadc 0
    iloadc 1
    isub
    istore 1
    istore 0

.const int 2
.const int 10
.exportfun "func" void func
.exportfun "f" void f
.exportfun "g" void g
