fastcd
======


   在linux命令行操作界面下，要进入某个目录，只能通过cd命令一层层的进入,如果该目录是我们平常使用所熟知的，这样一层层进入确实挺烦人，为了减少指骨节劳损的概率，我们希望能够直接进入任意的目录，fastcd即是这样一个非常方便使用的工具。
   
   fastcd 会通过用户提供的目录前缀搜索相关目录，即不一定要提供目录全名，如果搜索结果是惟一的即直接进入相关目录，如果有多个结果即列出所有结果让我们选择，选择相应编号即可，但如果每次都要选择结果编号才进入也挺烦人，实际上fastcd支持再加一个编号参数就可以直接进入相应目录拉，在记住某个目录编号的情况下这是很方便的。
   
   你也许会想，这通过一个shell脚本，使用find命令也可以实现类似功能，不过find命令需要从磁盘搜索文件，在有大量文件时速度会很慢，fastcd的一个重要优点是速度极快，即使成千上万的文件，我们选择进入某个目录时也会感觉不到延迟，我知道程序员的脾气，如果命令操作有延迟一定会想砸电脑。

下面看看fastcd的具体使用

fastcd 有一个服务端程序，负责接收目录查询请求，启动服务端程序如下所示

    $ ./fastcd-server . /usr/local/ /home/q/

服务程序fastcd-server可以接收多个参数，表示要建立索引的目录,相应目录下的所有子目录都会进行索引，如果不提供参数，则表示对当前目录建立索引。服务启动后会以后台进程运行，并输出如下结果

    ………
    /usr/local/go/test/safe
    /usr/local/go/test/stress
    /usr/local/go/test/syntax
    /usr/local/man
    /usr/local/man/man3
    /usr/local/redis26
    /usr/local/redis26/conf
    
    
    [fail index directory  list:]
    /home/lizhonghua/software/fastcd/dirtest/a?abc  [illegal char]
    /home/lizhonghua/software/fastcd/abc?efg  [illegal char]
    /usr/local/mysql/data/zd_user_actions  [no power]
    /usr/local/nginx/client_body_temp  [no power]
    /usr/local/nginx/proxy_temp  [no power]
    /usr/local/nginx/fastcgi_temp  [no power]
    /usr/local/nginx/uwsgi_temp  [no power]
    /usr/local/nginx/scgi_temp  [no power]
    /usr/local/inotify/backup/sync360  [no power]
    /usr/local/inotify/backup/.ssh/.ssh  [no power]
    
    
    [sum of index files:]
    6306
   
如上是服务启动时的相关输出，分别列出建立索引的目录列表，索引失败的目录列表，以及总共有多少目录进行了索引。 索引失败可能是因为目录包括了不在相应范围的字符或是没有权限进行读取

对于目录改变的情况，我们需要重新启动fastcd服务器才能生效。


负责与fastcd通信进行目录查询的是一个fastcd-client程序，不过直接在shell上运行fastcd-client程序改变目录是没有作用的，这是由于shell启动了子进程改变目录而不能影响shell本身，这需要一个shell脚本配合来使用，具体方法后面再描述，我们先看看fastcd真正操作时的效果:

    $ c model 
    1 /home/lizhonghua/project/admin/src/application/models
    2 /home/lizhonghua/project/web/src/application/models
    3 /home/lizhonghua/project/user/src/application/models
    select directory num: 3
    $ pwd
    /home/lizhonghua/project/user/src/application/models

我们键入 c model 命令(c只是一个alias,你可以选择自己喜欢的名字) ,即把当前目录下所有以model为前缀的目录列出来，然后选择编号3，即进入相应目录，也可以直接进入相应目录

      $c model 3
      $pwd
      /home/lizhonghua/project/user/src/application/models


另外第二个参数可以是某个目录路径，表示在其路径下搜索目录，比如 c model / 表示在所有索引中搜索 model,因/代表根目录


fastcd-server支持三种搜索模式，可以在启动时通过 –m 参数进行配置
在没有提供第二个参数时，即比如 c model 时配置m值对应的策略：<br />
1 只搜索当前目录<br />
2 搜索所有索引的目录<br />
3 先搜索当前目录，如果有结果进行相应处理，为空的话再搜索所有目录<br />

默认是第三种策略，这使得我们处在最深层次目录，而想进入另外的目录时，即使不提供第二个代表搜索目录的参数也能找到相应目录。



为了真正让shell改变目录我们需要一个脚本来调用fastcd-client程序，如下所示:

    FASTCDPATH='/home/lizhonghua/software/fastcd/fastcd-client' #这里配置成fastcd-client客户端路径即可
    
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

建立上面脚本文件后我们需要定义一个alias,如下所示

    alias c='.   /home/lizhonghua/software/fastcd/fastcd.sh'

注意以上. 和路径之间有空格，shell中.命令表示在shell本身中执行，这样在fastcd.sh中改变目录的命令就会在执行shell中起作用拉。


获取fastcd-server及fastcd-client程序请进入github,下载所有源码放在同一目录下按如下命令编译即可

    $gcc  -o  fastcd-server  fastcd-server.c  data.c  utils.c
    得到服务端程序fastcd-server
    $ gcc  -o fastcd-client fastcd-client.c
    得到客户端程序 fastcd-client

