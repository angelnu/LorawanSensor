import os
import argparse

################################################
# WIP: this was intended to chekout a submodule as lib but I found a work around without extra script
# I keep this around in case I need it later.
################################################

parser = argparse.ArgumentParser(description='Git library checkout')
parser.add_argument('--git_url',
                    dest='git_url',
                    help='Git URL to clone from',
                    required=True)
parser.add_argument('--output_dir',
                    dest='output_dir',
                    help='Folder for checkout',
                    required=True)
parser.add_argument('--git_subdir',
                    dest='git_subdir',
                    help='git subfolder to use as library -> will be placed in <output_dir>/<repo name>')
parser.add_argument('--lib_name',
                    dest='lib_name',
                    help='library dir name containing submodule')


args = parser.parse_args()

print("git_url: "+args.git_url)
print("output_dir: "+args.output_dir)
print("git_subdir: "+args.git_subdir)
print("lib_name: "+args.lib_name)

if git_subdir:
    LIB_DIR = args.output_dir + "_git"
else:
    LIB_DIR = args.output_dir
GIT_REPO="https://github.com/STMicroelectronics/STMems_Standard_C_drivers"

if not os.path.exists(LIB_DIR):
    print("Cloning repo "+args.git_url+" to "+LIB_DIR)
    env.Execute("git clone --depth 100 "+GIT_REPO+" "+LIB_DIR)
else:
    print("SKIP - Updating repo in "+LIB_DIR)
    #env.Execute("git --work-tree="+LIB_DIR+" --git-dir="+LIB_DIR+"/.git pull origin master --depth 100")

#if args.git_subdir:
#    TBD
#    COPY_DIR = 