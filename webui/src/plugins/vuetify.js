import { createVuetify } from 'vuetify'
import 'vuetify/styles'
import { aliases, mdi } from 'vuetify/iconsets/mdi'

export default createVuetify({
  theme: {
    defaultTheme: 'fluxDark',
    themes: {
      fluxDark: {
        dark: true,
        colors: {
          primary: '#00E5FF',
          secondary: '#2979FF',
          accent: '#FF4081',
          background: '#0A0E17',
          surface: '#121826',
          error: '#FF5252',
          info: '#2196F3',
          success: '#4CAF50',
          warning: '#FFC107'
        }
      }
    }
  },
  icons: {
    defaultSet: 'mdi',
    aliases,
    sets: { mdi }
  }
})
