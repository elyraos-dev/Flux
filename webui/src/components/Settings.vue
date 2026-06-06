<template>
  <div>
    <h1 class="text-h4 font-weight-bold mb-6 text-white">Settings</h1>

    <v-row>
      <v-col cols="12" md="6">
        <v-card color="surface" variant="outlined" class="pa-4">
          <v-card-title class="text-primary">
            <v-icon start>mdi-tune</v-icon>
            General
          </v-card-title>
          <v-card-text>
            <v-switch
              v-model="settings.autoDetect"
              label="Auto-detect Games"
              color="primary"
              hide-details
              class="mb-2"
            ></v-switch>
            <v-switch
              v-model="settings.verboseLogging"
              label="Verbose Logging"
              color="primary"
              hide-details
              class="mb-2"
            ></v-switch>
            <v-slider
              v-model="settings.profileDelay"
              label="Profile Switch Delay (sec)"
              min="1"
              max="10"
              step="1"
              color="primary"
              thumb-label
              class="mt-4"
            ></v-slider>
          </v-card-text>
        </v-card>
      </v-col>

      <v-col cols="12" md="6">
        <v-card color="surface" variant="outlined" class="pa-4">
          <v-card-title class="text-warning">
            <v-icon start>mdi-battery-alert</v-icon>
            Battery
          </v-card-title>
          <v-card-text>
            <v-slider
              v-model="settings.powersaveThreshold"
              label="Powersave Threshold (%)"
              min="5"
              max="50"
              step="5"
              color="warning"
              thumb-label
            ></v-slider>
            <v-switch
              v-model="settings.disableThermal"
              label="Disable Thermal on Performance"
              color="error"
              hide-details
              class="mt-2"
            ></v-switch>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <v-row class="mt-4">
      <v-col cols="12">
        <v-card color="surface" variant="outlined" class="pa-4">
          <v-card-title class="text-secondary">
            <v-icon start>mdi-memory</v-icon>
            Advanced
          </v-card-title>
          <v-card-text>
            <v-row>
              <v-col cols="12" md="4">
                <v-select
                  v-model="settings.perfGovernor"
                  label="Performance Governor"
                  :items="['performance', 'schedutil']"
                  variant="outlined"
                  color="primary"
                ></v-select>
              </v-col>
              <v-col cols="12" md="4">
                <v-select
                  v-model="settings.normalGovernor"
                  label="Normal Governor"
                  :items="['schedutil', 'conservative', 'ondemand']"
                  variant="outlined"
                  color="primary"
                ></v-select>
              </v-col>
              <v-col cols="12" md="4">
                <v-select
                  v-model="settings.ioScheduler"
                  label="I/O Scheduler"
                  :items="['noop', 'cfq', 'bfq', 'mq-deadline']"
                  variant="outlined"
                  color="primary"
                ></v-select>
              </v-col>
            </v-row>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <v-row class="mt-4">
      <v-col cols="12" class="d-flex justify-end">
        <v-btn color="primary" size="large" prepend-icon="mdi-content-save" @click="saveSettings">
          Apply Settings
        </v-btn>
      </v-col>
    </v-row>
  </div>
</template>

<script setup>
import { reactive } from 'vue'

const settings = reactive({
  autoDetect: true,
  verboseLogging: true,
  profileDelay: 3,
  powersaveThreshold: 20,
  disableThermal: false,
  perfGovernor: 'performance',
  normalGovernor: 'schedutil',
  ioScheduler: 'noop'
})

const saveSettings = () => {
  alert('Settings applied!')
}
</script>
