.\AStyle.exe ^
  --indent=spaces=4  --max-instatement-indent=80 ^
  --brackets=attach --break-closing-brackets --add-brackets ^
  --pad-oper --pad-paren-in --pad-header --align-pointer=type --suffix=none ^
  --keep-one-line-statements --keep-one-line-blocks --recursive  ^
  --indent-namespaces ^
  ./src/*.c* ./src/*.h* > astyle.log