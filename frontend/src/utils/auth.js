const TOKEN_KEY = 'algoforge_auth_token'
const USER_KEY = 'algoforge_auth_user'

export const getAuthToken = () => localStorage.getItem(TOKEN_KEY) || ''

export const getAuthUser = () => {
  try {
    return JSON.parse(localStorage.getItem(USER_KEY) || 'null')
  } catch (error) {
    return null
  }
}

export const setAuthSession = ({ token, user }) => {
  localStorage.setItem(TOKEN_KEY, token)
  localStorage.setItem(USER_KEY, JSON.stringify(user))
  window.dispatchEvent(new Event('algoforge-auth-changed'))
}

export const clearAuthSession = () => {
  localStorage.removeItem(TOKEN_KEY)
  localStorage.removeItem(USER_KEY)
  window.dispatchEvent(new Event('algoforge-auth-changed'))
}

export const isAuthenticated = () => Boolean(getAuthToken())
