# MeowEngine

## 框架介绍

### Cpp 静态反射

我个人的静态反射实现思路是

使用 `LLVM` 中的 `libclang` 解析头文件，识别 cpp 头文件的类型、变量、函数声明中的 `attribute` 属性

`libclang` 对传入的每一个头文件的 AST 都执行如下操作：首先记录那些属性为 `clang::annotate` 的类，然后以这些类为根 cursor 开始遍历。含有 `clang::annotate` 的字段和方法被记录下来

已知被反射的类的名称，字段和方法的名称，就可以生成反射代码文件

主目标中已经写好了 `TypeDescriptor` 类，`TypeDescriptor` 类会提供注册反射信息的功能，其中存储字段信息 `FieldAccessor` 和方法信息 `MethodAccessor`。生成的反射代码注册反射信息，也就是提取出类的成员变量指针，成员函数指针，存到 lambda 中。这个 lambda 接受 `void*`，`static_cast` 成被反射的类型。这样就完成了反射信息在 cpp 中的存储。

外部使用反射接口时，传入 `std::string` 类型名称，可以从全局单例的 map 中获得对应的 `TypeDescriptor`。而已知 `TypeDescriptor`，就可以获得他其中存储的字段信息 `FieldAccessor` 和方法信息 `MethodAccessor` 列表

向 `FieldAccessor` `MethodAccessor` 传入 `void*` 类型的实例指针，调用存储的 lambda 就能获得这个实例对应的成员和方法的指针

因为只有 `static_cast`，所以类型不匹配时会报错中断，程序容错性会很差

关于更多实现细节、序列化应用等内容，见博客

### SPIR-V 反射

使用 `SPIRV-Cross` 获取

### Render Pass

`Render Pass` 的基类负责创建资源，如 `Model`, `Shader`, `Material`, `Render Target` 等

`Render Pass` 的派生类负责管理本 `Render Pass` 对资源的依赖、格式转换；各个 subpass 之间的资源依赖、格式转换；最终创建 `VkRenderPass`。然后负责在 `Render Pass` 绘制时，对每一个 `Material` 绑定，绘制。

## 构建

### 环境

使用 msvc 编译

需要用户在外部安装的第三方库为

1.VulkanSDK

2.LLVM

为了找到这些外部安装的库，需要用户配置的环境变量为

1.`VK_SDK_PATH` `VULKAN_SDK`

2.`LLVM_DIR`

### 编译着色器

```shell
glslangValidator -V .\builtin\shaders\quad.frag -o .\builtin\shaders\quad.frag.spv
```

