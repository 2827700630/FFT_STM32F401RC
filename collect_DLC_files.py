import os
import shutil
from pathlib import Path
import re


def collect_files_and_generate_header(project_root):
    # 可自定义：只遍历这些顶级目录
    SEARCH_DIRS = [
        "Middlewares",
        "USB_DEVICE"
    ]

    # 目标文件夹
    DLCc_dir = os.path.join(project_root, "Core", "Src", "DLCc")
    DLCh_dir = os.path.join(project_root, "Core", "Inc", "DLCh")
    all_include_file = os.path.join(project_root, "Core", "Inc", "all_DLC_includes.h")

    Path(DLCc_dir).mkdir(parents=True, exist_ok=True)
    Path(DLCh_dir).mkdir(parents=True, exist_ok=True)

    collected_h_files = []

    for folder in SEARCH_DIRS:
        folder_path = os.path.join(project_root, folder)
        if not os.path.exists(folder_path):
            continue
        for root, dirs, files in os.walk(folder_path):
            for file in files:
                lower = file.lower()
                src_path = os.path.join(root, file)
                # .c 文件处理（自动修正 include 路径）
                if lower.endswith('.c'):
                    dst = os.path.join(DLCc_dir, file)
                    fix_include_paths(src_path, dst)
                    print(f"Copy & fix includes: {src_path} -> {dst}")
                # .h 文件处理
                elif lower.endswith('.h'):
                    dst = os.path.join(DLCh_dir, file)
                    shutil.copy2(src_path, dst)
                    print(f"Copy {src_path} -> {dst}")
                    if file not in collected_h_files:
                        collected_h_files.append(file)

    # 生成 all_dlch_includes.h 并包含所有头文件
    with open(all_include_file, "w", encoding="utf-8") as f:
        f.write("""
/**
* @author: 雪豹
* @version: 1.0
* @description: 自动生成的头文件，包含所有 DLCh 下的头文件。
* @note: 雪豹DLC管理器
* @date: 2025-04-22
*/
        """)
        f.write("\n")
        for hfile in collected_h_files:
            f.write(f'#include "DLCh/{hfile}"\n')
    print(f"\n生成汇总头文件: {all_include_file}")
    print("你只需在 main.c 里 #include \"all_dlch_includes.h\" 即可包含所有 USB 相关头文件。")


def fix_include_paths(src_file_path, dst_file_path):
    """
    读取src_file_path中的内容，将 #include "xxx.h" 替换为 #include "DLCh/xxx.h"，但如果是stm32开头则不替换，写入dst_file_path
    """
    include_re = re.compile(r'#include\s+"(\w+\.h)"')
    with open(src_file_path, "r", encoding="utf-8", errors="ignore") as f:
        lines = f.readlines()

    fixed_lines = []
    for line in lines:
        # 只替换不含斜杠的 include 路径
        match = include_re.search(line)
        if match:
            header = match.group(1)
            # 跳过stm32开头的头文件
            if header.lower().startswith("stm32"):
                pass  # 不修正
            # 只替换如 #include "usb_device.h" 这种形式
            elif '/' not in header and '\\' not in header:
                line = line.replace(f'#include "{header}"', f'#include "DLCh/{header}"')
        fixed_lines.append(line)
    with open(dst_file_path, "w", encoding="utf-8") as f:
        f.writelines(fixed_lines)

if __name__ == "__main__":
    PROJECT_ROOT = os.path.abspath(os.path.dirname(__file__))
    collect_files_and_generate_header(PROJECT_ROOT)
    print("\nDone! .c文件已在 Core/Src/DLCc，.h文件已在 Core/Inc/DLCh，all_dlch_includes.h 生成于 Core/Inc。")