import os

Import("env")

LIB_DIR = env.subst("$PROJECT_DIR/.pio/external_lib/stmems")
GIT_REPO="https://github.com/STMicroelectronics/STMems_Standard_C_drivers"

if not os.path.exists(LIB_DIR):
    print("Cloning repo "+GIT_REPO+" to "+LIB_DIR)
    env.Execute("git clone --depth 100 "+GIT_REPO+" "+LIB_DIR)
else:
    print("SKIP - Updating repo in "+LIB_DIR)
    #env.Execute("git --work-tree="+LIB_DIR+" --git-dir="+LIB_DIR+"/.git pull origin master --depth 100")