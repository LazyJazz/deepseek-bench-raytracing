# Simple Ray Tracer

这是一个使用 LongMarch 图形框架和 HLSL 像素着色器的独立小型 Ray Tracing Demo。程序创建一个
窗口，在全屏四边形上执行 CPU 提供场景数据、GPU 执行光线求交和着色的渲染流程。

项目内容：

- `gui/main.cpp`：创建示例场景并启动渲染窗口；
- `lib/`：场景数据结构、GPU 资源绑定和完成的 Ray Tracing shader；
- `external/LongMarch/`：图形框架；
- `external/glm/`、`external/stb/`：数学和 PNG 输出依赖。

## 构建

需要 C++17 编译器、Vulkan SDK、支持的图形设备，以及一个已安装依赖的 vcpkg。LongMarch
本身保留为 submodule，初始化全部 submodule 后配置：

```bash
git submodule update --init --recursive
cmake -S workspace -B build \
  -DVCPKG_PATH=/path/to/vcpkg \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

`VCPKG_PATH` 中应能解析 LongMarch 所需的 `fmt`、`freetype`、`eigen3`、`glfw3`、`glm`、
`vulkan-memory-allocator`、`stb`、`mikktspace` 和 `tinyobjloader` 包。Vulkan 的 shader 编译
工具由 Vulkan SDK 提供。

## 运行

```bash
./build/simple_raytracer
```

程序默认打开 `1280×720` 窗口。按住鼠标左键旋转视角，W/A/S/D 移动，空格和左 Ctrl 沿垂直
方向移动。关闭窗口后退出；渲染器也会把最后一帧保存为当前目录下的
`raytracing_result.png`。

## 渲染约定

场景由球体、三角形、点光源和两类材质组成。射线求交使用最近正根；三角形采用
Möller–Trumbore 算法；命中法线始终翻转到与入射方向相反的一侧。Lambertian 表面包含环境光、
平方反比点光源和硬阴影；Specular 表面执行最多八次理想镜面反射，吞吐量逐次乘以反射材质
的 albedo。射线起点使用法线方向的 epsilon 偏移以避免自相交。

像素着色器使用 `4×4` 子像素采样，输出为线性空间的 RGBA8 颜色并固定 alpha 为 1。
