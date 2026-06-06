<template>
  <div>
    <h1 class="text-h4 font-weight-bold mb-6 text-white">Dashboard</h1>

    <v-row>
      <v-col cols="12" md="4">
        <v-card color="surface" variant="outlined" class="pa-4">
          <v-card-title class="text-primary">
            <v-icon start>mdi-flash</v-icon>
            Current Profile
          </v-card-title>
          <v-card-text class="text-center py-6">
            <v-icon size="64" :color="profileColor">{{ profileIcon }}</v-icon>
            <div class="text-h3 font-weight-bold mt-4" :class="`text-${profileColor}`">
              {{ currentProfile }}
            </div>
            <v-chip class="mt-2" :color="profileColor" variant="tonal">
              {{ profileDesc }}
            </v-chip>
          </v-card-text>
        </v-card>
      </v-col>

      <v-col cols="12" md="4">
        <v-card color="surface" variant="outlined" class="pa-4">
          <v-card-title class="text-info">
            <v-icon start>mdi-cpu-64-bit</v-icon>
            CPU Status
          </v-card-title>
          <v-card-text>
            <div class="d-flex justify-space-between mb-2">
              <span>Governor</span>
              <span class="font-weight-bold text-primary">{{ cpuGovernor }}</span>
            </div>
            <div class="d-flex justify-space-between mb-2">
              <span>Cores</span>
              <span class="font-weight-bold">{{ cpuCores }}</span>
            </div>
            <div class="d-flex justify-space-between">
              <span>Max Freq</span>
              <span class="font-weight-bold text-success">{{ maxFreq }} MHz</span>
            </div>
          </v-card-text>
        </v-card>
      </v-col>

      <v-col cols="12" md="4">
        <v-card color="surface" variant="outlined" class="pa-4">
          <v-card-title class="text-warning">
            <v-icon start>mdi-battery</v-icon>
            Battery
          </v-card-title>
          <v-card-text class="text-center py-6">
            <v-progress-circular
              :model-value="batteryLevel"
              :color="batteryColor"
              size="80"
              width="8"
            >
              <span class="text-h6 font-weight-bold">{{ batteryLevel }}%</span>
            </v-progress-circular>
            <div class="mt-4 text-caption text-grey">{{ batteryStatus }}</div>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <v-row class="mt-4">
      <v-col cols="12">
        <v-card color="surface" variant="outlined" class="pa-4">
          <v-card-title class="text-secondary">
            <v-icon start>mdi-format-list-bulleted</v-icon>
            Monitored Games
          </v-card-title>
          <v-card-text>
            <v-chip-group>
              <v-chip
                v-for="game in games"
                :key="game"
                color="primary"
                variant="outlined"
                size="small"
              >
                {{ game }}
              </v-chip>
            </v-chip-group>
            <div v-if="games.length === 0" class="text-grey text-center py-4">
              No games configured. Add games in the Game List tab.
            </div>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>
  </div>
</template>

<script setup>
import { ref, computed } from 'vue'

const currentProfile = ref('Performance')
const cpuGovernor = ref('performance')
const cpuCores = ref(8)
const maxFreq = ref(2840)
const batteryLevel = ref(72)
const batteryStatus = ref('Discharging')
const games = ref([
  'Genshin Impact',
  'PUBG Mobile',
  'Mobile Legends',
  'Call of Duty Mobile'
])

const profileColor = computed(() => {
  const map = { 'Performance': 'primary', 'Normal': 'success', 'Powersave': 'warning' }
  return map[currentProfile.value] || 'grey'
})

const profileIcon = computed(() => {
  const map = { 'Performance': 'mdi-rocket-launch', 'Normal': 'mdi-check-circle', 'Powersave': 'mdi-leaf' }
  return map[currentProfile.value] || 'mdi-help-circle'
})

const profileDesc = computed(() => {
  const map = {
    'Performance': 'Maximum gaming performance',
    'Normal': 'Balanced daily usage',
    'Powersave': 'Extended battery life'
  }
  return map[currentProfile.value] || ''
})

const batteryColor = computed(() => {
  if (batteryLevel.value > 50) return 'success'
  if (batteryLevel.value > 20) return 'warning'
  return 'error'
})
</script>
