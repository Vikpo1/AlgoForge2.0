import http from './http'

export const login = (payload) => {
  return http.post('/api/auth/login', payload)
}

export const register = (payload) => {
  return http.post('/api/auth/register', payload)
}

export const getCurrentUser = () => {
  return http.get('/api/auth/me')
}
