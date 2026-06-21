# 优化计划：消除冗余、清理历史遗�?
## 约束
- 不增�?CLAUDE.md 文件大小（当�?98 行）
- 不改变框架功能行�?- skills/*.md 保持为执行权�?
---

## Step 1: 删除无用文件（P0�?
- 删除 `templates/ppt-templates/act-files/` 目录�?0MB 二进制垃圾）
- 删除空目�?`reference/ppt-demo/`
- �?`.gitignore` 中添�?`templates/ppt-templates/act-files/`

## Step 2: 精简 .clinerules（P1�?
当前 .clinerules §1-6 �?CLAUDE.md 逐字重复。改为：
- 保留文件头说明（1 行引�?CLAUDE.md�?- 删除 §1-6 重复内容
- 保留 §7 角色切换（Cline 特有�?- 保留 §8 辅助 Skill（Cline 特有�?
预计�?103 行缩减到 ~30 行�?
## Step 3: 提取日志规则到独立文件（P1�?
由于不能增加 CLAUDE.md 大小，创�?`templates/logging-standard.md` 作为日志规则的单一定义�?```
# 过程日志标准

追加日志�?`log/process.log`
格式：`[{timestamp}] [{角色}] {事件描述}`
timestamp：优�?`date -u +%Y-%m-%dT%H:%M:%SZ`；不可用时用递增序号 `#NNN`
```

然后�?7 �?skill 文件中的日志规则替换为一行引用：
```
**日志规则�?* �?`templates/logging-standard.md`
```

涉及文件�?- skills/agh-clarify.md �?5 �?- skills/agh-propose.md �?5 �?- skills/agh-apply.md �?5 �?- skills/agh-archive.md �?5 �?- skills/mh-ppt.md �?6 �?- skills/dev-test.md �?5 �?- skills/post-verify.md �?5 �?
## Step 4: 精简 README.md（P2�?
1. 更新 §2 标题 "五个Agent角色" �?"六个Agent角色"
2. 更新 §角色隔离 "五个 Agent 角色" �?"六个 Agent 角色"
3. 删除末尾重复的规则段落（�?282-318 行：特别约束 + 角色隔离 + Handoff 协议），这些内容已在 CLAUDE.md 中完整定�?4. 在删除位置添加一行引用：`> 全局纪律与角色隔离规则见 CLAUDE.md`

## Step 5: 精简 docs/workflow.md（P2�?
保留有价值的图表部分（流程总览图、时序图、状态机图、修复循环图），删除�?CLAUDE.md/skills 重复的文字规则：
- 删除 "通用规则" 整节（第 177-280 行），包含：
  - 角色切换指令格式（skills 中已有）
  - Handoff 协议（design.md §4 已有�?  - 心跳打印规则（CLAUDE.md §1 + agents/pm.md 已有�?  - 过程日志规则（新建的 logging-standard.md 已有�?  - 断点恢复协议（CLAUDE.md §5 已有�?  - 异常处理（skills 中已有）
  - 人工审批呈现格式（skills 中已有）
  - Token 节流规则（design.md 已有�?- 保留末尾�?各阶段详细执行序�?表格（有用的索引�?- 在删除位置添加引用：`> 通用规则�?CLAUDE.md，各阶段执行细节�?skills/*.md`

预计�?297 行缩减到 ~180 行�?
## Step 6: docs/design.md 加权威声明（P3�?
在文件头部（�?1 行后）添加：
```
> ⚠️ 执行权威�?skills/*.md �?agents/*.md。本文档为设计参考，如有冲突�?skills/agents 为准�?```

## Step 7: 更新 check-harness.sh

check-harness.sh 会检查文件完整性。需要：
- 添加 `templates/logging-standard.md` 到检查列�?- 确认 .clinerules 仍在检查列表中（文件保留，只是内容精简�?
---

## 不动的文�?- CLAUDE.md（不增加大小�?- skills/*.md 的执行逻辑（只改日志规则行�?- agents/*.md
- scripts/verify.sh, baseline.sh, verify-ppt.sh
- templates/（除新增 logging-standard.md�?- .claude/commands/*.md（平台要求的入口�?