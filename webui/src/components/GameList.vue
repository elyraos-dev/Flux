<template>
  <div>
    <h1 class="text-h4 font-weight-bold mb-6 text-white">Game List</h1>

    <v-card color="surface" variant="outlined" class="pa-4 mb-4">
      <v-card-title class="text-primary">
        <v-icon start>mdi-gamepad-variant</v-icon>
        Manage Games
      </v-card-title>
      <v-card-text>
        <v-textarea
          v-model="gameListText"
          label="Game Package Names (one per line)"
          variant="outlined"
          color="primary"
          bg-color="background"
          rows="12"
          placeholder="com.example.game"
        ></v-textarea>
        <div class="d-flex gap-2 mt-4">
          <v-btn color="primary" prepend-icon="mdi-content-save" @click="saveGames">
            Save List
          </v-btn>
          <v-btn color="secondary" variant="outlined" prepend-icon="mdi-restore" @click="loadDefaults">
            Load Defaults
          </v-btn>
        </div>
      </v-card-text>
    </v-card>

    <v-card color="surface" variant="outlined" class="pa-4">
      <v-card-title class="text-info">
        <v-icon start>mdi-help-circle</v-icon>
        How to Find Package Names
      </v-card-title>
      <v-card-text class="text-grey">
        <ol class="ml-4">
          <li>Install <strong>App Inspector</strong> from Play Store</li>
          <li>Open the app and find your game</li>
          <li>Copy the "Package name" field</li>
          <li>Paste it above and save</li>
        </ol>
      </v-card-text>
    </v-card>
  </div>
</template>

<script setup>
import { ref } from 'vue'

const defaultGames = `com.miHoYo.GenshinImpact
com.HoYoverse.hkrpgoversea
com.tencent.ig
com.mobile.legends
com.dts.freefireth
com.activision.callofduty.shooter
com.garena.game.kgtw
com.epicgames.fortnite
com.riotgames.league.wildrift`

const gameListText = ref(defaultGames)

const saveGames = () => {
  // In real implementation, write to /data/adb/flux/config/gamelist.txt
  alert('Game list saved!')
}

const loadDefaults = () => {
  gameListText.value = defaultGames
}
</script>
