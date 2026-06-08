#
# Copyright (C) 2024-2026 FebriCahyaa
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

MODDIR=$(dirname "$0")
MODULE_CONFIG="/data/adb/.config/flux"
CLEANUP_SCRIPT="/data/adb/service.d/.flux_cleanup.sh"
CPUFREQ="/sys/devices/system/cpu/cpu0/cpufreq"

# Restore original module.prop
[ -f "$MODDIR/module.prop.orig" ] && {
  cp "$MODDIR/module.prop.orig" "$MODDIR/module.prop"
}

# Rotate logs: keep previous boot's sysmon.log for crash diagnosis
[ -f "$MODULE_CONFIG/sysmon.log" ] && mv "$MODULE_CONFIG/sysmon.log" "$MODULE_CONFIG/sysmon.log.prev"
rm -f "$MODULE_CONFIG/flux.log"

# Parse Governor to use
chmod 644 "$CPUFREQ/scaling_governor"
default_gov=$(cat "$CPUFREQ/scaling_governor")
echo "$default_gov" >$MODULE_CONFIG/default_cpu_gov

# Create cleanup script
[ ! -f "$CLEANUP_SCRIPT" ] && {
  mkdir -p "$(dirname $CLEANUP_SCRIPT)"
  cp "$MODDIR/cleanup.sh" "$CLEANUP_SCRIPT"
  chmod +x "$CLEANUP_SCRIPT"
}

# Wait until boot completed
while [ -z "$(getprop sys.boot_completed)" ]; do
	sleep 40
done

# Handle case when 'default_gov' is performance
default_gov_preferred_array="
scx
schedhorizon
walt
sched_pixel
sugov_ext
uag
schedplus
energy_step
schedutil
interactive
conservative
powersave
"

if [ "$default_gov" == "performance" ]; then
	for gov in $default_gov_preferred_array; do
		grep -q "$gov" "$CPUFREQ/scaling_available_governors" && {
			echo "$gov" >$MODULE_CONFIG/default_cpu_gov
			default_gov="$gov"
			break
		}
	done
fi

# Revert to normal CPU governor
echo "$default_gov" | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# ── Detect GKI vs Non-GKI kernel and cache result ────────────────────────────
KERNEL_VER=$(uname -r)
if echo "$KERNEL_VER" | grep -qE "\-android[0-9]+-"; then
	IS_GKI=1
	echo "GKI" > "$MODULE_CONFIG/kernel_type"
else
	IS_GKI=0
	echo "Non-GKI" > "$MODULE_CONFIG/kernel_type"
fi
echo "$IS_GKI" > "$MODULE_CONFIG/is_gki"

# ── Thermal headroom API availability check ───────────────────────────────────
# Android Thermal API (getThermalHeadroom) requires API level 31+ (Android 12).
# On GKI kernels, the Java-side daemon exposes this via SynthesisCore.
# Write a hint file so the WebUI can show a meaningful explanation when unavailable.
SDK=$(getprop ro.build.version.sdk 2>/dev/null || echo "0")
if [ "$SDK" -lt 31 ]; then
	echo "unsupported_api_level" > "$MODULE_CONFIG/thermal_api_status"
elif [ "$IS_GKI" -eq 1 ]; then
	echo "gki_may_vary" > "$MODULE_CONFIG/thermal_api_status"
else
	echo "supported" > "$MODULE_CONFIG/thermal_api_status"
fi

# Mitigate buggy thermal throttling on post-startup
# in old MediaTek devices.
ENABLE_PPM="/proc/ppm/enabled"
if [ -f "$ENABLE_PPM" ]; then
	echo 0 >"$ENABLE_PPM"
	sleep 1
	echo 1 >"$ENABLE_PPM"
fi

# Start Flux Daemon
nohup app_process -Djava.class.path="$MODDIR/synthesiscore.apk" / --nice-name=FluxSysMon com.febricahyaa.synthesiscore.MainKt "$MODULE_CONFIG/synthesis_core.json" "$MODULE_CONFIG/java.lock" >"$MODULE_CONFIG/sysmon.log" 2>&1 &
sleep 1 # Buffer
fluxd daemon
