# MeowEngine

## 特性

### 渲染

1. 前向渲染/延迟渲染
2. PBR+IBL Pass
3. 天空盒 Pass
4. 半透明 Pass

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

## TODO

### 合理管理资源绑定

因为资源绑定以 descriptor set 为单位。

而每个 material 需要的 set 之间的兼容性是无法提前知道的。

比如材质 A, B 之间可能共享 0, 1, 2 号 set，他们只在 3 号 set 有不同。

那么在渲染 A, B 之前，可以先绑定 0, 1, 2 号 set，然后各自绑定 3 号 set。

但是之后新增了材质 C，他的 0 号 set 和 A, B 相同，但是 1 号 set 已经不同了，那么从 1 号 set 开始就要重新绑定了。

如果让 material 编写者自己判断，什么时候从几号 set 开始重新绑定，那简直是灾难了。

所以肯定是需要程序化绑定资源的。

最简单的方法就是不考虑。

其次就是根据更新频率，对一个材质中的各个 set 排序。这是一个好的想法，可以尽量减少材质之间 set 不兼容的问题，只是无法避免。

于是还是需要有一个 manager 获取材质的 set 信息，根据这些 set 信息，协调各个材质之间的绘制顺序，使得 `vkCmdBindDescriptorSets` 次数最少

但是 `vkCmdBindDescriptorSets` 的开销是否真的很大，需要来特意做优化？不知道……

### 将材质创建与 RenderPass 解耦

目前材质创建需要知道

1.color attachment 数量，以创建对应数量的 `VkPipelineColorBlendStateCreateInfo`

2.subpass 序号，以创建 `VkGraphicsPipelineCreateInfo`

不知道是否还需要别的信息，但是总之材质和 pass 也是耦合的，无法完全独立于 pass 创建

### 切换渲染管线时的资源管理

需要重新根据新渲染管线要求的顶点属性来加载物体

不知道别人是怎么做的？直接把这个物体身上所有可能的属性全部加载进来？

并且材质也是依赖于 pass 的，所以也不能直接迁移

Image 的 Layout 转换，一般是在初始化的时候需要显式转换，在 subpass 之间的依赖关系中写出 subpass 切换时进行的隐式的转换。如果用 Dynamic Rendering 才需要在 subpass 之间做显式转换。不知道有没有别的需要显式控制的场景。

### 如何显示 GPU 和主存之间的带宽

例如 MSAA 需要在 GPU 中创建 4x Depth 和 4x Color，如果之后的 Pass 不需要 Depth，那么仅仅把 Color 写回到主存就可以呈现了

但是我们可以利用硬件的 resolve，使得 4x Color 在写回主存的时候直接 resolve，那么写入的数据量只是 1x Color

如果不启动硬件 resolve，那么光照 Pass 渲染完 4x Color 还要写回主存，主存再把 4x Color 写回 GPU 单独做 resolve 就使得带宽增加了四倍

于是需要有一个东西来判断当前带宽，来观察是否配置正确，使得 MSAA 利用了硬件 resolve