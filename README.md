# 自定义mars

## 说明

由于mars只能保持一个长连接，在需要保持多个长连接的的情况下，只好引入多一个mars库，通过修改类名、命名空间等，来避免符号重复。

搞多一个库出来的步骤

1. marsksim的文件夹里所有文件里的ksim这部分改成你自定义名字，包括marsksim文件夹的名字;

2. 跑build_ios.py脚本，选1，就生成了;

## iOS

需要把iOS里所有ksim这部分改成你自定义名字，包括文件名;

里面的文件不是必要，自己看需参考

## 其他

iOS导入mars需要SystemConfiguration, libresolv, libz, CoreTelephony, 这个其实原来的mars里也有说明，更具体的问题看 <https://github.com/Tencent/mars/wiki/Mars-iOS%EF%BC%8FOS-X-%E6%8E%A5%E5%8F%A3%E8%AF%A6%E7%BB%86%E8%AF%B4%E6%98%8E>

## 问题

这只是个临时的解决方案，这个方法由于会加多mars库，包会明显增大，如果需要保持很多长连接，这个方法明显就不合适了。最好还是能从源码上修改成支持多个长连接。

## Author

azusalee, 384433472@qq.com

