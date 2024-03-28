# MeowEngine

## 渲染数据结构

### Shader

#### 反编译顶点属性，缓冲区和纹理输入

需要有一个东西管理 `vk::raii::ShaderModule` 和对应的 `vk::ShaderStageFlagBits`

DescriptorSet 需要负责从内存接受描述符对应的资源，上传到 gpu

因此 Shader 存储自己的对应的 DescriptorSet

DescriptorSet 和 DescriptorSetLayout 如果由人来指定的话，就会很麻烦，需要人手设置与 shader 一一对应的值，并且容易出错

例如模型文件的顶点数据的结构，需要和 shader 的顶点数据结构相匹配

不匹配的结果就会是一团糟，比如原本模型文件里面只有 position 和 normal，现在你用一个 position, normal, color, uv 输入的顶点着色器

那么就会把原来是 position 和 normal 的数据读一部分到 color, uv，这就全部乱了

vertex 数据错误读取了，原来能画 100 个三角形的数据现在错误地读成了画 50 个的，那剩下 50 个三角形的绘制没有数据了，vulkan 也不会报错

所以这可能一开始令人一头雾水……明明没有报错，文件读取也没有问题……

创建管线的时候会设置顶点数据的 stride，这个也跟顶点数据结构有关

这些都会导致错误……所以这些确实应该有一个自动化配置的方法

所以需要用一个类负责反编译 spv，自动生成 DescriptorSetLayout

这里也就直接用 Shader 类来处理

目前的反编译是直接识别顶点属性的名称

所以 glsl 里面的顶点属性的名称必须是特定的值，例如 inPosition, inUV0

#### 使用 pool 数量可变的 Descriptor Allocator

创建 descriptor pool 的时候，可以初始指定可能需要的 descriptor 的种类和对应的数量。具体到 API 来说，这些是通过 pool size 来设置的

如果材质个数已知，那么其实 pool size 和 set count 都已知，那就不需要动态的 descriptor allocator 了

所以很多教程代码上面是直接通过某个 shader 对应的 descriptor set layout 算出对应的 pool size，得到的 descriptor pool 只用分配一次 descriptor，永远不会出错

因为 shader 数量是未知的，所以 descriptor set 的数量也是未知的

所以固定 set count 的 descriptor pool 可能会出错

所以使用动态大小的分配器，在 set count 不足的情况下动态创建 descriptor pool

那么这个动态大小的分配器内含的 descriptor pool 的初始的 pool size 怎么确定呢？既然现在你不想暴力通过某些 shader 的 descriptor set layout 来算

我感觉……应该没有标准做法，就是自己设定吧

我看到有一个做法是，在 `Shader` 类里面存动态数量的 descriptor pool 的

我在想，这个和有一个全局的 allocator，allocator 里面存很多个 descriptor pool，有什么区别

也就是，每个 `Shader` 有一堆 descriptor pool，和全局的一个 allocator 里面存一堆 descriptor pool，之间的区别

#### 分配 descriptor set

从 descriptor pool 创建 descriptor set 需要 descriptor set layout

这个 set layout 是存储在 `Shader` 中的，是 `Shader` 对 spv 反编译的结果，描述了 `Shader` 所需的资源

获取 descriptor set 这个操作应该是在渲染主循环里面，原则上应该是程序员来控制，而不是在某个 struct 里面？

从最简单的情景开始想起：

假设一个程序员现在已经把反编译 spv 创建 descriptor set layout 的流程写完了，除此之外就没有别的自动化逻辑了

那么他仍然需要知道，自己要读取什么 spv shader，仍然要 hard code 从哪个 spv shader 来获取 descriptor，然后再根据这个 descriptor 来写入纹理或者 buffer

或许之后能够达到：在编辑器里面创建任意数量的 `Shader`，都不用程序员自己 hard code 要读取什么了，你有什么我就读什么

当然这里肯定涉及到资源加载的问题，在游戏运行时要读取什么 `Shader`

假设先不管这个，那么之后肯定是可以有这么一个自动化的东西的

不管怎么说，创建 descriptor set 的这个位置，不管是 hard code 还是自动化流程，位置应该是在主循环的

#### 存储 descriptor set

根据 [vulkan_best_practice_for_mobile_developers/samples/performance/descriptor_management
/descriptor_management_tutorial.md](https://github.com/ARM-software/vulkan_best_practice_for_mobile_developers/blob/master/samples/performance/descriptor_management/descriptor_management_tutorial.md)

一般来说，只有当出现新的资源组合的时候，才需要创建新的 descriptor set

而不是在每帧渲染的时候 reset descriptor pool 然后重新为各个 `Shader` allocate descriptor set

那么其实"出现新的资源组合"就相当于出现新的 `Shader` 吧？

那么这种情况下，descriptor set 在主循环中获取，也应该是存储在全局的？

虽然我看大部分教程都是这么做的，但是我感觉这样做是不是不太好

因为本质上这也是一个 hard code，你需要写下

```cpp
class Renderer{
public:
    VkDescriptorSet descriptor_set_0 = nullptr;
    VkDescriptorSet descriptor_set_1 = nullptr;
    ...

    void Init(){
        // Create shader
        ...

        // Allocate descriptor set

        descriptor_set_0 = m_Shader0->AllocateDescriptorSet();
        descriptor_set_1 = m_Shader1->AllocateDescriptorSet();

        // Write descriptor set
        ...
    }
};
```

当然，如果用 vector 的话，似乎也不是不能接受？

```cpp
class Renderer{
public:
    std::vector<VkDescriptorSet> descriptor_set_vec;
    ...

    void Init(){
        // Create shader
        ...

        // Allocate descriptor set

        descriptor_set_vec.push_back(m_Shader0->AllocateDescriptorSet());
        descriptor_set_vec.push_back(m_Shader1->AllocateDescriptorSet());

        // Write descriptor set
        ...
    }
};
```

但是实际上我还是需要知道 descriptor set 和 `Shader` 的对应关系的，比如某个 `Shader` 需要写入 ubo 和一些特定的纹理，那我需要知道这个 `Shader` 对应的是哪个 descriptor set 才能导入

所以继续用容器的话，似乎还要用 map 啊之类的

这么一想似乎就没有必要，真的要保存对应关系那还不如用 struct。那就更进一步，为什么他不是一个类的部分？那更进一步，为什么不是 `Shader` 类的一部分？

所以最终还是决定把 descriptor set 放到 `Shader` 里面

#### 更新 descriptor set

`VkWriteDescriptorSet` 完成的是绑定的工作，核心就是，对于 buffer 是 `pBufferInfo，对于` texture 是 `pImageInfo`

`pBufferInfo` 需要缓冲区句柄，`pImageInfo` 需要 sampler 句柄，这就把资源句柄绑定到了 descriptor set 上面

这里是不涉及怎么更新资源本身的，比如摄像机在每帧运动，View 矩阵时刻在变，那么 UBO 这个 uniform buffer 应该每帧用 memory copy 来更新。资源数据的更新和 `VkWriteDescriptorSet` 这里的绑定不是一个东西。

pBufferInfo, pImageInfo 需要的 `VkDescriptorBufferInfo`, `VkDescriptorImageInfo` 可以提前创建好，所以有的教程会把它们和 `VkBuffer`, `VkImage` 都封装在一个类里面

但是我感觉，这个东西，在创建 `VkWriteDescriptorSet` 的时候创建就好了，这样就显得 `pBufferInfo`, `pImageInfo` 的意义比较明确

### DescriptorAllocatorGrowable

pool 数量可变的 descriptor set 分配器

内部存储了两个 vector，保存 descriptor pool。第一个 vector 是 `readyPools`，存储可用于分配 descriptor set 的 pool，第二个 vector 是 `fullPools`，存储已经分配满了的 pool

descriptor pool 的 `std::vector<vk::DescriptorPoolSize> pool_sizes` 都是相同的，暂时不去细究

### Uniform buffer

在哪里控制 uniform buffer 的 memory copy？

## 常见错误

### CreateInfo 可能引用了局部变量

比如某个 `CreateInfo` 需要 `&BufferInfo`

这个 `BufferInfo` 是当前函数的局部变量

在当前函数中创建完 `CreateInfo`，并不是立即使用，而是存到 `vector` 中留待后用

那么在离开函数的时候，`BufferInfo` 销毁，存在 `vector` 中的 `CreateInfo` 就出错了

别人引用了局部变量 `BufferInfo` 怎么没出错？因为他们创建出来 `CreateInfo` 之后就立即使用了

### 从 RAII 类转型成非 RAII 类

从 RAII 类转型成非 RAII 类，并且使用 `std::move` 来移动资源所有权，这种方法不一定能达成转移资源的目的

例如：

```cpp
// header file
std::vector<vk::DescriptorSet> descriptor_sets;

// source file
vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(
    *descriptor_pool, descriptorSetLayouts.size(), descriptorSetLayouts.data());
vk::raii::DescriptorSets raii_descriptor_sets(logical_device, descriptor_set_allocate_info);

for (size_t i = 0; i < raii_descriptor_sets.size(); ++i)
{
    descriptor_sets.push_back(*std::move(raii_descriptor_sets[i]));
}
```

关键在于转型到的非 RAII 类并不是出于 RAII 的目的设计的，所以可能不会提供移动构造函数，这样，即使使用了 `std::move`，其实仍然是把 `std::move` 产生的左值当作右值传给了拷贝构造函数

而拷贝构造函数一般只是共享了这个资源。因此，在离开作用域之后，RAII 类析构，其中的资源没有转移，因此就析构掉了资源，同时非 RAII 类即使没有被析构，其中的资源也已经无效了