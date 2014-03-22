./configure
cd kern/conf
./config ASST1
cd ../compile/ASST1
bmake depend
bmake
bmake install
cd ../../../

cd ../root
if [ "$1" =  "-d" ]
then
  sys161 -w kernel q
else
  sys161 kernel q
fi
cd ..
