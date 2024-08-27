# MeowEngine

## 编译依赖

大部分第三方库被添加为 submodule，加入构建系统，因此第一次构建的时间会比较久。

需要用户自行安装的第三方库有：

1. VulkanSDK

2. LLVM

其中 VulkanSDK 在安装时会配置好环境变量

LLVM 安装后需要自行配置环境变量 `LLVM_DIR` 为 LLVM 根目录（这并不是为了从源码构建，所以这个环境变量与 LLVMConfig.cmake 无关）

## 编译着色器

```shell
glslangValidator -V .\builtin\shaders\quad.frag -o .\builtin\shaders\quad.frag.spv
```

## 特性

### 异常安全

使用 `sd::swap` 实现赋值构造和移动构造