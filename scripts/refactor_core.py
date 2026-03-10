#!/usr/bin/env python3
"""
Core 文件夹重构脚本
将 Core/ 下的文件移动到子文件夹，并更新所有 include 路径
"""

import os
import re
import shutil

# 项目根目录
PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MULTIAGENT_DIR = os.path.join(PROJECT_ROOT, "unreal_project/Source/MultiAgent")
CORE_DIR = os.path.join(MULTIAGENT_DIR, "Core")

# 文件分类
FILE_MAPPING = {
    # GameFlow - 游戏流程
    "GameFlow": [
        "MAGameInstance.h", "MAGameInstance.cpp",
        "MAGameMode.h", "MAGameMode.cpp",
        "MASetupGameMode.h", "MASetupGameMode.cpp",
    ],
    # AgentRuntime / Squad / Camera - 已拆分上下文
    "AgentRuntime/Runtime": [
        "MAAgentManager.h", "MAAgentManager.cpp",
    ],
    "Squad/Runtime": [
        "MASquadManager.h", "MASquadManager.cpp",
    ],
    "Camera/Runtime": [
        "MAViewportManager.h", "MAViewportManager.cpp",
    ],
    # Comm - 通信相关
    "Comm": [
        "MACommSubsystem.h", "MACommSubsystem.cpp",
        "MACommSubsystemTest.cpp",
        "MACommTypes.h", "MACommTypes.cpp",
    ],
    # Shared Types - 共享合同类型
    "Shared/Types": [
        "MATypes.h",
        "MASimTypes.h", "MASimTypes.cpp",
    ],
    "TaskGraph/Domain": [
        "MATaskGraphTypes.h",
        "MATaskGraphTypes.cpp",
    ],
    "TaskGraph/Application": [
        "MATaskGraphUseCases.h", "MATaskGraphUseCases.cpp",
    ],
    "TaskGraph/Feedback": [
        "MATaskGraphFeedback.h",
    ],
    "TaskGraph/Infrastructure": [
        "MATaskGraphJsonCodec.h", "MATaskGraphJsonCodec.cpp",
    ],
    "TaskGraph/Bootstrap": [
        "MATaskGraphBootstrap.h", "MATaskGraphBootstrap.cpp",
    ],
    "SkillAllocation/Domain": [
        "MASkillAllocationTypes.h",
        "MASkillAllocationTypes.cpp",
    ],
    "SkillAllocation/Application": [
        "MASkillAllocationUseCases.h", "MASkillAllocationUseCases.cpp",
    ],
    "SkillAllocation/Feedback": [
        "MASkillAllocationFeedback.h",
    ],
    "SkillAllocation/Infrastructure": [
        "MASkillAllocationJsonCodec.h", "MASkillAllocationJsonCodec.cpp",
    ],
    "SkillAllocation/Bootstrap": [
        "MASkillAllocationBootstrap.h", "MASkillAllocationBootstrap.cpp",
    ],
    "Squad/Application": [
        "MASquadUseCases.h", "MASquadUseCases.cpp",
    ],
    "Squad/Bootstrap": [
        "MASquadBootstrap.h", "MASquadBootstrap.cpp",
    ],
    "Squad/Domain": [
        "MASquad.h", "MASquad.cpp",
    ],
    "Squad/Feedback": [
        "MASquadFeedback.h",
    ],
    "Squad/Infrastructure": [
        "MASquadRuntimeBridge.h", "MASquadRuntimeBridge.cpp",
    ],
    # 保留在根目录
    "": [
        "MARTSCompatibilityTest.cpp",
    ],
}

# 构建文件到子文件夹的映射
def build_file_to_folder_map():
    result = {}
    for folder, files in FILE_MAPPING.items():
        for f in files:
            result[f] = folder
    return result

FILE_TO_FOLDER = build_file_to_folder_map()

# include 路径更新规则
def get_new_include_path(old_include, current_file_folder):
    """
    根据旧的 include 路径和当前文件所在文件夹，计算新的 include 路径
    """
    # 提取文件名
    match = re.search(r'["\']([^"\']+)["\']', old_include)
    if not match:
        return old_include
    
    include_path = match.group(1)
    
    # 只处理 Core/ 相关的 include
    if "Core/" not in include_path and not include_path.startswith("MA"):
        return old_include
    
    # 提取被 include 的文件名
    filename = os.path.basename(include_path)
    
    # 查找该文件应该在哪个子文件夹
    target_folder = FILE_TO_FOLDER.get(filename, None)
    if target_folder is None:
        return old_include  # 不在映射中，保持不变
    
    # 计算相对路径
    if current_file_folder == target_folder:
        # 同一文件夹，直接引用
        new_path = filename
    elif current_file_folder == "":
        # 当前在根目录，目标在子文件夹
        new_path = f"{target_folder}/{filename}" if target_folder else filename
    elif target_folder == "":
        # 当前在子文件夹，目标在根目录
        new_path = f"../{filename}"
    else:
        # 都在子文件夹
        new_path = f"../{target_folder}/{filename}"
    
    # 替换路径
    return old_include.replace(include_path, new_path)

def update_includes_in_file(filepath, current_folder):
    """更新文件中的 include 路径"""
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    lines = content.split('\n')
    updated_lines = []
    changed = False
    
    for line in lines:
        if line.strip().startswith('#include'):
            new_line = get_new_include_path(line, current_folder)
            if new_line != line:
                changed = True
                print(f"  {line.strip()} -> {new_line.strip()}")
            updated_lines.append(new_line)
        else:
            updated_lines.append(line)
    
    if changed:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write('\n'.join(updated_lines))
    
    return changed

def move_files():
    """移动文件到子文件夹"""
    for folder, files in FILE_MAPPING.items():
        if folder:  # 跳过根目录
            target_dir = os.path.join(CORE_DIR, folder)
            os.makedirs(target_dir, exist_ok=True)
            
            for filename in files:
                src = os.path.join(CORE_DIR, filename)
                dst = os.path.join(target_dir, filename)
                if os.path.exists(src):
                    print(f"Moving {filename} -> {folder}/")
                    shutil.move(src, dst)

def update_all_includes():
    """更新所有文件的 include 路径"""
    # 更新 Core 目录下的文件
    for folder, files in FILE_MAPPING.items():
        folder_path = os.path.join(CORE_DIR, folder) if folder else CORE_DIR
        for filename in files:
            filepath = os.path.join(folder_path, filename)
            if os.path.exists(filepath):
                print(f"Updating includes in {folder}/{filename}" if folder else f"Updating includes in {filename}")
                update_includes_in_file(filepath, folder)
    
    # 更新其他目录的文件
    for subdir in ["Agent", "UI", "Input", "Environment"]:
        subdir_path = os.path.join(MULTIAGENT_DIR, subdir)
        if os.path.exists(subdir_path):
            for root, dirs, files in os.walk(subdir_path):
                for filename in files:
                    if filename.endswith(('.h', '.cpp')):
                        filepath = os.path.join(root, filename)
                        print(f"Checking {os.path.relpath(filepath, MULTIAGENT_DIR)}")
                        update_external_includes(filepath)

def update_external_includes(filepath):
    """更新 Core 目录外的文件中对 Core 的引用"""
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    lines = content.split('\n')
    updated_lines = []
    changed = False
    
    for line in lines:
        if '#include' in line and 'Core/' in line:
            # 提取文件名
            match = re.search(r'Core/([^"\']+)', line)
            if match:
                filename = match.group(1)
                target_folder = FILE_TO_FOLDER.get(filename, None)
                if target_folder is not None:
                    if target_folder:
                        new_path = f"Core/{target_folder}/{filename}"
                    else:
                        new_path = f"Core/{filename}"
                    new_line = line.replace(f"Core/{filename}", new_path)
                    if new_line != line:
                        changed = True
                        print(f"  {line.strip()} -> {new_line.strip()}")
                        line = new_line
        updated_lines.append(line)
    
    if changed:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write('\n'.join(updated_lines))

if __name__ == "__main__":
    print("=== Core 文件夹重构 ===\n")
    
    print("1. 移动文件到子文件夹...")
    move_files()
    
    print("\n2. 更新 include 路径...")
    update_all_includes()
    
    print("\n=== 完成 ===")
