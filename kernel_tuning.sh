#!/usr/bin/bash

# https://ioi.github.io/checklist/
# To attain more deterministic execution times on contestant machines and workers,
# some kernel and system settings should be tuned.
# Please run this script on superuser to disable swap.

RES = $(stat -fc %T /sys/fs/cgroup)

if [ "$RES" != "tmpfs" ]; then
    echo "cgroup v2 -- isolate is not supported in this environment!\n"
    echo "add kernel parameters below:\n"
    echo "systemd.unified_cgroup_hierarchy=false systemd.legacy_systemd_cgroup_controller=false\n"
    echo "sudo vi /etc/default/grub and add above parameters to GRUB_CMDLINE_LINUX_DEFAULT, after quiet splash\n"
    echo "then run sudo update-grub\n"
    exit 1
fi

echo 0 > /proc/sys/kernel/randomize_va_space 
echo never > /sys/kernel/mm/transparent_hugepage/enabled
echo never > /sys/kernel/mm/transparent_hugepage/defrag
echo 0 > /sys/kernel/mm/transparent_hugepage/khugepaged/defrag
sudo swapoff -a
