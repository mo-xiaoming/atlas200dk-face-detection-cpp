# 华为Atlas 200DK，几年后再见吗？

因为我像打不完的地老鼠一样不停的散布贬低甲方管理能力的负能量，甲方某人找我谈话的时候想拿华为的项目质量做对比。作为一个技术人员的我，回答说：“没接触过华为的项目，我不能做评价“。

经过两个星期的折腾，现在我可以评价：“至少就Atlas 200DK来说，当前的项目质量是不能让人满意的“

  [一步一坑](#one)
  [重写的face-detection](#two)

## 一步一坑<a name="one"></a>

### `Failed to import te.platform.log_util.`

按照"Atlas 200 DK 开发者套件（20.1）"的描述一步一步的去配置开发机的话，在atc命令行转换模型的时候会发生报错

```
ModuleNotFoundError: No module named '_struct'
ATC run failed, Please check the detail log, Try 'atc --help' for more information
E40000: Failed to import te.platform.log_util.
```

此时把`/usr/local/python3.7.5/bin`加在`PATH`的最前面就行了

### 连不上的摄像头

如果使用Mind Studio的话，开发板上电启动完毕后，要按一下板上的复位键，否则摄像头不能被认出

### 盖不上的上盖

开发板的盖子就不要盖了，敞开着让它吃灰吧。开发板有四个指示灯，上电时依次点亮表明不同的状态，复位时两个指示灯表示重启状态。上盖一旦盖上，就很难看清是哪个灯在闪，所以盖子就是个摆设。。。其实不是，它有非常重要的用途

### 颠倒的摄像头

观察一下树莓派Zero的摄像头和排线，非常自然。淘宝上买的大路货的支架也没什么可说的

![Pi Zero](https://github.com/mo-xiaoming/atlas200dk-face-detection-cpp/blob/master/jpgs/200dk.jpg?raw=true)

再看一下200DK的摄像头，一个夜视，一个普通，两个支架全都是倒着的。既然文档里用的摄像头就是树莓派兼容版，难道华为的人们就没有发现淘宝上销量最大的摄像头支架非得倒着才能用吗？

![200DK](https://github.com/mo-xiaoming/atlas200dk-face-detection-cpp/blob/master/jpgs/pi.jpg?raw=true)

这就是上面提到的上盖的用处，倒过来的摄像头支架正好靠在上面

### 风扇永不眠

开发板的风扇一旦上电就永不停歇，噪音不能说奇大，因为据论坛上说这已经是改进过的了

### 该死的`ada`

如果使用Mind Studio远程部署的话，每次开发板重启后都要手工`kill`掉`/var/ada`进程，然后在重启此进程，否则部署会统统失败

### 编程实践不友好的开发板配置

如果编译时带sanitizer的话，开发板上运行会报

```
==4489==ERROR: AddressSanitizer failed to allocate 0x200001000 (8589938688) bytes at address ffffff000 (errno: 12)
==4489==ReserveShadowMemoryRange failed while trying to map 0x200001000 bytes. Perhaps you're using ulimit -v
Aborted
```

这是因为制卡脚本会把开发板上的`/proc/sys/vm/overcommit_memory`的值设置为`0`，这样会内存不够。在root下，`echo 0 > /proc/sys/vm/overcommit_memory`就好了

### 不能带参数执行

如果用Mind Studio的话，当程序带参数运行时会报错。因为`MindStudio-ubuntu/tools/run.py`会生成在开发板上运行的`run.sh`，然后`run.sh`中参数放的地方错了

```
if [ ! -z "$result" ];then
  echo "[INFO]  pmupload exists."
  pmupload $currentdir/workspace_mind_studio_objectdetection_cvwithaipp
else
  export SLOG_PRINT_TO_STDOUT=0
  $currentdir/workspace_mind_studio_objectdetection_cvwithaipp
fi ARGS_AT_WRONG_PLACE #<<<<<<<<<<<<<<<
```

修改`MindStudio-ubuntu/tools/run.py`就行了，如果你能忍受Mind Studio的话

### opencv打不开视频文件

开发板上安装`libv4l-dev`然后重新编译opencv就行了，好像没有文档提到这一点。而且[官方安装opencv](https://gitee.com/ascend/samples/tree/master/common/install_opencv)的文档中python路径之类的也和20.1的200DK文档对不上

### 接显示器是不可能的

我几十块钱的树莓派Zero都可以接显示器，你几千块钱的200DK没有视频输出？现在有什么开发板不带视频输出吗？！曾经ssh一登陆就报`input/output error`然后闪退，弄得我完全没有办法，只能重新制卡。

### 例程完全谈不上代码质量

[官方的face detection](https://gitee.com/ascend/samples/tree/master/facedetection/for_atlas200dk_1.7x.0.0_c++)中，[camera.cpp](https://gitee.com/ascend/samples/blob/master/facedetection/for_atlas200dk_1.7x.0.0_c++/facedetection/src/camera.cpp#L122)，第122行

```
bool Camera::IsOpened(int channelID){
    if(1 < channelID)
        return false;
    return isOpened_[channelID]; // uninitialized before reading
}
```

当时我在改完全不相干的代码，然后发现运行时摄像头不打开了，然后发现这个bug，随后被gcc的undefined sanitizer确认

```
/home/HwHiAiUser/romote/src/camera.cpp:122:31:runtime error: load of value 170, which is not a valid value for type 'bool'
```

这说明这些代码完全没有经过工具坚持，也就是说没有任何值得一提的CI/CD

### 我以为这是C++接口

acl的API里到处是Create/Destroy, Load/Unload之类的调用，对不起，我以为这是C++接口呢。C++相对C的最重大的改进，就是RAII，我们不需要关心资源释放问题，因为语言本身会帮我们处理。看到ACL的API，我不由得怀疑，这究竟是道德的沦丧还是人性的扭曲，不对，到底是想赶一个过时的时髦，还是压根不知道C++应该怎么写。这堆C样子的C++接口给我们增加了多少开发成本，究竟这帮家伙有没有概念？

另外，如果用到摄像头的话，链接时会报libmedia_mini.so中的函数找不到，你会非常奇怪路径什么的都是正确的。其实，你只要做如下操作就好了

```cpp
extern "C" {
#include "peripheral_api.h"
}
```

任何文档都没告诉你Media库其实是C链接的，而不是C++

## 重写的face-detection <a name="two"></a>

相同的功能，重写了一遍，可以进一步重构，但是我实在是丧失兴趣了

### 编译

我是用了conan，配置的方法参见conan.io官方文档。C++项目应该使用包管理器，而不是手工折腾第三方库。很遗憾opencv的一个依赖目前在arm下编译有问题，所以opencv只能依赖手工编译的方式。如果你不用的话可以手工编译spdlog和可选的fmt。

更改`src/CMakeLists.txt`中的include和lib路径


```bash
cd build
conan install .. --build missing
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=/usr/bin/aarch64-linux-gnu-gcc-10 -DCMAKE_CXX_COMPILER=/usr/bin/aarch64-linux-gnu-g++-10
make -j
```

⚠️，我是用了gcc 10，因为我想用最新的编译器，因为它能提供更高的优化，更简练的代码，更...，这是2021年，没有理由卡在老编译器版本上。另外`CMakeLists.txt`里用了c++20的标准，如果你不想安装新编译器的话，需要把它改成17以下。并且有些地方会编译不过，例如`main.cpp`里，gcc7不支持`std::from_chars`，你必须改成`std::strtol`之类的。至于为什么要用更新的`std::from_chars`？因为它的性能更好，如我提到的那样，这种benchmark很多了

### 运行

运行前需要在另一台机器上打开presenter，`./script/run_presenter_server.sh`。当然需要配置好`./data/param.conf`，一如[原始文档](https://gitee.com/ascend/samples/tree/master/facedetection/for_atlas200dk_1.7x.0.0_c++)所示

```bash
cd ../out
./main 0
```

`./main`后面跟摄像头的channel，默认是`0`

