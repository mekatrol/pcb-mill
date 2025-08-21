set pagination off
set logging enabled on
target extended-remote localhost:3333
monitor reset init
monitor halt
load
continue
