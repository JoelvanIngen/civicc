bar:
    esr 1
    iloadc 0
    istore 0
    isrg
    iload_0
    jsre 0
    isrg
    iloadc_1
    jsre 2

boz:
    esr 1
    iload_0
    istore 0
    iload_0
    iloadc 0
    iadd
    istore 0
    isrg
    iload_0
    jsre 0
    isrg
    iloadc_1
    jsre 2

biz:
    iloadc 1
    istoren 1 0

main:
    esr 2
    iloadc 2
    iload_0
    istore 1
    istore 0
    isrl
    iload_0
    jsr 1
    isrg
    iload_0
    jsre 0
    isrg
    iloadc 3
    jsre 2
    isrg
    iload_0
    jsr 1
    isrg
    iload_0
    jsre 0
    isrg
    iloadc 3
    jsre 2
    isrg
    iload_0
    jsr 1
    isrg
    iload_0
    jsre 0
    isrg
    iloadc 3
    jsre 2
    isrl
    jsr 0
    isrg
    iload_0
    jsre 0
    isrg
    iloadc 3
    jsre 2
    isrg
    iload_1
    jsre 0
    isrg
    iloadc 3
    jsre 2
    isrl
    jsr 0
    isrg
    iload_0
    jsre 0
    isrg
    iloadc 3
    jsre 2
    isrg
    iload_1
    jsre 0
    isrg
    iloadc_1
    jsre 2
    iloadc_0
    ireturn

baz_inner:
    esr 1
    iloadc 4
    istore 0
    isrg
    iload_0
    jsre 0
    isrg
    iloadc_1
    jsre 2

baz:
    esr 1
    isrl
    jsr 0

b:
    isrg
    iloadc 5
    jsre 0
    isrg
    iloadc_1
    jsre 2

baz_inner:
    isrl
    jsr 0

bor:
    esr 1
    isrl
    jsr 0

.const int 11111
.const int 55555
.const int 123
.const int 2
.const int 22222
.const int 33333
.exportfun "printInt" void printInt
.exportfun "printSpaces" void printSpaces
.exportfun "printNewlines" void printNewlines
.exportfun "main" int main
.exportfun "bar" void bar
.exportfun "boz" void boz
.exportfun "biz" void biz
.exportfun "baz" void baz
.exportfun "baz_inner" void baz_inner
.exportfun "bor" void bor
.exportfun "baz_inner" void baz_inner
.exportfun "b" void b
