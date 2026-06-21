#!/bin/bash
# verify.sh - 产出物校验脚本
# 退出码: 0=全部通过, 1=存在失败
# 用法: ./scripts/verify.sh [A|B|C|all] [feature-name]

set -euo pipefail

SPEC_DIR="specs"
OUTPUT_DIR="output"
ERRORS=0

check_type="${1:-all}"
feature_name="${2:-}"

# 从.state.md 读取 feature_name
if [ -z "$feature_name" ]; then
    feature_name=$(grep "^feature_name:" ".state.md" 2>/dev/null | awk '{print $2}' || echo "")
fi

if [ -z "$feature_name" ]; then
    echo "WARN: 未指定 feature_name 且无法从 .state.md 读取，部分检查将跳过"
fi

# 读取 mode 字段
get_mode() {
    if [ -f ".state.md" ]; then
        grep "^mode:" ".state.md" 2>/dev/null | awk '{print $2}' || echo ""
    else
        echo ""
    fi
}

# A类检查 - 基础文件存在性
check_a() {
    echo "=== A类检查 - 文件存在性 ==="

    # 状态文件
    local f=".state.md"
    if [ ! -f "$f" ]; then
        echo "FAIL: $f 不存在"
        ERRORS=$((ERRORS + 1))
    elif [ ! -s "$f" ]; then
        echo "FAIL: $f 为空"
        ERRORS=$((ERRORS + 1))
    else
        echo "PASS: $f"
    fi

    # 特性级别文件
    if [ -n "$feature_name" ]; then
        local req_files=(
            ".state.md"
            "specs/proposal.md"
        )
        for f in "${req_files[@]}"; do
            if [ ! -f "$f" ]; then
                echo "FAIL: $f 不存在"
                ERRORS=$((ERRORS + 1))
            elif [ ! -s "$f" ]; then
                echo "FAIL: $f 为空"
                ERRORS=$((ERRORS + 1))
            else
                echo "PASS: $f"
            fi
        done
    fi
}

# B类检查 - 阶段产出物完整性（mode + output_type 感知）
check_b() {
    echo "=== B类检查 - 阶段产出物完整性 ==="

    if [ -z "$feature_name" ]; then
        echo "SKIP: 无 feature_name，无法执行B类检查"
        return
    fi

    local phase mode output_type test_strategy
    phase=$(grep "^phase:" ".state.md" 2>/dev/null | awk '{print $2}' || echo "")
    mode=$(get_mode)
    output_type=$(grep "^output_type:" ".state.md" 2>/dev/null | awk '{print $2}' || echo "")
    test_strategy=$(grep "^test_strategy:" ".state.md" 2>/dev/null | awk '{print $2}' || echo "")

    echo "INFO: phase=$phase, mode=$mode, output_type=$output_type, test_strategy=$test_strategy"

    # propose 阶段产物检查（propose/apply/archive 都需要）
    if [ "$phase" = "propose" ] || [ "$phase" = "apply" ] || [ "$phase" = "archive" ]; then
        # SA design.md - required for standard/full
        if [ "$mode" != "fast" ]; then
            if [ ! -s "specs/design.md" ]; then
                echo "FAIL: specs/design.md 缺失或为空"
                ERRORS=$((ERRORS + 1))
            else
                echo "PASS: specs/design.md"
            fi
        fi

        # BA requirement-spec.md - only full mode
        if [ "$mode" = "full" ]; then
            if [ ! -s "specs/requirement-spec.md" ]; then
                echo "FAIL: specs/requirement-spec.md 缺失或为空"
                ERRORS=$((ERRORS + 1))
            else
                echo "PASS: specs/requirement-spec.md"
            fi
        fi

        # TE testcases.md - standard/full, skip for manual/none test_strategy
        if [ "$mode" != "fast" ]; then
            if [ "$test_strategy" != "manual" ] && [ "$test_strategy" != "none" ]; then
                if [ ! -s "test/test-cases.md" ]; then
                    echo "FAIL: test/test-cases.md 缺失或为空"
                    ERRORS=$((ERRORS + 1))
                else
                    echo "PASS: test/test-cases.md"
                fi
            else
                echo "INFO: test_strategy=$test_strategy, testcases.md 非必需"
            fi
        fi

        # plan-action.md - always required
        if [ ! -s "specs/plan-action.md" ]; then
            echo "FAIL: specs/plan-action.md 缺失或为空"
            ERRORS=$((ERRORS + 1))
        else
            echo "PASS: specs/plan-action.md"
        fi
    fi

    # apply 阶段产物检查（apply/archive 都需要）
    if [ "$phase" = "apply" ] || [ "$phase" = "archive" ]; then
        if [ ! -d "output" ] || [ -z "$(ls -A "output" 2>/dev/null)" ]; then
            echo "FAIL: output/ 为空"
            ERRORS=$((ERRORS + 1))
        else
            echo "PASS: output/ 非空"
        fi

        # output_type-specific checks
        case "$output_type" in
            ppt)
                if [ ! -s "specs/ux/ux-design.md" ]; then
                    echo "FAIL: slide-spec.md 缺失（output_type=ppt）"
                    ERRORS=$((ERRORS + 1))
                else
                    echo "PASS: slide-spec.md 存在（output_type=ppt）"
                fi
                ;;
            backend-api|cli-tool|library)
                if ! find "output" -type f \( -name "*.py" -o -name "*.go" -o -name "*.java" -o -name "*.rs" -o -name "*.js" -o -name "*.ts" -o -name "*.rb" -o -name "*.c" -o -name "*.cpp" \) 2>/dev/null | grep -q .; then
                    echo "WARN: output_type=$output_type 但未检测到源代码文件"
                fi
                ;;
            *)
                echo "INFO: output_type=$output_type, 使用通用产出物检查"
                ;;
        esac
    fi

    # archive 阶段产物检查
    if [ "$phase" = "archive" ]; then
        if [ "$mode" != "fast" ]; then
            if [ ! -s "$SPEC_DIR/design.md" ]; then
                echo "FAIL: $SPEC_DIR/design.md 缺失或为空"
                ERRORS=$((ERRORS + 1))
            else
                echo "PASS: $SPEC_DIR/design.md"
            fi
        fi
        if [ "$mode" = "full" ]; then
            if [ ! -s "$SPEC_DIR/requirement-spec.md" ]; then
                echo "FAIL: $SPEC_DIR/requirement-spec.md 缺失或为空"
                ERRORS=$((ERRORS + 1))
            else
                echo "PASS: $SPEC_DIR/requirement-spec.md"
            fi
        fi
    fi

    if [ "$phase" != "propose" ] && [ "$phase" != "apply" ] && [ "$phase" != "archive" ]; then
        echo "INFO: phase=$phase，跳过B类检查"
    fi
}

# C类检查 - 流程一致性
check_c() {
    echo "=== C类检查 - 流程一致性 ==="

    if [ -z "$feature_name" ]; then
        echo "SKIP: 无 feature_name，无法执行C类检查"
        return
    fi

    if [ ! -f ".state.md" ]; then
        echo "FAIL: .state.md 不存在，无法校验流程"
        ERRORS=$((ERRORS + 1))
        return
    fi

    # .state.md 必填字段校验
    local required_fields="mode phase current_step current_role last_updated"
    for field in $required_fields; do
        if ! grep -q "^${field}:" ".state.md" 2>/dev/null; then
            echo "FAIL: .state.md 缺少必填字段: $field"
            ERRORS=$((ERRORS + 1))
        fi
    done

    # phase 与产物一致性校验
    local phase
    phase=$(grep "^phase:" ".state.md" 2>/dev/null | awk '{print $2}' || echo "")
    local mode
    mode=$(get_mode)

    if [ "$phase" = "apply" ] || [ "$phase" = "archive" ] || [ "$phase" = "done" ]; then
        if [ ! -s "specs/plan-action.md" ]; then
            echo "FAIL: phase=$phase → plan-action.md 缺失（propose 阶段产物不完整）"
            ERRORS=$((ERRORS + 1))
        fi
    fi

    if [ "$phase" = "done" ]; then
        if [ ! -d "$OUTPUT_DIR" ] || [ -z "$(ls -A "$OUTPUT_DIR" 2>/dev/null)" ]; then
            echo "FAIL: phase=done → $OUTPUT_DIR/ 为空（归档未完成）"
            ERRORS=$((ERRORS + 1))
        fi
    fi

    # Handoff 文件统计
    local handoff_dir="handoffs"
    if [ -d "$handoff_dir" ]; then
        local handoff_count
        handoff_count=$(find "$handoff_dir" -name "*.md" -not -name ".*" | wc -l)
        echo "INFO: 共 $handoff_count 个 handoff 文件"

        # fast 模式至少 2 个 handoff（DEV + TEST），standard/full 至少 propose 阶段的 handoff
        if [ "$mode" = "fast" ] && [ "$phase" = "done" ] && [ "$handoff_count" -lt 2 ]; then
            echo "WARN: fast 模式 phase=done → handoff 数量不足（期望>=2，实际=$handoff_count）"
        fi
    fi

    echo "PASS: 流程一致性检查完整"
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
        echo "用法: $0 [A|B|C|all] [feature-name]"
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
