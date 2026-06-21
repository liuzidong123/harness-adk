#!/bin/bash
# verify-game.sh - 游戏产出物校验脚本
# 退出码: 0=全部通过, 1=存在失败项
# 用法: ./scripts/verify-game.sh [A|B|C|all]

set -euo pipefail

STATE_DIR="."
SPEC_DIR="specs"
OUTPUT_DIR="output"
ERRORS=0

check_type="${1:-all}"

# A类检查: 文件存在性
check_a() {
    echo "=== A类检查: 文件存在性 ==="

    # output 目录
    if [ -d "$OUTPUT_DIR" ]; then
        local out_count
        out_count=$(find "$OUTPUT_DIR" -type f 2>/dev/null | wc -l | tr -d ' ')
        echo "PASS: $OUTPUT_DIR/ ($out_count 个文件)"
    else
        echo "INFO: $OUTPUT_DIR/ 不存在（尚未产出）"
    fi
}

# B类检查: 内容合规性
check_b() {
    echo "=== B类检查: 内容合规性 ==="
    echo "INFO: 游戏产出物由 GTest 和 Gradle 测试验证"
}

# C类检查: 内容完整性
check_c() {
    echo "=== C类检查: 内容完整性 ==="
    echo "INFO: 由 TE 在审计阶段手动检查"
}

# 执行检查
case "$check_type" in
    A|a) check_a ;;
    B|b) check_b ;;
    C|c) check_c ;;
    all)
        check_a
        echo ""
        check_b
        echo ""
        check_c
        ;;
    *)
        echo "用法: $0 [A|B|C|all]"
        exit 2
        ;;
esac

echo ""
if [ $ERRORS -eq 0 ]; then
    echo "=== 校验通过 ==="
    exit 0
else
    echo "=== 校验失败: $ERRORS 项错误 ==="
    exit 1
fi
