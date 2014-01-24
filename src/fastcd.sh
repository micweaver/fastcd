FASTCDPATH='/home/lizhonghua/software/fastcd/fastcd-client'

if test $# -lt 1
then
 echo "usage: c directoyprefix [num]|[path]"
 return
fi

if test $# = 1
then 
CMD=$FASTCDPATH' '$1
else 
CMD=$FASTCDPATH' '$1' '$2
fi

DSTDIR=`$CMD`
STATUS=$?

if test $STATUS -lt 0
then
echo $DSTDIR
return
fi

if test $STATUS = 1
then
cd $DSTDIR
return 
fi

if test $STATUS = 2
then
declare -a RES
no=0
for DIR in $DSTDIR
do 
 no=`echo "$no + 1" | bc`
 echo $no $DIR
 RES[$no]=$DIR
done

read -p "select directory num:" num
DSTDIR=${RES[$num]}
cd $DSTDIR
fi
