import { createRouter, createWebHashHistory } from 'vue-router'
import Dashboard from './components/Dashboard.vue'
import GameList from './components/GameList.vue'
import Settings from './components/Settings.vue'
import About from './components/About.vue'

const routes = [
  { path: '/', component: Dashboard },
  { path: '/games', component: GameList },
  { path: '/settings', component: Settings },
  { path: '/about', component: About }
]

export default createRouter({
  history: createWebHashHistory(),
  routes
})
