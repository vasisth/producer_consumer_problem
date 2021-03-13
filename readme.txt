Steps followed for project execution.

1) Using localhost

[schandan@mercury assign3_DIR]$ cd buffer\ managersection/
[schandan@mercury buffer managersection]$ make
[schandan@mercury buffer managersection]$ ./BM
[schandan@mercury assign3_DIR]$ cd Server\ section/
[schandan@mercury Server section]$ make
[schandan@mercury Server section]$ ./nc
Enter node number
1
[schandan@mercury Server section]$ ./nc
Enter node number
2
[schandan@mercury Server section]$ ./nc
Enter node number
3


2) Using different hosts

[schandan@91308-04 assign3_DIR]$ cd buffer\ managersection/
[schandan@91308-04 buffer managersection]$ make
[schandan@91308-04 buffer managersection]$ ./BM
[schandan@91308-01 assign3_DIR]$ cd Server\ section/
[schandan@91308-01 Server section]$ make
[schandan@91308-01 Server section]$ ./nc
Enter node number
1
[schandan@91308-02 Server section]$ ./nc
Enter node number
2
[schandan@91308-03 Server section]$ ./nc
Enter node number
3
