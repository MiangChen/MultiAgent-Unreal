import os
import argparse
import zipfile
import sys
import shutil
import unrealcv

modelscope = {
    'UE5': 'UnrealZoo/UnrealZoo-UE5',
}

binary_linux = dict(
    UE5_ExampleScene='UE5_ExampleScene_Linux.zip',
    Textures='Textures.zip'
)

binary_win = dict(
    UE5_ExampleScene='UE5_ExampleScene_Win.zip',
    Textures='Textures.zip'
)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=None)
    parser.add_argument("-e", "--env", nargs='?', default='Textures',
                        help='Select the binary to download')
    parser.add_argument("-cloud", "--cloud", nargs='?', default='modelscope',
                        help='Select the cloud to download the binary, modelscope or aliyun')
    args = parser.parse_args()

    if 'linux' in sys.platform:
        binary_all = binary_linux
    elif 'win' in sys.platform:
        binary_all = binary_win
    else:
        print(f"Platform {sys.platform} is not supported")
        exit()

    if args.env in binary_all:
        target_name = binary_all[args.env]
    else:
        print(f"{args.env} is not available to your platform")
        exit()

    if args.cloud == 'modelscope':
        remote_repo = modelscope['UE5']
        cmd = f"modelscope download --dataset {remote_repo} --include {target_name} --local_dir ."
        try:
            os.system(cmd)
        except:
            print('Please install modelscope first: pip install modelscope')
            exit()
        filename = target_name

    with zipfile.ZipFile(filename, "r") as z:
        z.extractall()
    if 'Textures' in filename:
        folder = 'textures'
    else:
        folder = filename[:-4]
    target = unrealcv.util.get_path2UnrealEnv()
    shutil.move(folder, target)
    os.remove(filename)

