#!/usr/bin/bash

# https://ioi.github.io/checklist/
# To attain more deterministic execution times on contestant machines and workers,
# some kernel and system settings should be tuned.
# Please run this script on superuser to disable swap.

echo 0 > /proc/sys/kernel/randomize_va_space 
echo never > /sys/kernel/mm/transparent_hugepage/enabled
echo never > /sys/kernel/mm/transparent_hugepage/defrag
echo 0 > /sys/kernel/mm/transparent_hugepage/khugepaged/defrag
sudo swapoff -a
