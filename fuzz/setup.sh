# To be sourced by the shell that starts a new fuzzing session

AFL_PATH="$HOME/projects/ctfs_and_challenges/Fuzzing101/afl_build_root/usr/local/bin"
if [ -d "$AFL_PATH" ] && ! command -v afl-fuzz >/dev/null 2>&1; then
    echo "Adding AFL binaries to path"
    PATH="$AFL_PATH:$PATH"
fi
unset AFL_PATH

echo "Using sudo to switch all cores to performance mode"
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor &>/dev/null

echo "Using sudo to disable any coredumps"
echo core | sudo tee /proc/sys/kernel/core_pattern &>/dev/null

# sudo afl-persistent-config
