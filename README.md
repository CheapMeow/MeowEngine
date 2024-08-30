# MeowEngine

## 编译

### 第三方库

`ExternalProject_Add` 在 install 上出现问题。自行复制 dll 来模仿 install，会出现 dll 缺失的问题。`vcpkg` 可能在查找路径上出现问题。最终还是 `submodule` 最稳定

需要用户安装的第三方库为

VulkanSDK

LLVM

### 环境变量

需要用户配置的环境变量为

`VK_SDK_PATH` `VULKAN_SDK`

`LLVM_DIR`

### 编译着色器

```shell
glslangValidator -V .\builtin\shaders\quad.frag -o .\builtin\shaders\quad.frag.spv
```

## 特性

### Cpp 静态反射

使用 `LLVM` 中的 `libclang` 解析头文件，识别 cpp `attribute` 属性

属性为 `clang::annotate` 的类会被识别到，其中含有相同属性的字段和方法可以获得名字

已知被反射的类的名称，字段和方法的名称，使用代码生成，生成反射代码

该反射代码提取出类的成员变量指针，成员函数指针，存到 lambda 中，这个 lambda 接受 `void*`，`static_cast` 成被反射的类型。这样就完成了反射信息在 cpp 中的存储。

外部使用反射接口时，传入类型名称，可以获得对应的 `TypeDescriptor`，其中存储字段信息 `FieldAccessor` 和方法信息 `MethodAccessor`

向 `FieldAccessor` `MethodAccessor` 传入 `void*` 类型的实例指针，就能获得这个实例对应的成员和方法的指针

因为只有 `static_cast`，所以类型不匹配时会报错中断

### SPIR-V 反射

使用 `SPIRV-Cross` 获取

## 开发指南

### 坐标系

`glm` 使用右手系

“使用某个手性”这个表述的意义在于，手性不同，进行向量叉乘时，叉乘得到的向量的值相同，但方向不同

