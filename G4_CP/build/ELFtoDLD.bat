echo off
echo Converting fast.elf to fast.dld
echo using S8 and S2 records
gsrec -S2 -S8 -noS5 fast.elf -o fast0.dld
echo .
echo Computing and Adding CRC to fast.dld
programID fastII.pid fast0.dld fast
echo .
echo .
echo Done!

pause