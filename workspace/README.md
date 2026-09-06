# Task 1 — JSON Scene CPU Ray Tracer

从零实现一个读取 JSON 场景并输出 PNG 的纯 C++17 CPU ray tracer。只检查最终图片；本目录
不提供渲染基础代码。可以在 `src/` 和 `include/` 下创建任意数量的 `.cpp`、`.h` 文件，
但程序必须且只能定义一个 `main`。

## 编译与运行约定

构建程序使用本目录中的 `CMakeLists.txt`，递归收集 `src/` 下的所有 `.cpp` 文件，将它们一次性
编译并链接成 `raytracer_submission`，固定使用 C++17 且关闭编译器扩展。编译器会提供以下
include 路径：

- `include`
- `external/glm`
- `external/stb`
- `external/json/include`

因此可以直接使用 `<glm/glm.hpp>`、`<stb_image_write.h>` 和
`<nlohmann/json.hpp>`。依赖文件已经完整提供在本目录中。不能依赖网络、GPU、窗口系统、
预安装的非标准库或本目录之外的文件。

可执行程序必须支持：

```text
raytracer_submission <scene.json> <output.png>
```

成功时返回 0，并在指定路径写出尺寸正确的 8-bit RGBA PNG；失败时返回非零值。输入路径、
输出路径、工作目录均不固定，不能根据文件名或工作目录选择预制结果。程序必须完整读取传入
的场景文件，并允许覆盖已经存在的输出文件。

## 坐标与射线定义

- 使用右手世界坐标系。相机朝向 `target-eye`，相机右方向为
  `normalize(cross(forward, up))`，修正后的上方向为 `cross(right, forward)`。
- 图像原点位于左上角，x 向右、y 向下。像素中心和分层采样位置由下文公式定义。
- 射线为 `p(t)=origin+t*direction`。任何内部或公开使用的非零 direction 都必须先归一化；
  因而 t 是世界空间距离。
- 球和三角形均双面可见。命中法线使用 face-forward：必须满足
  `dot(normal, ray.direction) <= 0`。从球内击中球面时应翻转几何外法线。
- 有效交点包含闭区间 `[t_min,t_max]` 内的最近交点。

## JSON 场景标准（version 1）

所有数组均按 `[x,y,z]` 排列，颜色使用线性 RGB，数值以单精度浮点计算。未知字段应忽略。
输入场景均遵守以下格式。场景文件放在本目录的 `scenes/` 中；对应的参考图片放在
`reference/` 中。程序运行时会收到场景文件的实际路径。

```json
{
  "version": 1,
  "image": {"width": 320, "height": 240, "samples_per_axis": 2},
  "camera": {
    "eye": [6, 4, 8], "target": [0, 1, 0], "up": [0, 1, 0],
    "vertical_fov_degrees": 40
  },
  "render": {"max_bounces": 8, "ray_epsilon": 0.001},
  "ambient": [0.1, 0.1, 0.1],
  "materials": {
    "white": {"type": "lambertian", "albedo": [0.8, 0.8, 0.8]},
    "mirror": {"type": "specular", "albedo": [0.9, 0.9, 0.9]}
  },
  "spheres": [{"center": [0, 1, 0], "radius": 1, "material": "mirror"}],
  "triangles": [{
    "vertices": [[-10, 0, -10], [10, 0, -10], [-10, 0, 10]],
    "material": "white"
  }],
  "lights": [{"position": [3, 8, 4], "power": [500, 500, 500]}]
}
```

约束如下：

- `version` 固定为 1。
- `width,height` 为正整数；`samples_per_axis` 为正整数。每像素样本数为其平方。
- `eye != target`，`up` 不与观察方向平行，`0 < vertical_fov_degrees < 180`。
- `max_bounces` 为正整数；`ray_epsilon > 0`。
- material 名称唯一，`type` 只会是 `lambertian` 或 `specular`，几何体引用的名称一定存在。
- 球半径为正；三角形不退化；所有 direction 均非零。

## 确定性成像标准

令 `s=samples_per_axis`，对像素 `(x,y)` 的每个 `sx,sy in [0,s)` 使用：

```text
sample_x = x + (sx + 0.5) / s
sample_y = y + (sy + 0.5) / s
ndc_x = (2*sample_x/width - 1) * (width/height) * tan(fov_y/2)
ndc_y = (2*sample_y/height - 1) * tan(fov_y/2)
direction = normalize(forward + right*ndc_x - camera_up*ndc_y)
```

Lambertian 命中的线性颜色为：

```text
albedo * (ambient + sum(power / distance^2 * max(dot(normal,to_light),0)))
```

若 `[ray_epsilon,distance-ray_epsilon]` 内有任意遮挡，忽略该灯。阴影和反射射线从
`hit_point + normal*ray_epsilon` 出发。Specular 使用理想镜面反射，吞吐量逐次乘以 albedo；
首次 Lambertian 命中返回吞吐量乘其颜色，逃逸返回吞吐量乘 ambient，达到
`max_bounces` 返回黑色。

所有子样本取算术平均。输出前逐通道 clamp 到 `[0,1]`，不做 gamma 或 tone mapping，使用
`round(value*255)` 转换为 8-bit；alpha 固定为 255。

## 评测

每个 JSON 场景只比较程序输出的 PNG 与 `reference/` 中的参考图片。比较允许少量浮点和边缘
像素差异，不会调用、链接或检查任何内部求交、着色函数。编译成功本身不构成功能得分。
