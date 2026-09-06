# DeepSeek Ray Tracing Benchmark

任务文件位于 `workspace/`，只读测试位于 `test_files/data/`，评测入口为
`eval/test_by_code.py`。详细要求见 `workspace/README.md`。

在完整克隆后先初始化依赖：

```bash
git submodule update --init --recursive
cd eval
python3 test_by_code.py
```

评测结果写入 `eval/code_result.json`。
