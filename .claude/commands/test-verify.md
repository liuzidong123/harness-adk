# /test-verify

基于 Feature/Spec 的 adb 设备端 TDD 测试验证。黑盒测试（需求验证）+ 白盒测试（代码覆盖）→ 生成测试报告。

## 用法

```
/test-verify feature: {Feature 标识}
/test-verify spec: {Spec 标识}
/test-verify spec: {Spec 标识} --black-box --white-box
```

## 流程

1. SpecService 读取 Spec SHALL/GWT + FeatureService 读取 Feature 配置
2. 从 SHALL + Feature 边界派生黑盒用例；从 CodeGraphService CFG 派生白盒用例
3. adb 模拟输入 + 截图/日志/状态验证（黑盒）
4. adb instrumented test + 覆盖率收集（白盒）
5. 生成测试报告（关联 Spec + 覆盖率 + 结论）

详见 `skills/test-verify.md`
