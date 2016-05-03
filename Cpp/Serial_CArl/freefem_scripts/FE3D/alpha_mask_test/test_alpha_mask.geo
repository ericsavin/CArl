cl__1 = 1;

Point(1) = {0, 0, 0, 1};
Point(2) = {0, 1, 0, 1};
Point(3) = {0, 1, 1, 1};
Point(4) = {0, 0, 1, 1};

Point(5) = {1, 0, 0, 1};
Point(6) = {1, 1, 0, 1};
Point(7) = {1, 1, 1, 1};
Point(8) = {1, 0, 1, 1};

Point(9) = {2, 0, 1, 1};
Point(10) = {2, 1, 1, 1};
Point(11) = {2, 1, 0, 1};
Point(12) = {2, 0, 0, 1};

Point(13) = {3, 0, 1, 1};
Point(14) = {3, 1, 1, 1};
Point(15) = {3, 1, 0, 1};
Point(16) = {3, 0, 0, 1};

Line(101) = {3, 7};
Line(102) = {7, 6};
Line(103) = {6, 2};
Line(104) = {2, 3};
Line(105) = {3, 4};
Line(106) = {4, 1};
Line(107) = {1, 2};
Line(108) = {4, 8};
Line(109) = {8, 5};
Line(1010) = {5, 1};
Line(1011) = {8, 7};
Line(1012) = {6, 5};
Line(1013) = {5, 12};
Line(1014) = {12, 16};
Line(1015) = {16, 15};
Line(1016) = {15, 11};
Line(1017) = {11, 12};
Line(1018) = {11, 6};
Line(1019) = {7, 10};
Line(1020) = {10, 11};
Line(1021) = {10, 14};
Line(1022) = {14, 15};
Line(1023) = {14, 13};
Line(1024) = {13, 16};
Line(1025) = {13, 9};
Line(1026) = {9, 10};
Line(1027) = {9, 8};
Line(1028) = {9, 12};

Line Loop(1029) = {104, 101, 102, 103};
Plane Surface(1030) = {1029};
Line Loop(1031) = {1019, 1020, 1018, -102};
Plane Surface(1032) = {1031};
Line Loop(1033) = {1016, -1020, 1021, 1022};
Plane Surface(1034) = {1033};
Line Loop(1035) = {1015, 1016, 1017, 1014};
Plane Surface(1036) = {1035};
Line Loop(1037) = {1017, -1013, -1012, -1018};
Plane Surface(1038) = {1037};
Line Loop(1039) = {103, -107, -1010, -1012};
Plane Surface(1040) = {1039};
Line Loop(1041) = {104, 105, 106, 107};
Plane Surface(1042) = {1041};
Line Loop(1043) = {1011, 102, 1012, -109};
Plane Surface(1044) = {1043};
Line Loop(1045) = {1026, 1020, 1017, -1028};
Plane Surface(1046) = {1045};
Line Loop(1047) = {1023, 1024, 1015, -1022};
Plane Surface(1048) = {1047};
Line Loop(1049) = {1025, 1028, 1014, -1024};
Plane Surface(1050) = {1049};
Line Loop(1051) = {1027, 109, 1013, -1028};
Plane Surface(1052) = {1051};
Line Loop(1053) = {108, 109, 1010, -106};
Plane Surface(1054) = {1053};
Line Loop(1055) = {105, 108, 1011, -101};
Plane Surface(1056) = {1055};
Line Loop(1057) = {1019, -1026, 1027, 1011};
Plane Surface(1058) = {1057};
Line Loop(1059) = {1021, 1023, 1025, 1026};
Plane Surface(1060) = {1059};
Surface Loop(1061) = {1030, 1042, 1056, 1054, 1040, 1044};
Volume(1062) = {1061};
Surface Loop(1063) = {1032, 1058, 1052, 1038, 1046, 1044};
Volume(1064) = {1063};
Surface Loop(1065) = {1034, 1036, 1048, 1060, 1050, 1046};
Volume(1066) = {1065};