# CMake generated Testfile for 
# Source directory: /home/sakura/text/distributed-task-system
# Build directory: /home/sakura/text/distributed-task-system/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
subdirs("_deps/grpc-build")
subdirs("_deps/hiredis-build")
subdirs("_deps/googletest-build")
subdirs("_deps/glog-build")
subdirs("src/common")
subdirs("src/api-server")
subdirs("src/worker/task-executor")
subdirs("tests")
