cmake_minimum_required(VERSION 3.21)
project(simulator)

set(CMAKE_CXX_STANDARD 14)

add_executable(simulator main.cpp Scheduler.cpp Scheduler.h Scheduler_EDF_VD.cpp Scheduler_FMC.cpp Scheduler_FMC_Drop.cpp Scheduler_H_FMC.cpp Scheduler_RED.cpp)
add_executable(taskGen main_task_gen.cpp)
