# /log-analysis

基于 Spec 关键字的 adb logcat 日志分析。根据问题描述，定位关联 Spec，提取代码关键字，从 logcat 中过滤并分析问题根因。

## 用法

```
/log-analysis 问题描述: {用户描述的问题}
```

## 流程

1. 解析问题描述 → 提取领域、症状、关键词
2. SpecService 搜索关联 Spec → KnowledgeService 查询历史案例
3. 从 Spec 提取代码关键字（TAG/类名/方法名/错误码）
4. adb logcat 获取日志 → 按关键字过滤
5. 模式匹配 + 时序分析 → 根因定位 → 关联回 Spec
6. 输出分析报告（含修复建议）

详见 `skills/log-analysis.md`
