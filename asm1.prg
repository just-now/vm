li r0 0       #  0 -- syscall(write)
li r1 A       # 10 -- address
li r2 8       #  8 -- len
push r1
push r2
int 80
halt
