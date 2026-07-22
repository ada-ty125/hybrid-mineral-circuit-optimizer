# 工业多矿物选矿电路混合遗传寻优引擎

这是 Cuprite Team 小组项目的求职展示版：使用 C++20 与 OpenMP，对选矿电路的离散网络拓扑和连续运行参数进行联合优化。仓库保留完整团队提交历史、MIT 许可证和全部成员署名，便于核验个人贡献。

[English README](README.md) · [基准测试方法](docs/BENCHMARKS.md) · [个人贡献记录](docs/CONTRIBUTIONS.md)

![算法 4 基准测试](docs/assets/algo4_business_report.png)

## 可核验结果

在相同目标函数、种群 `2000`、`200` 代、每组 5 次重复的本机测试中：

| 指标 | 基线 | 混合 GA | 变化 |
|---|---:|---:|---:|
| 平均经济效益得分 | 323.34 | 770.13 | **2.38 倍** |
| 历史最高得分 | 361.93 | **812.93** | +124.6% |
| 平均运行时间 | 261.87 秒 | **58.84 秒** | **4.45 倍加速** |

以上是 Apple Silicon 单机实验结果，不代表所有硬件或参数下的普遍结论。原始 CSV、统计口径和环境见 [docs/BENCHMARKS.md](docs/BENCHMARKS.md)。

## 我的工作范围

Tianyu Yang 负责 GA 核心工作流，包括混合离散/连续搜索、OpenMP 并行、早停、约束失败时的独立子代捕获和 Explorer 回退、停滞期 2.5 倍突变震荡、增量评价，以及后续的可复现实验与四版本消融基准。模拟器、经济模型、合法性检查、可视化和系统集成属于团队共同成果。

详细的原始提交证据见 [docs/CONTRIBUTIONS.md](docs/CONTRIBUTIONS.md)。

## 核心设计

- 离散拓扑使用多点交叉与合法性约束，连续参数使用线性交叉与高斯变异；
- 两个子代分别判断和捕获，避免联合概率失败导致整对子代退化；
- 多轮无效后注入全新合法 Explorer，避免近亲克隆和局部停滞；
- OpenMP 线程使用独立 `std::mt19937`，避免共享随机数锁与数据竞争；
- 父代保留已缓存适应度，仅对新子代执行昂贵物理模拟；
- 支持固定拓扑与可变单元类型的 swappable 模式；
- 支持 `GA_SEED` 和收敛 CSV 输出，方便复现实验。

## 构建与运行

macOS 的 AppleClang 用户需先执行 `brew install libomp`；CMake 会自动识别 Apple Silicon 与 Intel Homebrew 路径。

```bash
git clone --recursive https://github.com/ada-ty125/hybrid-mineral-circuit-optimizer.git
cd hybrid-mineral-circuit-optimizer
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure

GA_SEED=20260521 ./build/bin/Circuit_Optimizer \
  --population-size 100 --max-iterations 200 \
  --crossover-probability 0.9 --mutation-probability 0.01 \
  --mode fixed --output optimal_circuit.png
```

完整参数请运行 `./build/bin/Circuit_Optimizer --help`。

## 署名与许可

本仓库不是个人独立项目，而是团队项目的个人作品集版本。完整团队名单和版权信息保留在 [LICENSE](LICENSE) 与 Git 历史中，采用 MIT License。
